/// @file doctors/real_doctors.hpp
/// @brief Multiprocess real doctors queue
#pragma once

#include "../doctors_queue.hpp"

#include <map>
#include <repast_hpc/AgentId.h>

#include "../clock.hpp"

// Fw. declarations
namespace boost {
template <typename T>
class optional;
}
namespace sti {
class hospital_plan;
}

namespace sti {

class real_doctors final : public doctors_queue {

public:
    /// @brief The list of doctors
    using single_queue        = std::list<patient_turn>;
    using patients_queue_type = std::map<specialty_type, single_queue>;

    /// @brief Construct real queue, specifing the rank of the real queue
    /// @param communicator The MPI Communicator
    /// @param mpi_tag The MPI tag used for comunication
    /// @param hospital The hospital plan, to extract the doctors
    real_doctors(communicator_ptr communicator, int mpi_tag, const hospital_plan& hospital);

    /// @brief Enqueue in a doctor type
    /// @param type The doctor specialization to enqueue in
    /// @param id The agent id
    /// @param timeout Instant of time that the patient will leave if doesn't receive attention
    void enqueue(const specialty_type& type, const repast::AgentId& id, const datetime& timeout) override;

    /// @brief Remove an agent from the queues
    /// @param type The doctor type/specialty to dequeue from
    /// @param id The agent id to dequeue
    void dequeue(const specialty_type& type, const agent_id& id) override;

    /// @brief Check if the agent has a turn assigned. Returns the location of the doctor to go
    /// @param type The doctor type/specialty to check
    /// @param id The agent ID to check
    /// @return If the agent has a doctor assigned, the destination
    boost::optional<position> is_my_turn(const specialty_type& type, const agent_id& id) override;

    /// @brief Sync the queues between the processes
    void sync() override;

private:
    communicator_ptr _communicator;
    int              _my_rank;
    int              _base_tag;

    // doctors             _doctors;
    front_type          _front;
    patients_queue_type _patients_queue;

    // Helper functions

    /// @brief Insert a new patient turn in order
    /// @param type The doctor type/specialty to enqueue in
    /// @param turn The patient turn, containing id and timeout/priority
    void insert_in_order(const specialty_type& type, const patient_turn& turn);

    /// @brief Remove an agent from a queue
    /// @param type The doctor type to dequeue from
    /// @param id The agent id
    void remove_patient(const specialty_type& type, const agent_id& id);
};

} // namespace sti