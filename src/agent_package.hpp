/// @brief Serialization of parallel agent
#pragma once

#include <sstream>
#include <vector>

#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/queue.hpp>
#include <boost/serialization/variant.hpp>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/SharedContext.h>
#include <repast_hpc/AgentRequest.h>

#include "agent_factory.hpp"
#include "contagious_agent.hpp"
#include "infection_logic/infection_cycle.hpp"

namespace sti {

/// @brief Error trying to serialize or deserialize an agent
struct wrong_serialization : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Error serializing or deserializing an agent";
    }
};

}

/// @brief Agent package, for serialization
struct agent_package {

    agent_package() = default;

    /// @brief Create a new package, passing id and the contagious agent data
    /// @param id The repast ID
    /// @param data The serialized data of the contagious agent
    agent_package(const repast::AgentId& id, std::string&& data)
        : id { id }
        , data { std::move(data) }
    {
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& id;
        ar& data;
    }

    /// @brief Get the ID as a Repast ID
    /// @return The repast ID
    repast::AgentId get_id() const
    {
        return id;
    }

    repast::AgentId id {};

    // Use Boost.Serialization to serialize the contagious_agent into a string
    // using boost::archive::text_iarchive and boost::archive::text_oarchive
    std::string data;

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
        auto       data    = agent->serialize();
        auto       package = agent_package { id, std::move(data) };
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
    using context_ptr       = repast::SharedContext<sti::contagious_agent>*;
    using space_ptr         = repast::SharedDiscreteSpace<sti::contagious_agent, repast::StrictBorders, repast::SimpleAdder<sti::contagious_agent>>*;
    using agent_factory_ptr = sti::agent_factory*;

    /// @brief Create an agent receiver
    /// @param context The repast context
    /// @param space The repast discrete space
    /// @param agent_factory A agent_factory to create the patient type
    agent_receiver(context_ptr       context,
                   agent_factory_ptr agent_factory)
        : _context { context }
        , _agent_factory { agent_factory }
    {
    }

    /// @brief Create an agent from a package (deserializing the data)
    sti::contagious_agent* createAgent(const agent_package& package)
    {
        const auto id          = package.get_id();
        const auto agent_type  = sti::to_agent_enum(id.agentType());
        auto       string_data = package.data;

        if (agent_type == sti::contagious_agent::type::PATIENT) {
            return _agent_factory->recreate_patient(id, string_data);
        }
        if (agent_type == sti::contagious_agent::type::FIXED_PERSON) {
            return _agent_factory->recreate_person(id, string_data);
        }
        if (agent_type == sti::contagious_agent::type::OBJECT) {
            return _agent_factory->recreate_object(id, string_data);
        }
        throw sti::wrong_serialization {};
    }

    /// @brief Update a "borrowed" agent
    void updateAgent(const agent_package& pkg)
    {
        _context->getAgent(pkg.get_id())->serialize(pkg.get_id(), pkg.data);
    }

private:
    context_ptr       _context;
    agent_factory_ptr _agent_factory;

}; // class agent_receiver
