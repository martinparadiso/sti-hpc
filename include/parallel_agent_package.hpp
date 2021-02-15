/// @brief Serialization of parallel agent
#pragma once

#include <vector>

#include <repast_hpc/AgentId.h>
#include <repast_hpc/SharedContext.h>
#include <repast_hpc/AgentRequest.h>

#include "agent_creator.hpp"

/// @brief Agent package, for serialization
struct parallel_agent_package {

    parallel_agent_package() = default;

    /// @brief Create a new package, passing id and the contagious agent data
    /// @param id The repast ID
    /// @param data The serialized data of the contagious agent
    parallel_agent_package(const repast::AgentId& id, sti::contagious_agent::serial_data&& data)
        : id { id.id() }
        , rank { id.startingRank() }
        , type { id.agentType() }
        , current_rank { id.currentRank() }
        , data { std::move(data) }
    {
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& id;
        ar& rank;
        ar& type;
        ar& current_rank;
        ar& data;
    }

    /// @brief Get the ID as a Repast ID
    /// @return The repast ID
    repast::AgentId get_id() const
    {
        return repast::AgentId{id, rank, type, current_rank};
    }

    int id {};
    int rank {};
    int type {};
    int current_rank {};

    // Use generic data structure
    sti::contagious_agent::serial_data data;

}; // class parallel_agent_package

////////////////////////////////////////////////////////////////////////////////
// Package receiver and provider
////////////////////////////////////////////////////////////////////////////////

class parallel_agent_provider {

public:
    parallel_agent_provider(repast::SharedContext<sti::parallel_agent>* agents)
        : _agents { agents }
    {
    }

    /// @brief Serialize an agent and add it to a vector
    void providePackage(sti::parallel_agent* agent, std::vector<parallel_agent_package>& out)
    {
        const auto id      = agent->getId();
        auto       package = parallel_agent_package { id, agent->serialize() };
        out.push_back(package);
    }

    /// @brief Serialize a group of agents and add it to a vector
    void provideContent(const repast::AgentRequest& req, std::vector<parallel_agent_package>& out)
    {
        for (const auto& id : req.requestedAgents()) {
            providePackage(_agents->getAgent(id), out);
        }
    }

private:
    repast::SharedContext<sti::parallel_agent>* _agents;

}; // class parallel_agent_provider

class parallel_agent_receiver {

public:
    parallel_agent_receiver(repast::SharedContext<sti::parallel_agent>* agents)
        : _agents { agents }
    {
    }

    /// @brief Create an agent from a package
    static sti::parallel_agent * createAgent(const parallel_agent_package& package) {
        auto new_agent = sti::make_agent(package.get_id(), package.data);
        return new_agent.release();
    }

    /// @brief Update a "borrowed" agent
    void updateAgent(const parallel_agent_package& package) {
        _agents->getAgent(package.get_id())->update(package.data);
    }

private:
    repast::SharedContext<sti::parallel_agent>* _agents;
}; // class parallel_agent_receiver
