/// @file patient.hpp
/// @brief Patient agent
#pragma once

#include <cstdint>

#include "contagious_agent.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"

namespace sti {

/// @brief An agent representing a patient
class patient_agent final : public contagious_agent {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Patient common properties
    struct flyweight {
        const infection_factory* inf_factory;
    };

    using flyweight_ptr = const flyweight*;

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
    patient_agent(const id_t&      id,
                  const flyweight* fw,
                  serial_data&     queue)
        : contagious_agent { id }
        , _flyweight { fw }
        , _entry_time {}
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
        queue.push(_entry_time.epoch());
        _infection_logic.serialize(queue);
    }

    /// @brief Deserialize the data and update the agent data
    /// @param queue The queue containing the data
    void deserialize(serial_data& queue) final
    {
        _entry_time = boost::get<clock::date_t::resolution>(queue.front());
        queue.pop();
        _infection_logic.deserialize(queue);
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
    flyweight_ptr         _flyweight;
    clock::date_t         _entry_time;
    human_infection_cycle _infection_logic;
}; // patient_agent

} // namespace sti
