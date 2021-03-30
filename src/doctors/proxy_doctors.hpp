/// @file doctors/proxy_doctors.hpp
/// @brief Multiprocess proxy doctors queue
#pragma once

#include "../doctors_queue.hpp"

namespace sti {

class proxy_doctors final : public doctors_queue {

public:
    /// @brief Construct proxy queue, specifing the rank of the real queue
    /// @param communicator The MPI Communicator
    /// @param real_rank The rank of the process containing the real queue
    /// @param mpi_tag The MPI tag used for comunication
    proxy_doctors(communicator_ptr communicator, int real_rank, int mpi_tag);

    /// @brief Enqueue in a doctor type
    /// @param type The doctor specialization to enqueue in
    /// @param id The agent id
    /// @param timeout Instant of time that the patient will leave if doesn't receive attention
    void enqueue(const doctor_type& type, const repast::AgentId& id,  const datetime& timeout) override;
    
    /// @brief Remove an agent from the queues
    /// @param type The doctor type/specialty to dequeue from
    /// @param id The agent id to dequeue
    void dequeue(const doctor_type& type, const agent_id& id) override;

    /// @brief Check if the agent has a turn assigned. Returns the location of the doctor to go
    /// @param type The doctor type/specialty to check
    /// @param id The agent ID to check
    /// @return If the agent has a doctor assigned, the destination
    boost::optional<position> is_my_turn(const doctor_type& type, const agent_id& id) override;

    /// @brief Sync the queues between the processes
    void sync() override;

private:
    communicator_ptr _communicator;
    int              _real_rank;
    int              _base_tag;
    front_type       _front;

    std::vector<std::pair<doctor_type, patient_turn>>    _enqueue_buffer;
    std::vector<std::pair<doctor_type, repast::AgentId>> _dequeue_buffer;
};

} // namespace sti