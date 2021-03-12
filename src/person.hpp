/// @file person.hpp
/// @brief Person with no logic or mobility, only transmission
#pragma once

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
    person_agent(const id_t& id, const flyweight* fw, const human_infection_cycle& hic)
        : contagious_agent { id }
        , _flyweight { fw }
        , _infection_logic { hic }
    {
    }

    /// @brief Create a new person from serialized data
    /// @param id The agent id
    /// @param fw The agent flyweight
    /// @param data The serialized data
    /// @param inf An infection factory
    person_agent(const id_t&      id,
                 const flyweight* fw,
                 serial_data&     queue)
        : contagious_agent { id }
        , _flyweight { fw }
        , _infection_logic { fw->inf_factory->make_human_cycle() }
    {
        deserialize(queue);
    }

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the internal state of the infection
    /// @param queue The queue to store the data
    void serialize(serial_data& queue) const final
    {
        _infection_logic.serialize(queue);
    }

    /// @brief Deserialize the data and update the agent data
    /// @param queue The queue containing the data
    void deserialize(serial_data& queue) final
    {
        _infection_logic.deserialize(queue);
    }

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of this agent
    /// @return The type of the agent
    type get_type() const final
    {
        return type::FIXED_PERSON;
    }

    /// @brief Perform the actions this agent is supposed to
    void act() final
    {
        _infection_logic.tick();
    }

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const infection_cycle* get_infection_logic() const final
    {
        return &_infection_logic;
    }

    // TODO: Implement properly
    boost::json::object kill_and_collect() override;

private:
    flyweight_ptr         _flyweight;
    human_infection_cycle _infection_logic;

}; // class person_agent

} // namespace sti