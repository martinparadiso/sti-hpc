/// @brief Serialization of parallel agent
#pragma once

#include <vector>

#include <repast_hpc/AgentId.h>
#include <repast_hpc/SharedContext.h>
#include <repast_hpc/AgentRequest.h>

#include "agent_creator.hpp"
#include "contagious_agent.hpp"

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
        return repast::AgentId { id, rank, type, current_rank };
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
    using context_ptr         = repast::SharedContext<sti::parallel_agent>*;
    using space_ptr           = repast::SharedDiscreteSpace<sti::parallel_agent, repast::StrictBorders, repast::SimpleAdder<sti::parallel_agent>>*;
    using patient_factory_ptr = sti::patient_factory*;

    /// @brief Create an agent receiver
    /// @param context The repast context
    /// @param space The repast discrete space
    /// @param patient_factory A patient_factory to create the patient type
    parallel_agent_receiver(context_ptr         context,
                            patient_factory_ptr patient_factory)
        : _context { context }
        , _patient_factory { patient_factory }
    {
    }

    /// @brief Create an agent from a package (deserialize)
    sti::parallel_agent* createAgent(const parallel_agent_package& package)
    {
        const auto  id         = package.get_id();
        const auto  agent_type = sti::to_agent_enum(id.agentType());
        const auto& data       = package.data;

        using E = decltype(agent_type);
        switch (agent_type) {
        case E::PATIENT:
            return _patient_factory->deserialize(id, data);
        default:
            throw std::exception();
        }
        // auto new_agent = sti::make_agent(package.get_id(), package.data);
        // return new_agent.release();
    }

    /// @brief Update a "borrowed" agent
    void updateAgent(const parallel_agent_package& package)
    {
        _context->getAgent(package.get_id())->update(package.data);
    }

private:
    context_ptr         _context;
    patient_factory_ptr _patient_factory;

}; // class parallel_agent_receiver

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