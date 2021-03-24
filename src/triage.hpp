/// @file triage.hpp
/// @brief Implements the triage queue
#pragma once

#include <boost/mpi/communicator.hpp>
#include <memory>

#include "coordinates.hpp"
#include "queue_manager.hpp"

// Fw. declarations
namespace boost {
template <typename T>
class optional;
} // namespace boost

namespace repast {
class Properties;
class AgentId;
} // namespace repastr

namespace sti {
class hospital_plan;
} // namespace sti

namespace sti {

class triage {

public:
    using agent_id         = repast::AgentId;
    using properties_type  = repast::Properties;
    using communicator_ptr = boost::mpi::communicator*;

    /// @brief Construct a triage
    /// @param props Simulation properties
    /// @param comm MPI Communicator
    /// @param plan Hospital plan
    triage(const properties_type& props,
              communicator_ptr       comm,
              const hospital_plan&   plan);

    /// @brief Enqueue an agent into the triage queue
    /// @param id The agent id
    void enqueue(const agent_id& id);

    /// @brief Remove an agent into the triage queue
    /// @param id The agent id
    void dequeue(const agent_id& id);

    /// @brief Check if an agent has a triage assigned
    /// @param id The id of the agent to check
    /// @return If the agent has a triage assigned, the triage location
    boost::optional<sti::coordinates<double>> is_my_turn(const agent_id& id);

    /// @brief Synchronize the queues between the process
    void sync();

private:
    std::unique_ptr<sti::queue_manager> _queue_manager;

}; // class triage

} // namespace sti
