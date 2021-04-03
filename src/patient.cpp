#include "patient.hpp"

#include <repast_hpc/Point.h>

#include "chair_manager.hpp"
#include "clock.hpp"
#include "hospital_plan.hpp"
#include "json_serialization.hpp"
#include "reception.hpp"
#include "space_wrapper.hpp"
#include "triage.hpp"

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Create a new patient
/// @param id The agent id
/// @param fw The flyweight containing the shared properties
/// @param entry_time The instant the patient enter the building
sti::patient_agent::patient_agent(const id_t&                  id,
                                  flyweight_ptr                fw,
                                  const datetime&              entry_time,
                                  const human_infection_cycle& hic)
    : contagious_agent { id }
    , _flyweight { fw }
    , _entry_time { entry_time }
    , _infection_logic { hic }
    , _fsm { _flyweight, this }
{
}

/// @brief Create a new patient from serialized data
/// @param id The agent id
/// @param fw The agent flyweight
/// @param data The serialized data
sti::patient_agent::patient_agent(const id_t& id, flyweight_ptr fw)
    : contagious_agent { id }
    , _flyweight { fw }
    , _entry_time {}
    , _infection_logic { fw->inf_factory->make_human_cycle() }
    , _fsm { _flyweight, this }
{
}

sti::patient_agent::~patient_agent() = default;

////////////////////////////////////////////////////////////////////////////
// BAHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Get the type of the agent
/// @returns Patient
sti::patient_agent::type sti::patient_agent::get_type() const
{
    return contagious_agent::type::PATIENT;
}

/// @brief Return the agent statistics as a json object
/// @return A Boost.JSON object containing relevant statistics
boost::json::object sti::patient_agent::stats() const
{

    return {
        { "id", to_string(getId()) },
        { "type", "patient" },
        { "entry_time", _entry_time.epoch() },
        { "path", _path },
        { "infection", _infection_logic.stats() },
        { "exit_time", _flyweight->clk->now().epoch() },
        { "last_state", _fsm._last_state }
    };
}

/// @brief Get the time the patient was admitted at the hospital
/// @return The date (with a precission of seconds) the patient was admitted
sti::datetime sti::patient_agent::entry_time() const
{
    return _entry_time;
}

/// @brief Get the infection logic
/// @return A pointer to the infection logic
const sti::infection_cycle* sti::patient_agent::get_infection_logic() const
{
    return &_infection_logic;
}

/// @brief Execute the patient logic, both infection and behaviour
void sti::patient_agent::act()
{
    _infection_logic.infect_with_environment();
    _infection_logic.infect_with_nearby();
    _infection_logic.tick();
    _fsm.tick();
}