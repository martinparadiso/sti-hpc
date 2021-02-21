/// @brief Contagious Agent
#pragma once

#include <memory>

#include <repast_hpc/AgentId.h>
#include <repast_hpc/SharedContext.h>
#include <repast_hpc/SharedDiscreteSpace.h>
#include <repast_hpc/VN2DGridQuery.h>
#include <vector>

#include "contagious_agent.hpp"

namespace sti {

/// @brief A class wrapping the logic agent capable of moving between processes
class parallel_agent {

public:
    using agent_type = contagious_agent;

    /// @brief Create a new parallel agent
    /// @param id The repast ID, must be created externally to avoid problems
    /// @param contagious_agent The contagious agent associated with this parallel agent
    parallel_agent(const repast::AgentId& id, std::unique_ptr<agent_type> contagious_agent)
        : _agent_id { id }
        , _contagious_agent { std::move(contagious_agent) }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // Required Repast methods
    ////////////////////////////////////////////////////////////////////////////

    repast::AgentId& getId() {
        return _agent_id;
    }

    const repast::AgentId& getId() const {
        return _agent_id;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Serialization
    ////////////////////////////////////////////////////////////////////////////

    auto serialize() const {
        return _contagious_agent->serialize();
    }
    
    void update(const contagious_agent::serial_data& data) {
        _contagious_agent->update(data);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Behaviour
    ////////////////////////////////////////////////////////////////////////////

    void act() {
        _contagious_agent->act();
    }

private:
    repast::AgentId             _agent_id;
    std::unique_ptr<agent_type> _contagious_agent;
}; // class parallel_agent

} // namespace sti
