/// @file person.hpp
/// @brief Person with no logic or mobility, only transmission
#pragma once

#include <cstdint>

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

    /// @brief Create an object from serialized data
    /// @param id The agent id
    /// @param fw Object flyweight
    /// @param data The serialized data
    /// @param inf An infection factory
    object_agent(const id_t&   id,
                 flyweight_ptr fw,
                 serial_data&  queue)
        : contagious_agent { id }
        , _flyweight { fw }
        , _infection_logic { fw->inf_factory->make_object_cycle() }
    {
        deserialize(queue);
    }

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the internal state of the infection
    /// @param queue The queue to store the data
    void serialize(serial_data& queue) const override
    {
        // No internal state, serialize only infect logic
        _infection_logic.serialize(queue);
    }

    /// @brief Deserialize the data and update the agent data
    /// @param queue The queue containing the data
    void deserialize(serial_data& queue) override
    {
        _infection_logic.deserialize(queue);
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

private:
    flyweight_ptr          _flyweight;
    object_infection_cycle _infection_logic;
}; // class object_agent

} // namespace sti