/// @brief Serialization of parallel agent
#pragma once

#include <vector>

#include <repast_hpc/AgentId.h>
#include <repast_hpc/SharedContext.h>
#include <repast_hpc/AgentRequest.h>

#include "agent_creator.hpp"
#include "contagious_agent.hpp"

/// @brief Agent package, for serialization
struct agent_package {

    agent_package() = default;

    /// @brief Create a new package, passing id and the contagious agent data
    /// @param id The repast ID
    /// @param data The serialized data of the contagious agent
    agent_package(const repast::AgentId& id, sti::contagious_agent::serial_data&& data)
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
        return repast::AgentId { id, rank, type, current_rank };
    }

    int id {};
    int rank {};
    int type {};
    int current_rank {};

    // Use generic data structure
    sti::contagious_agent::serial_data data;

}; // class agent_package

////////////////////////////////////////////////////////////////////////////////
// Package receiver and provider
////////////////////////////////////////////////////////////////////////////////

class agent_provider {

public:
    agent_provider(repast::SharedContext<sti::contagious_agent>* agents)
        : _agents { agents }
    {
    }

    /// @brief Serialize an agent and add it to a vector
    static void providePackage(sti::contagious_agent* agent, std::vector<agent_package>& out)
    {
        const auto id      = agent->getId();
        auto       package = agent_package { id, agent->serialize() };
        out.push_back(package);
    }

    /// @brief Serialize a group of agents and add it to a vector
    void provideContent(const repast::AgentRequest& req, std::vector<agent_package>& out)
    {
        for (const auto& id : req.requestedAgents()) {
            providePackage(_agents->getAgent(id), out);
        }
    }

private:
    repast::SharedContext<sti::contagious_agent>* _agents;

}; // class agent_provider

class agent_receiver {

public:
    using context_ptr         = repast::SharedContext<sti::contagious_agent>*;
    using space_ptr           = repast::SharedDiscreteSpace<sti::contagious_agent, repast::StrictBorders, repast::SimpleAdder<sti::contagious_agent>>*;
    using agent_factory_ptr = sti::agent_factory*;

    /// @brief Create an agent receiver
    /// @param context The repast context
    /// @param space The repast discrete space
    /// @param agent_factory A agent_factory to create the patient type
    agent_receiver(context_ptr         context,
                   agent_factory_ptr agent_factory)
        : _context { context }
        , _agent_factory { agent_factory }
    {
    }

    /// @brief Create an agent from a package (deserializing the data)
    sti::contagious_agent* createAgent(const agent_package& package)
    {
        const auto  id         = package.get_id();
        const auto  agent_type = sti::to_agent_enum(id.agentType());
        const auto& data       = package.data;

        switch (agent_type) {
        case sti::contagious_agent::type::PATIENT:
            return _agent_factory->recreate_patient(id, data);
        case sti::contagious_agent::type::FIXED_PERSON:
            return _agent_factory->recreate_person(id, data);
        case sti::contagious_agent::type::OBJECT:
            return _agent_factory->recreate_object(id, data);
        }
    }

    /// @brief Update a "borrowed" agent
    void updateAgent(const agent_package& package)
    {
        _context->getAgent(package.get_id())->update(package.get_id(), package.data);
    }

private:
    context_ptr         _context;
    agent_factory_ptr _agent_factory;

}; // class agent_receiver

// TODO: Remove this
/// @brief Create a new agent
/// @param id The id of the agent
/// @param data The data making up the agent
// inline std::unique_ptr<parallel_agent> make_agent(const repast::AgentId& id, const contagious_agent::serial_data& data)
// {

//     const auto agent_type = to_agent_enum(id.agentType());

//     using E = decltype(agent_type);

//     switch (agent_type) {
//     case E::PATIENT:
//         return std::make_unique<parallel_agent>(id, std::make_unique<patient_agent>(data));
//         break;
//     default:
//         throw std::exception();
//     }
// }