#include <algorithm>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/map.hpp>
#include <repast_hpc/AgentId.h>

#include "proxy_queue_manager.hpp"

/// @brief Construct a real queue display
/// @param comm The MPI Communicator
/// @param tag The MPI tag
/// @param positions The "boxes"/fronts of the queue
sti::proxy_queue_manager::proxy_queue_manager(communicator_ptr comm, int tag, int real_rank)
    : _communicator(comm)
    , _tag { tag }
    , _real_rank { real_rank }
{
}

////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Add a new patient to the queue
/// @param id The agent to enqueue
void sti::proxy_queue_manager::enqueue(const agent_id& id)
{
    _to_enqueue.push_back(id);
}

/// @brief Remove a patient from the queue
/// @param id The id to remove from the queue
void sti::proxy_queue_manager::dequeue(const agent_id& id)
{
    _to_dequeue.push_back(id);
}

/// @brief Check if the given agent is next in the attention
/// @param id The agent id
/// @return If the agent is in the front of the queue, the coordinates
boost::optional<sti::coordinates<double>> sti::proxy_queue_manager::is_my_turn(const agent_id& id)
{
    // Search for the patient in the box list
    auto box_it = std::find_if(_boxes.begin(), _boxes.end(), [&](const auto& pair) {
        return pair.second == id;
    });

    if (box_it != _boxes.end()) {
        return box_it->first;
    }

    return boost::none;
}

/// @brief Synchronize the real queue and the remote queues
void sti::proxy_queue_manager::sync()
{
    auto mpi_tag = _tag;
    _communicator->send(_real_rank, mpi_tag++, _to_enqueue);
    _communicator->send(_real_rank, mpi_tag++, _to_dequeue);

    boost::mpi::broadcast(*_communicator, _boxes, _real_rank);

    _to_enqueue.clear();
    _to_dequeue.clear();
}