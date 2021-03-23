#include "patient.hpp"

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
    output["exit_time"] = boost::json::value_from(_flyweight->clk->now());

    output["chair_assigned"]     = boost::json::value_from(_chair_assigned);
    output["reception_leave_time"] = boost::json::value_from(_reception_time);
    output["reception_box"] = boost::json::value_from(_reception_assigned);

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

void sti::patient_agent::act()
{
    _infection_logic.tick();

    // The patient 0 must request a chair, and walk to it
    if (getId().id() != 0) {

        const auto get_my_loc = [&]() {
            const auto repast_coord = _flyweight->space->get_discrete_location(getId());
            return coordinates {
                repast_coord.getX(),
                repast_coord.getY()
            };
        };

        if (_stage == STAGES::START) {
            // Request a chair
            _flyweight->chairs->request_chair(getId());
            _stage = STAGES::WAITING_CHAIR;
            return;
        }

        if (_stage == STAGES::WAITING_CHAIR) {

            // Check if the request has been satisfied
            const auto response = _flyweight->chairs->get_response(getId());
            if (response) {
                if (response->chair_location) {

                    _chair_assigned = response->chair_location.get();
                    _stage          = STAGES::WALKING_TO_CHAIR;
                } else {
                    _stage = STAGES::WALKING_TO_EXIT;
                }
            }
            return;
        }

        if (_stage == STAGES::WALKING_TO_CHAIR) {

            if (get_my_loc() == _chair_assigned) {
                _flyweight->reception->enqueue(getId());
                _stage = STAGES::WAIT_FOR_RECEPTION;
            } else {
                const auto final_pos = _flyweight->space->move_towards(getId(),
                                                                       _chair_assigned,
                                                                       _flyweight->walk_speed);
                _path.push_back(final_pos);
            }

            return;
        }

        if (_stage == STAGES::WAIT_FOR_RECEPTION) {
            // Wait a couple of ticks
            if (_flyweight->reception->is_my_turn(getId())) {
                _flyweight->chairs->release_chair(_chair_assigned);
                _reception_assigned = _flyweight->reception->is_my_turn(getId()).get();
                _stage              = STAGES::WALKING_TO_RECEPTION;
            }
            return;
        }

        if (_stage == STAGES::WALKING_TO_RECEPTION) {

            if (get_my_loc() == _reception_assigned) {
                _reception_time = _flyweight->clk->now() + timedelta { 0, 0, 15, 0 };
                _stage          = STAGES::WAIT_IN_RECEPTION;
            } else {
                const auto final_pos = _flyweight->space->move_towards(getId(),
                                                                       _reception_assigned,
                                                                       _flyweight->walk_speed);
                _path.push_back(final_pos);
            }

            return;
        };

        if (_stage == STAGES::WAIT_IN_RECEPTION) {
            if ( _reception_time < _flyweight->clk->now()) {
                _flyweight->reception->dequeue(getId());
                _stage = STAGES::WALKING_TO_EXIT;
            }
        }

        if (_stage == STAGES::WALKING_TO_EXIT) {
            const auto& exit_loc = _flyweight->hospital->exit().location;

            const auto final_pos = _flyweight->space->move_towards(getId(),
                                                                   exit_loc,
                                                                   _flyweight->walk_speed);
            _path.push_back(final_pos);
            return;
        };
    }
}