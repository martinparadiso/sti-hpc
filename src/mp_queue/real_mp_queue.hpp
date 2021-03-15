/// @file mp_queue/real_mp_queue.hpp
/// @brief The real class of the mp_queue
#pragma once

#include "../mp_queue.hpp"

#include <boost/mpi/communicator.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <cstdint>
#include <map>
#include <queue>
#include <vector>

namespace sti {

/// @brief The real mp_queue, containing all the values
template <typename T, int TAG>
class real_mp_queue final : public mp_queue<T, TAG> {

public:
    using value_type       = T;
    using mpi_communicator = boost::mpi::communicator*;

    /// @brief Construct a real_mp_queue
    real_mp_queue(mpi_communicator communicator)
        : _communicator { communicator }
    {
    }

    /// @brief Add a new element to the queue
    /// @param v The element to insert
    void put(const value_type& v) override
    {
        _queue.push(v);
    }

    /// @brief Request an element, that will be delivered in the next iteration
    /// @details Due to the tick-based synchronization, the proxy queue
    ///          communicates with the real queue (that holds all the
    ///          values) once per tick. In order to dequeue an element, it
    ///          has to be requested first: in the next synchronization the
    ///          proxy queue will ask for N elements, one for each time this
    ///          function has been called. The real queue will then return at
    ///          most N elements, depending on availability.
    ///          In the case of the real queue, no action is performed.
    void request_element() override
    {
    }

    /// @brief Get an element from the queue
    /// @return An optional containing an element, if there are any
    boost::optional<value_type> get() override
    {
        if (_queue.empty()) return boost::none;

        const auto ret_value = _queue.front();
        _queue.pop();
        return ret_value;
    }

    /// @brief Synchronize the proxy queues with the real queue
    void sync() override
    {
        const auto my_rank    = _communicator->rank();
        const auto world_size = _communicator->size();

        auto new_data = std::vector<std::vector<value_type>> { static_cast<size_t>(world_size), std::vector<value_type> {} };

        // Retrieve all the new elements
        for (auto p = 0; p < world_size; p++) {
            if (p != my_rank) {
                _communicator->recv(p, TAG + 0, new_data[p]);
            }
        }

        // Retrieve the requests
        auto requests = std::map<int, std::uint32_t> {};
        for (auto p = 0; p < world_size; p++) {
            if (p != my_rank) {
                _communicator->recv(p, TAG + 1, requests[p]);
            }
        }

        // Insert the new elements
        for (const auto& vector : new_data) {
            for (const auto& element : vector) {
                _queue.push(element);
            }
        }

        // Fullfil requests
        auto outgoing = std::vector<std::vector<value_type>> { static_cast<size_t>(world_size), std::vector<value_type> {} };
        for (auto p = 0; p < world_size; p++) {
            if (p != my_rank) {
                for (auto pushed = 0; pushed < requests.at(p) && !_queue.empty(); ++pushed) {
                    outgoing[p].push_back(_queue.front());
                    _queue.pop();
                }
            }
        }

        // Send the requested values
        for (auto p = 0; p < world_size; p++) {
            if (p != my_rank) {
                _communicator->send(p, TAG + 2, outgoing[p]);
            }
        }
    }

private:
    mpi_communicator _communicator;
    std::queue<T>    _queue;
}; // class real_mp_queue

} // namespace sti