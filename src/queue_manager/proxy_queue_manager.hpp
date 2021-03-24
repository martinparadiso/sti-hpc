/// @file queue_manager/proxy_queue_manager.hpp
/// @brief A proxy queue, that communicates with the real queue every tick
#pragma once

#include <boost/mpi/communicator.hpp>
#include <queue>

#include "../queue_manager.hpp"

namespace sti {

/// @brief Real queue manager, containing the entire queue
class proxy_queue_manager final : public queue_manager {

public:
    using communicator_ptr = boost::mpi::communicator*;

    /// @brief Construct a real queue display
    /// @param comm The MPI Communicator
    /// @param tag The MPI tag
    /// @param positions The "boxes"/fronts of the queue
    proxy_queue_manager(communicator_ptr comm, int tag, int real_rank);

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Add a new patient to the queue
    /// @param id The agent to enqueue
    void enqueue(const agent_id& id) override;

    /// @brief Remove a patient from the queue
    /// @param id The id to remove from the queue
    void dequeue(const agent_id& id) override;

    /// @brief Check if the given agent is next in the attention
    /// @param id The agent id
    /// @return If the agent is in the front of the queue, the coordinates
    boost::optional<coordinates<double>> is_my_turn(const agent_id& id) override;

    /// @brief Synchronize the real queue and the remote queues
    void sync() override;

private:
    communicator_ptr _communicator;
    int              _tag;
    int              _real_rank;
    front_type       _front;

    std::vector<agent_id> _to_enqueue;
    std::vector<agent_id> _to_dequeue;
};

} // namespace sti