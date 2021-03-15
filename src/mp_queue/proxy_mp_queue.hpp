/// @file mp_queue/proxy_mp_queue.hpp
/// @brief The proxy class of the mp_queue
#pragma once

#include "../mp_queue.hpp"

#include <boost/mpi/communicator.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/none.hpp>
#include <queue>

namespace sti {

/// @brief The proxy mp_queue
template <typename T, int TAG>
class proxy_mp_queue final : public mp_queue<T, TAG> {

public:
    using value_type       = T;
    using mpi_communicator = boost::mpi::communicator*;

    /// @brief Construct a proxy mp_queue
    proxy_mp_queue(mpi_communicator communicator, int real_rank)
        : _communicator { communicator }
        , _real_rank { real_rank }
        , _request_to_make { 0 }
        , _to_real {}
        , _from_real {}
    {
    } // proxy_mp_queue()

    /// @brief Add a new element to the queue
    /// @param v The element to insert
    void put(const value_type& v) override
    {
        _to_real.push_back(v);
    } // put()

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
        _request_to_make += 1;
    } // request_element()

    /// @brief Get an element from the queue
    /// @return An optional containing an element, if there are any
    boost::optional<value_type> get() override
    {
        if (!_from_real.empty()) {
            const auto ret = *(_from_real.end() - 1);
            _from_real.pop_back();
            return ret;
        }
        return boost::none;
    } // get()

    /// @brief Synchronize the proxy queues with the real queue
    void sync() override
    {

        // First send all the new values
        _communicator->send(_real_rank, TAG + 0, _to_real);

        // Then request the elements
        _communicator->send(_real_rank, TAG + 1, _request_to_make);

        // Finally retrieve the requested values
        auto values_requested = std::vector<value_type> {};
        _communicator->recv(_real_rank, TAG + 2, values_requested);

        _from_real.insert(_from_real.end(), values_requested.begin(), values_requested.end());

        // Clear the values
        _request_to_make = 0;
        _to_real.clear();
    } // sync()

private:
    mpi_communicator _communicator;

    int           _real_rank;
    std::uint32_t _request_to_make;

    // Note: the values are stored in vector because they have no particular
    // order, given that are buffers between the user and the real queue.

    std::vector<value_type> _to_real; // Values to send to the real queue
    std::vector<value_type> _from_real; // Values received from the real queue

}; // class proxy_mp_queue

} // namespace sti