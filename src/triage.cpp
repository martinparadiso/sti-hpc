#include "triage.hpp"

#include <boost/lexical_cast.hpp>
#include <memory>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/Properties.h>
#include <vector>

#include "coordinates.hpp"
#include "hospital_plan.hpp"
#include "queue_manager/real_queue_manager.hpp"
#include "queue_manager/proxy_queue_manager.hpp"
#include "queue_manager.hpp"

/// @brief Construct a triage
/// @param props Simulation properties
/// @param comm MPI Communicator
/// @param plan Hospital plan
sti::triage::triage(const properties_type& props,
                    communicator_ptr       comm,
                    const hospital_plan&   plan)
    : _queue_manager {
        [&]() -> queue_manager* {
            const auto manager_rank = boost::lexical_cast<int>(props.getProperty("triage.manager.rank"));
            if (manager_rank == comm->rank()) {
                const auto locations = [&]() {
                    auto ret = std::vector<coordinates<double>> {};
                    for (const auto& triage : plan.triages()) {
                        ret.push_back(triage.location.continuous());
                    }
                    return ret;
                }();
                return new real_queue_manager { comm, 4542, locations };
            }
            return new proxy_queue_manager { comm, 4542, manager_rank };
        }()
    }
{
}
/// @brief Enqueue an agent into the triage queue
/// @param id The agent id
void sti::triage::enqueue(const agent_id& id)
{
    _queue_manager->enqueue(id);
}

/// @brief Remove an agent into the triage queue
/// @param id The agent id
void sti::triage::dequeue(const agent_id& id)
{
    _queue_manager->dequeue(id);
}

/// @brief Check if an agent has a triage assigned
/// @param id The id of the agent to check
/// @return If the agent has a triage assigned, the triage location
boost::optional<sti::coordinates<double>> sti::triage::is_my_turn(const agent_id& id)
{
    return _queue_manager->is_my_turn(id);
}

/// @brief Synchronize the queues between the process
void sti::triage::sync()
{
    _queue_manager->sync();
}