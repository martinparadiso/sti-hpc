/// @file person.hpp
/// @brief Person with no logic or mobility, only transmission
#pragma once

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <cstdint>
#include <sstream>

#include "contagious_agent.hpp"
#include "infection_logic.hpp"
#include "infection_logic/infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "infection_logic/object_infection_cycle.hpp"

namespace sti {

/// @brief Generic object
class object_agent final : public contagious_agent {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Generic object flyweight/common attributes
    struct flyweight {
        const infection_factory* inf_factory;
    };

    using flyweight_ptr = const flyweight*;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a new object
    /// @param id The repast id associated with this agent
    /// @param fw Flyweight containing shared information
    /// @param oic The infection logic
    object_agent(const id_t& id, flyweight_ptr fw, const object_infection_cycle& oic)
        : contagious_agent { id }
        , _flyweight { fw }
        , _infection_logic { oic }
    {
    }

    /// @brief Create an object with no internal data
    /// @param id The agent id
    /// @param fw Object flyweight
    object_agent(const id_t& id, flyweight_ptr fw)
        : contagious_agent { id }
        , _flyweight { fw }
        , _infection_logic { fw->inf_factory->make_object_cycle() }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the agent state into a string using Boost.Serialization
    /// @return A string with the serialized data
    serial_data serialize() override
    {
        auto ss = std::stringstream {};
        { // Used to make sure the stream is flushed
            auto oa = boost::archive::text_oarchive { ss };
            oa << (*this);
        }
        return ss.str();
    }

    /// @brief Reconstruct the agent state from a string using Boost.Serialization
    /// @param data The serialized data
    void serialize(const id_t& id, const serial_data& data) override
    {
        contagious_agent::id(id);
        auto ss = std::stringstream {};
        ss << data;
        { // Used to make sure the stream is flushed
            auto ia = boost::archive::text_iarchive { ss };
            ia >> (*this);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of this agent
    /// @return The type of the agent
    type get_type() const override
    {
        return type::OBJECT;
    }

    /// @brief Perform the actions this agents is suppossed to
    void act() override
    {
        _infection_logic.tick();
    }

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const infection_cycle* get_infection_logic() const override
    {
        return &_infection_logic;
    }

    // TODO: Implement properly
    boost::json::object kill_and_collect() final;

private:
    friend class boost::serialization::access;

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _infection_logic;
    }

    flyweight_ptr          _flyweight;
    object_infection_cycle _infection_logic;
}; // class object_agent

} // namespace sti