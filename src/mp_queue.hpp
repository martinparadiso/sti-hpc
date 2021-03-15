/// @file mp_queue.hpp
/// @brief Implements a multi-process queue, with a proxy pattern
#pragma once

#include <boost/optional.hpp>
#include <cstdint>

namespace sti {

/// @brief A cross-process message queue that of type T
/// @details A cross-process queue, the queue resides in one process, and the
///          rest use a proxy class that communicates over MPI. The type T must
///          be supported by Boost.MPI or Boost.Serialization. The corresponding
///          serialization must be included.
/// @tparam T The type to store
/// @tparam TAG The MPI tag used to identify the communication
template <typename T, int TAG>
class mp_queue {

public:
    using value_type = T;

    mp_queue() = default;

    mp_queue(const mp_queue&) = default;
    mp_queue& operator=(const mp_queue&) = default;

    mp_queue(mp_queue&&) noexcept = default;
    mp_queue& operator=(mp_queue&&) noexcept = default;

    virtual ~mp_queue() = default;

    /// @brief Add a new element to the queue
    /// @param v The element to insert
    virtual void put(const value_type& v) = 0;

    /// @brief Request an element, that will be delivered in the next iteration
    /// @details Due to the tick-based synchronization, the proxy queue
    ///          communicates with the real queue (that holds all the
    ///          values) once per tick. In order to dequeue an element, it
    ///          has to be requested first: in the next synchronization the
    ///          proxy queue will ask for N elements, one for each time this
    ///          function has been called. The real queue will then return at
    ///          most N elements, depending on availability.
    ///          In the case of the real queue, no action is performed.
    virtual void request_element() = 0;

    /// @brief Get an element from the queue
    /// @return An optional containing an element, if there are any
    virtual boost::optional<value_type> get() = 0;

    /// @brief Synchronize the proxy queues with the real queue
    virtual void sync() = 0;

}; // class mp_queue

} // namespace sti