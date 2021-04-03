/// @file person.hpp
/// @brief Person with no logic or mobility, only transmission
#pragma once

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <cstdint>
#include <string>
#include <utility>

#include "contagious_agent.hpp"
#include "infection_logic.hpp"
#include "infection_logic/human_infection_cycle.hpp"

namespace sti {

/// @brief An agent representing a person with no logic or mobility only tranmission
class person_agent final : public contagious_agent {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Person flyweight/common attributes
    struct flyweight {
        const infection_factory* inf_factory;
    };

    using flyweight_ptr = const flyweight*;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a new person
    /// @param fw The flyweight containing the shared attributes
    /// @param hic The infection logic
    person_agent(const id_t& id, const flyweight* fw, const human_infection_cycle& hic);

    /// @brief Create an empty person
    /// @param fw The agent flyweight
    person_agent(const id_t& id, const flyweight* fw);

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

    /// @brief Perform the actions this agent is supposed to
    void act() final;

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const infection_cycle* get_infection_logic() const override;

    /// @brief Return the agent statistics as a json object
    /// @return A Boost.JSON object containing relevant statistics
    boost::json::object stats() const override;

private:
    friend class boost::serialization::access;

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _infection_logic;
    }
    flyweight_ptr         _flyweight;
    human_infection_cycle _infection_logic;

}; // class person_agent

} // namespace sti