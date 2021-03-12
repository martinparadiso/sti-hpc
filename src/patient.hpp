/// @file patient.hpp
/// @brief Patient agent
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <math.h>
#include <memory>
#include <repast_hpc/Grid.h>
#include <repast_hpc/SharedContext.h>
#include <sstream>
#include <vector>

#include "chair_manager.hpp"
#include "clock.hpp"
#include "contagious_agent.hpp"
#include "hospital_plan.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "print.hpp"
#include "space_wrapper.hpp"

namespace sti {

/// @brief An agent representing a patient
class patient_agent final : public contagious_agent {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Patient common properties
    struct flyweight {
        const sti::infection_factory*            inf_factory;
        sti::chair_manager*                      chairs;
        sti::hospital_plan*                      hospital;
        sti::clock*                              clk;
        repast::SharedContext<contagious_agent>* context;
        sti::space_wrapper*                      space;
        const double                             walk_speed;
    };

    using flyweight_ptr = flyweight*;

    ////////////////////////////////////////////////////////////////////////////
    // PIMPL
    ////////////////////////////////////////////////////////////////////////////

    class pimpl;

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
                  flyweight_ptr                fw,
                  const datetime&              entry_time,
                  const human_infection_cycle& hic);

    /// @brief Create a new patient from serialized data
    /// @param id The agent id
    /// @param fw The agent flyweight
    /// @param data The serialized data
    patient_agent(const id_t&   id,
                  flyweight_ptr fw,
                  serial_data&  queue);

    ~patient_agent();

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the internal state of the infection
    /// @param queue The queue to store the data
    void serialize(serial_data& queue) const final;

    /// @brief Deserialize the data and update the agent data
    /// @param queue The queue containing the data
    void deserialize(serial_data& queue) final;

    ////////////////////////////////////////////////////////////////////////////
    // BAHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of the agent
    /// @returns Patient
    type get_type() const final;

    // TODO: Implement properly
    boost::json::object kill_and_collect() final;

    /// @brief Get the time the patient was admitted at the hospital
    /// @return The date (with a precission of seconds) the patient was admitted
    datetime entry_time() const;

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const infection_cycle* get_infection_logic() const final;

    void act() final;

private:
    flyweight_ptr         _flyweight;
    datetime              _entry_time;
    human_infection_cycle _infection_logic;

    std::unique_ptr<pimpl> _pimpl;

    // TODO: Remove this, temp
    enum class STAGES {
        START,
        WAITING_CHAIR,
        WALKING_TO_CHAIR,
        SIT_DOWN,
        WALKING_TO_EXIT,
        DULL,
    };
    STAGES      _stage          = STAGES::START;
    coordinates _chair_assigned = { -1, -1 };
    datetime    _chair_release_time;
}; // patient_agent

} // namespace sti
