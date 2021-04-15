#include "proxy_doctors.hpp"

#include <algorithm>
#include <boost/mpi/communicator.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/utility.hpp>
#include <repast_hpc/AgentId.h>

/// @brief Construct proxy queue, specifing the rank of the real queue
/// @param communicator The MPI Communicator
/// @param real_rank The rank of the process containing the real queue
/// @param mpi_tag The MPI tag used for comunication
sti::proxy_doctors::proxy_doctors(communicator_ptr communicator, int real_rank, int mpi_tag)
    : _communicator { communicator }
    , _real_rank(real_rank)
    , _base_tag { mpi_tag }
    , _front {}
{
}

/// @brief Enqueue in a doctor type
/// @param type The doctor specialization to enqueue in
/// @param id The agent id
/// @param timeout Instant of time that the patient will leave if doesn't receive attention
void sti::proxy_doctors::enqueue(const doctor_type& type, const repast::AgentId& id, const datetime& timeout)
{
    _enqueue_buffer.push_back({ type, { id, timeout } });
}

/// @brief Remove an agent from the queues
/// @param type The doctor type/specialty to dequeue from
/// @param id The agent id to dequeue
void sti::proxy_doctors::dequeue(const doctor_type& type, const agent_id& id)
{
    _dequeue_buffer.push_back({ type, id });
}

/// @brief Check if the agent has a turn assigned. Returns the location of the doctor to go
/// @param type The doctor type/specialty to check
/// @param id The agent ID to check
/// @return If the agent has a doctor assigned, the destination
boost::optional<sti::doctors_queue::position> sti::proxy_doctors::is_my_turn(const doctor_type& type, const agent_id& id)
{
    const auto& queue = _front[type];
    const auto  it    = std::find_if(queue.begin(), queue.end(),
                                 [&](const auto& turn) {
                                     return turn.second == id;
                                 });

    if (it != queue.end()) {
        return it->first;
    }

    return {};
}

/// @brief Sync the queues between the processes
void sti::proxy_doctors::sync()
{
    auto mpi_tag = _base_tag;

    _communicator->send(_real_rank, mpi_tag++, _enqueue_buffer);
    _communicator->send(_real_rank, mpi_tag++, _dequeue_buffer);
    
    // Clear the queues
    _enqueue_buffer.clear();
    _dequeue_buffer.clear();

    // Use broadcast for the front, more practical
    boost::mpi::broadcast(*_communicator, _front, _real_rank);

}