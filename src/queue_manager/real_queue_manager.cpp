#include <algorithm>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/map.hpp>
#include <repast_hpc/AgentId.h>
#include <vector>

#include "real_queue_manager.hpp"

/// @brief Construct a real queue display
/// @param comm The MPI Communicator
/// @param tag The MPI tag for the communication
/// @param positions The "boxes"/fronts of the queue
sti::real_queue_manager::real_queue_manager(communicator_ptr                        comm,
                                            int                                     tag,
                                            const std::vector<coordinates<double>>& boxes)
    : _communicator { comm }
    , _tag { tag }
    , _boxes { [&]() {
        auto tmp_boxes = decltype(_boxes) {};
        for (const auto& box : boxes) {
            tmp_boxes[box] = boost::none;
        }
        return tmp_boxes;
    }() }
{
}

////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Add a new patient to the queue
/// @param id The agent to enqueue
void sti::real_queue_manager::enqueue(const agent_id& id)
{
    _queue.push_back(id);
}

/// @brief Remove a patient from the queue
/// @param id The id to remove from the queue
void sti::real_queue_manager::dequeue(const agent_id& id)
{
    // Search  for the patient in the assigned boxes
    auto box_it = std::find_if(_boxes.begin(), _boxes.end(), [&](const auto& pair) {
        return pair.second == id;
    });

    if (box_it != _boxes.end()) {
        // If the patient is in a box, remove it
        box_it->second = {};
    } else {
        // The patient is not in the boxes, is in the queue
        _queue.remove(id);
    }
}

/// @brief Check if the given agent is next in the attention
/// @param id The agent id
/// @return If the agent is in the front of the queue, the coordinates
boost::optional<sti::coordinates<double>> sti::real_queue_manager::is_my_turn(const agent_id& id)
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
void sti::real_queue_manager::sync()
{
    const auto& my_rank    = _communicator->rank();
    const auto& world_size = _communicator->size();
    auto        mpi_tag    = _tag;

    // Receive the new agents
    auto to_enqueue = std::vector {
        static_cast<std::size_t>(world_size),
        std::vector<agent_id> {}
    };

    for (auto p = 0; p < world_size; ++p) {
        if (p != my_rank) {
            _communicator->recv(p, mpi_tag, to_enqueue[static_cast<unsigned long>(p)]);
        }
    }
    mpi_tag++;

    // Receive the agent to remove
    auto to_dequeue = std::vector {
        static_cast<std::size_t>(world_size),
        std::vector<agent_id> {}
    };

    for (auto p = 0; p < world_size; ++p) {
        if (p != my_rank) {
            _communicator->recv(p, mpi_tag, to_dequeue[static_cast<unsigned long>(p)]);
        }
    }
    mpi_tag++;

    // First insert the new ones, then remove
    for (const auto& vector : to_enqueue) {
        for (const auto& new_agent : vector) {
            this->enqueue(new_agent);
        }
    }

    for (const auto& vector : to_dequeue) {
        for (const auto& agent : vector) {
            this->dequeue(agent);
        }
    }

    // Update the front
    for (auto& [box, patient] : _boxes) {
        if (!patient.is_initialized() && !_queue.empty()) {
            patient = _queue.front();
            _queue.pop_front();
        }
    }

    // Broadcast the new front
    boost::mpi::broadcast(*_communicator, _boxes, _communicator->rank());
}
