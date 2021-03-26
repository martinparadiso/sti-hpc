#include "patient.hpp"

#include "coordinates.hpp"
#include "json_serialization.hpp"

#include <repast_hpc/Point.h>

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
    , _fsm{_flyweight, this}
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
    , _fsm{_flyweight, this}
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

// TODO: Implement properly
boost::json::object sti::patient_agent::kill_and_collect()
{
    auto output = boost::json::object {};

    output = {
        { "type", "patient" },
        { "entry_time", boost::json::value_from(_entry_time) },
        { "path", boost::json::value_from(_path) },
        { "infection_time", boost::json::value_from((*_infection_logic.get_infection_time())) }
    };

    const auto& stage_str = [&]() {
        const auto& stage = _infection_logic.get_stage();

        switch (stage) {
        case human_infection_cycle::STAGE::HEALTHY:
            return "healthy";
        case human_infection_cycle::STAGE::INCUBATING:
            return "incubating";
        case human_infection_cycle::STAGE::SICK:
            return "sick";
        }
    }();
    output["final_stage"] = stage_str;
    output["exit_time"]   = boost::json::value_from(_flyweight->clk->now());

    // output["chair_assigned"]       = boost::json::value_from(_chair_assigned);
    // output["reception_leave_time"] = boost::json::value_from(_reception_time);
    // output["reception_box"]        = boost::json::value_from(_reception_assigned);
    output["final_state"] = _fsm._last_state;

    // Remove from simulation
    _flyweight->space->remove_agent(this);
    _flyweight->context->removeAgent(this);

    return output;
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
    _infection_logic.tick();
    _fsm.tick();
}