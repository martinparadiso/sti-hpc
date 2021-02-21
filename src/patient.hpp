/// @file patient.hpp
/// @brief Patient agent
#pragma once

#include "contagious_agent.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include <cstdint>

namespace sti {

/// @brief An agent representing a patient
class patient_agent final : public contagious_agent {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Patient common properties
    struct flyweight {
    };

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief A patient cannot be default constructed, unusable
    patient_agent() = delete;

    /// @brief Create a new patient
    /// @param id The agent id
    /// @param fw The flyweight containing the shared properties
    /// @param entry_time The instant the patient enter the building
    patient_agent(const id_t&                  id,
                  const flyweight*             fw,
                  const clock::date_t&         entry_time,
                  const human_infection_cycle& hic)
        : contagious_agent { id }
        , _flyweight { fw }
        , _entry_time { entry_time }
        , _infection_logic { hic }
    {
    }

    /// @brief Create a new patient from serialized data
    /// @param id The agent id
    /// @param fw The agent flyweight
    /// @param data The serialized data
    /// @param inf An infection factory
    patient_agent(const id_t&              id,
                  const flyweight*         fw,
                  const serial_data&       data,
                  const infection_factory* inf)
        : contagious_agent { id }
        , _flyweight { fw }
        , _entry_time { boost::get<clock::resolution>(data.at(0)) }
        , _infection_logic {
            inf->make_human_cycle(id, { boost::get<std::uint32_t>(data.at(1)), boost::get<clock::resolution>(data.at(2)) })
        }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the internal state of the agent
    /// @return The serialized data
    serial_data serialize() const override
    {
        const auto sp = _infection_logic.serialize();
        return { _entry_time.epoch(), sp.stage, sp.infection_time };
    }

    /// @brief Update the agent state
    /// @details Update the agent state with new data
    /// @param id The agent id associated with this
    /// @param data The new data for the agent
    /// @param inf An infection factory
    void update(const repast::AgentId& id,
                const serial_data&     data) final
    {
        contagious_agent::id(id);
        _entry_time   = boost::get<clock::date_t::resolution>(data.at(0));
        const auto sp = human_infection_cycle::serialization_pack {
            boost::get<std::uint32_t>(data.at(1)),
            boost::get<clock::date_t::resolution>(data.at(2))
        };
        _infection_logic.update(id, sp);
    }

    ////////////////////////////////////////////////////////////////////////////
    // BAHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of the agent
    /// @returns Patient
    type get_type() const override
    {
        return contagious_agent::type::PATIENT;
    }

    /// @brief Get the time the patient was admitted at the hospital
    /// @return The date (with a precission of seconds) the patient was admitted
    auto entry_time() const
    {
        return _entry_time;
    }

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const infection_cycle* get_infection_logic() const final
    {
        return &_infection_logic;
    }

    void act() override
    {
        // TODO
    }

private:
    const flyweight*      _flyweight;
    clock::date_t         _entry_time;
    human_infection_cycle _infection_logic;
}; // patient_agent

} // namespace sti
