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
    using object_type = std::string;

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
    /// @param type The object type, i.e.
    /// @param fw Flyweight containing shared information
    /// @param oic The infection logic
    object_agent(const id_t&                   id,
                 const object_type&            type,
                 flyweight_ptr                 fw,
                 const object_infection_cycle& oic);

    /// @brief Create an object with no internal data
    /// @param id The agent id
    /// @param fw Object flyweight
    object_agent(const id_t& id, flyweight_ptr fw);

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the agent state into a string using Boost.Serialization
    /// @return A string with the serialized data
    serial_data serialize() override;

    /// @brief Reconstruct the agent state from a string using Boost.Serialization
    /// @param data The serialized data
    void serialize(const id_t& id, const serial_data& data) override;

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of this agent
    /// @return The type of the agent
    type get_type() const override;

    /// @brief Perform the actions this agents is suppossed to
    void act() override;

    /// @brief Get the infection logic
    /// @return A  pointer to the infection logic
    object_infection_cycle* get_infection_logic() override;

    /// @brief Get the infection logic
    /// @return A const pointer to the infection logic
    const object_infection_cycle* get_infection_logic() const override;

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    object_infection_cycle* get_object_infection_logic();

    /// @brief Return the agent statistics as a json object
    /// @return A Boost.JSON object containing relevant statistics
    boost::json::object stats() const override;
    
private:
    friend class boost::serialization::access;

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _object_type;
        ar& _infection_logic;
    }

    flyweight_ptr _flyweight;

    object_type            _object_type;
    object_infection_cycle _infection_logic;
}; // class object_agent

} // namespace sti