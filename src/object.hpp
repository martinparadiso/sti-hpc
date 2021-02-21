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

class object_agent final : public contagious_agent {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Person flyweight/common attributes
    struct flyweight {
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

    object_agent(const id_t& id,
    flyweight_ptr fw,
    const serial_data& data,
    const infection_factory* inf) 
    : contagious_agent{id}
    , _flyweight{fw}
    , _infection_logic{
        inf->make_object_cycle(id, {boost::get<std::uint32_t>(data.at(0))})
    } {

    }

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the agent data into a vector
    /// @return A vector with the agent data required for reconstruction
    serial_data serialize() const final {
        auto data = serial_data{};

        // No internal state, serialize only infection logic
        const auto ic = _infection_logic.serialize();
        data.push_back(ic.stage);

        return data;
    }

    /// @brief Update the agent state with new data
    /// @param id The agent id
    /// @param data The serial data
    void update(const id_t& id, const serial_data& data) final {
        const auto sp = object_infection_cycle::serialization_pack{
            boost::get<std::uint32_t>(data.at(0))
        };
        _infection_logic.update(id, sp);
    }

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of this agent
    /// @return The type of the agent
    type get_type() const final {
        return type::OBJECT;
    }

    /// @brief Perform the actions this agents is suppossed to
    void act() final {
        _infection_logic.tick();
    }

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const infection_cycle* get_infection_logic() const final {
        return &_infection_logic;
    }

private:
    flyweight_ptr          _flyweight;
    object_infection_cycle _infection_logic;
}; // class object_agent

} // namespace sti