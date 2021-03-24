/// @file queue_manager.hpp
/// @brief Implements a multi-process pseudo-queue
#pragma once

#include <boost/optional.hpp>
#include <cstdint>
#include <utility>
#include <vector>

#include "coordinates.hpp"

// Fw. declarations
namespace repast {
class AgentId;
}
namespace sti {

/// @brief A cross-process simple queue used to dispatch patients
/// @details A cross-process queue, the queue resides in one process, and the
///          rest use a proxy class that communicates over MPI.
class queue_manager {

public:
    using agent_id = repast::AgentId;

    // The front of the queue, containing the next agents/patients to be
    // attended and their assigned location
    using front_type = std::vector<std::pair<sti::coordinates<double>, agent_id>>;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    queue_manager() = default;

    queue_manager(const queue_manager&) = default;
    queue_manager& operator=(const queue_manager&) = default;

    queue_manager(queue_manager&&) noexcept = default;
    queue_manager& operator=(queue_manager&&) noexcept = default;

    virtual ~queue_manager() = default;

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Add a new patient to the queue
    /// @param id The agent to enqueue
    virtual void enqueue(const agent_id& id) = 0;

    /// @brief Remove a patient from the queue
    /// @param id The id to remove from the queue
    virtual void dequeue(const agent_id& id) = 0;

    /// @brief Check if the given agent is next in the attention
    /// @param id The agent id
    /// @return If the agent is in the front of the queue, the coordinates
    virtual boost::optional<coordinates<double>> is_my_turn(const agent_id& id) = 0;

    /// @brief Synchronize the real queue and the remote queues
    virtual void sync() = 0;

}; // class queue_manager

} // namespace sti
