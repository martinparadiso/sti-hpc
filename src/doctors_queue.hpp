/// @file doctors_queue.hpp
/// @brief Doctor multiprocess queue
#pragma once

#include <repast_hpc/AgentId.h>
#include <cstdint>
#include <map>
#include <string>

#include "clock.hpp"
#include "coordinates.hpp"

namespace boost {
template <typename T>
class optional;
namespace mpi {
    class communicator;
} // namespace mpi
} // namespace boost

namespace repast {
class Properties;
}; // namespace repast

namespace sti {
class hospital_plan;
} // namespace sti

namespace sti {

/// @brief Multiprocess queue that holds the doctors turns
class doctors_queue {

public:
    /// @brief Represents a patient turn,
    struct patient_turn {
        repast::AgentId id;
        sti::datetime   timeout;

        template <typename Archive>
        void serialize(Archive& ar, const unsigned int /*unused*/)
        {
            ar& id;
            ar& timeout;
        }
    };

    using mpi_tag_type     = int;
    using communicator_ptr = boost::mpi::communicator*;

    using doctor_type  = std::string;
    using agent_id = repast::AgentId;
    using position = sti::coordinates<double>;
    /// @brief The type used to reprent the turns
    using front_type = std::map<doctor_type, std::vector<std::pair<position, agent_id>>>;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    doctors_queue() = default;

    doctors_queue(const doctors_queue&) = default;
    doctors_queue& operator=(const doctors_queue&) = default;


    doctors_queue(doctors_queue&&) = default;
    doctors_queue& operator=(doctors_queue&&) = default;

    virtual ~doctors_queue() = default;

    ////////////////////////////////////////////////////////////////////////////
    // QUEUE MANAGEMENT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Enqueue in a doctor type
    /// @param type The doctor specialization to enqueue in
    /// @param id The agent id
    /// @param timeout Instant of time that the patient will leave if doesn't receive attention
    virtual void enqueue(const doctor_type& type, const repast::AgentId& id,  const datetime& timeout) = 0;

    /// @brief Remove an agent from the queues
    /// @param type The doctor type/specialty to dequeue from
    /// @param id The agent id to dequeue
    virtual void dequeue(const doctor_type& type, const agent_id& id) = 0;

    /// @brief Check if the agent has a turn assigned. Returns the location of the doctor to go
    /// @param type The doctor type/specialty to check
    /// @param id The agent ID to check
    /// @return If the agent has a doctor assigned, the destination
    virtual boost::optional<position> is_my_turn(const doctor_type& type, const agent_id& id) = 0;

    /// @brief Sync the queues between the processes
    virtual void sync() = 0;

}; // class doctors_queue

} // namespace sti