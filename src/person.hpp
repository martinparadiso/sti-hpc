/// @file person.hpp
/// @brief Person with no logic or mobility, only transmission
#pragma once

#include <cstdint>

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
    };

    using flyweight_ptr = const flyweight*;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a new person
    /// @param fw The flyweight containing the shared attributes
    /// @param hic The infection logic
    person_agent(const id_t& id, const flyweight* fw, const human_infection_cycle& hic)
        : contagious_agent{id}
        , _flyweight { fw }
        , _infection_logic { hic }
    {
    }
    
    /// @brief Create a new person from serialized data
    /// @param id The agent id
    /// @param fw The agent flyweight
    /// @param data The serialized data
    /// @param inf An infection factory
    person_agent(const id_t&              id,
                  const flyweight*         fw,
                  const serial_data&       data,
                  const infection_factory* inf)
        : contagious_agent { id }
        , _flyweight { fw }
        , _infection_logic {
            inf->make_human_cycle(id, { boost::get<std::uint32_t>(data.at(0)), boost::get<clock::resolution>(data.at(1)) })
        }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the agent data into a vector
    /// @return A vector with the agent data required for reconstruction
    serial_data serialize() const final
    {
        auto data = serial_data{};

        // Has no internal state, serialize only the infection logic
        const auto ic = _infection_logic.serialize();
        data.push_back(ic.stage);
        data.push_back(ic.infection_time);

        return data;
    }

    /// @brief Update the agent state
    /// @details Update the agent state with new data
    /// @param id The agent id associated with this logic
    /// @param data The new data for the agent
    void update(const id_t& id, const serial_data& data) final
    {
        const auto sp = human_infection_cycle::serialization_pack {
            boost::get<std::uint32_t>(data.at(0)),
            boost::get<clock::date_t::resolution>(data.at(1))
        };
        _infection_logic.update(id, sp);
    }

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of this agent
    /// @return The type of the agent
    type get_type() const final {
        return type::FIXED_PERSON;
    }

    /// @brief Perform the actions this agent is supposed to
    void act() final
    {
        _infection_logic.tick();
    }

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const infection_cycle* get_infection_logic() const final {
        return &_infection_logic;
    }

private:
    flyweight_ptr         _flyweight;
    human_infection_cycle _infection_logic;

}; // class person_agent

} // namespace sti