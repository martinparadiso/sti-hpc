#include "reception.hpp"

#include <memory>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/Properties.h>
#include <boost/lexical_cast.hpp>

#include "hospital_plan.hpp"
#include "queue_manager/real_queue_manager.hpp"
#include "queue_manager/proxy_queue_manager.hpp"
#include "queue_manager.hpp"

/// @brief Construct a reception
/// @param props Simulation properties
/// @param comm MPI Communicator
/// @param plan Hospital plan
sti::reception::reception(const properties_type& props,
                          communicator_ptr       comm,
                          const hospital_plan&   plan)
    : _queue_manager {
        [&]() -> queue_manager* {
            const auto manager_rank = boost::lexical_cast<int>(props.getProperty("reception.manager.rank"));
            if (manager_rank == comm->rank()) {
                const auto locations = [&]() {
                    auto ret = std::vector<coordinates<double>> {};
                    for (const auto& reception : plan.receptionists()) {
                        ret.push_back(reception.patient_chair.continuous());
                    }
                    return ret;
                }();
                return new real_queue_manager { comm, 1324, locations };
            }
            return new proxy_queue_manager { comm, 1324, manager_rank };
        }()
    }
{
}
/// @brief Enqueue an agent into the reception queue
/// @param id The agent id
void sti::reception::enqueue(const agent_id& id)
{
    _queue_manager->enqueue(id);
}

/// @brief Remove an agent into the reception queue
/// @param id The agent id
void sti::reception::dequeue(const agent_id& id)
{
    _queue_manager->dequeue(id);
}

/// @brief Check if an agent has a reception assigned
/// @param id The id of the agent to check
/// @return If the agent has a reception assigned, the reception location
boost::optional<sti::coordinates<double>> sti::reception::is_my_turn(const agent_id& id)
{
    return _queue_manager->is_my_turn(id);
}

/// @brief Synchronize the queues between the process
void sti::reception::sync()
{
    _queue_manager->sync();
}