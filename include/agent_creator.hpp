/// @brief Agent creation
#pragma once 

#include <exception>
#include <memory>

#include <repast_hpc/AgentId.h>

#include "contagious_agent.hpp"
#include "parallel_agent.hpp"

namespace sti {

    /// @brief Create a new agent 
    /// @param id The id of the agent
    /// @param data The data making up the agent
    inline std::unique_ptr<parallel_agent> make_agent(const repast::AgentId& id, const contagious_agent::serial_data& data) {
        
        const auto agent_type = to_agent_enum(id.agentType());
        
        using E = decltype(agent_type);

        switch (agent_type) {
            case E::PATIENT:
                return std::make_unique<parallel_agent>(id, std::make_unique<patient_agent>(data));
                break;
            default:
                throw std::exception();
        }
    }

} // namespace sti
