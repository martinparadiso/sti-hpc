/// @file queue_manager/real_queue_manager.hpp
/// @brief The real queue containing the data
#pragma once

#include <boost/mpi/communicator.hpp>
#include <list>
#include <map>
#include <optional>

#include "../queue_manager.hpp"
#include "../hospital_plan.hpp"

namespace sti {

/// @brief Real queue manager, containing the entire queue
class real_queue_manager final : public queue_manager {

public:
    using communicator_ptr = boost::mpi::communicator*;

    /// @brief Construct a real queue display
    /// @param comm The MPI Communicator
    /// @param tag The MPI tag for the communication
    /// @param positions The "boxes"/fronts of the queue
    real_queue_manager(communicator_ptr comm, int tag, const std::vector<coordinates<double>>& boxes);

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
    communicator_ptr                                       _communicator;
    int                                                    _tag;
    std::list<agent_id>                                    _queue;
    std::map<coordinates<double>, boost::optional<agent_id>> _boxes;
    // std::vector<coordinates<double>> _boxes;
};

} // namespace sti