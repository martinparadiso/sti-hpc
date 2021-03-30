#include "patient_fsm.hpp"

#include <iostream>
#include <repast_hpc/AgentId.h>

#include "chair_manager.hpp"
#include "clock.hpp"
#include "coordinates.hpp"
#include "doctors_queue.hpp"
#include "hospital_plan.hpp"
#include "json_serialization.hpp"
#include "patient.hpp"
#include "reception.hpp"
#include "space_wrapper.hpp"
#include "triage.hpp"
#include "debug_flags.hpp"

namespace {

std::string state_2_string(sti::patient_fsm::STATE state)
{

    using STATE = sti::patient_fsm::STATE;

    switch (state) {
    case STATE::ENTRY:
        return "ENTRY";
    case STATE::WAIT_CHAIR_1:
        return "WAIT_CHAIR_1";
    case STATE::WALK_TO_CHAIR_1:
        return "WALK_TO_CHAIR_1";
    case STATE::WAIT_RECEPTION_TURN:
        return "WAIT_RECEPTION_TURN";
    case STATE::WALK_TO_RECEPTION:
        return "WALK_TO_RECEPTION";
    case STATE::WAIT_IN_RECEPTION:
        return "WAIT_IN_RECEPTION";
    case STATE::WAIT_CHAIR_2:
        return "WAIT_CHAIR_2";
    case STATE::WALK_TO_CHAIR_2:
        return "WALK_TO_CHAIR_2";
    case STATE::WAIT_TRIAGE_TURN:
        return "WAIT_TRIAGE_TURN";
    case STATE::WALK_TO_TRIAGE:
        return "WALK_TO_TRIAGE";
    case STATE::WAIT_IN_TRIAGE:
        return "WAIT_IN_TRIAGE";
    case STATE::DISPATCH:
        return "DISPATCH";
    case STATE::WAIT_CHAIR_3:
        return "WAIT_CHAIR_3";
    case STATE::WALK_TO_CHAIR_3:
        return "WALK_TO_CHAIR_3";
    case STATE::WAIT_FOR_DOCTOR:
        return "WAIT_FOR_DOCTOR";
    case STATE::WALK_TO_DOCTOR:
        return "WALK_TO_DOCTOR";
    case STATE::WAIT_IN_DOCTOR:
        return "WAIT_IN_DOCTOR";
    case STATE::NO_ATTENTION:
        return "NO_ATTENTION";
    case STATE::WAIT_IN_ICU:
        return "WAIT_IN_ICU";
    case STATE::WALK_TO_ICU:
        return "WALK_TO_ICU";
    case STATE::SLEEP:
        return "SLEEP";
    case STATE::MORGUE:
        return "MORGUE";
    case STATE::WALK_TO_EXIT:
        return "WALK_TO_EXIT";
    case STATE::AWAITING_DELETION:
        return "AWAITING_DELETION";
    }
}

sti::patient_fsm::transition_table create_transition_table()
{
    auto table  = sti::patient_fsm::transition_table {};
    using fsm   = sti::patient_fsm;
    using STATE = sti::patient_fsm::STATE;

    ////////////////////////////////////////////////////////////////////////////
    // AUXILIARY FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto always_true = [](fsm& /*unused*/) { return true; };
    auto empty       = [](fsm& /*unused*/) {};

    ////////////////////////////////////////////////////////////////////////////
    // CHAIR FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto request_chair = [](fsm& m) {
        m._flyweight->chairs->request_chair(m._patient->getId());
    };

    auto got_chair = [](fsm& m) {
        auto response_peek = m._flyweight->chairs->peek_response(m._patient->getId());
        if (response_peek.is_initialized()) {
            if (response_peek->chair_location.is_initialized()) {
                return true;
            }
        }
        return false;
    };

    auto set_destination_chair = [](fsm& m) {
        auto response  = m._flyweight->chairs->get_response(m._patient->getId());
        m._destination = response->chair_location.get();
    };

    auto no_chair_available = [](fsm& m) {
        auto response = m._flyweight->chairs->peek_response(m._patient->getId());
        if (response.is_initialized()) {
            if (!response->chair_location.is_initialized()) {
                // Remove the response from the queue
                m._flyweight->chairs->get_response(m._patient->getId());
                return true;
            }
        }
        return false;
    };

    ////////////////////////////////////////////////////////////////////////////
    // TIME WAIT FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto time_elapsed = [](fsm& m) {
        return m._attention_end < m._flyweight->clk->now();
    };

    ////////////////////////////////////////////////////////////////////////////
    // WALK AND MOVEMENT FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto arrived = [](fsm& m) {
        const auto patient_location = sti::coordinates<double> {
            m._flyweight->space->get_continuous_location(m._patient->getId())
        };
        return m._destination == patient_location;
    };

    auto not_arrived = [](fsm& m) {
        const auto patient_location = sti::coordinates<double> {
            m._flyweight->space->get_continuous_location(m._patient->getId())
        };
        return m._destination != patient_location;
    };

    auto walk = [](fsm& m) {
        m._flyweight->space->move_towards(
            m._patient->getId(),
            m._destination,
            m._flyweight->walk_speed);
    };

    ////////////////////////////////////////////////////////////////////////////
    // RECEPTION FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto enqueue_in_reception = [](fsm& m) {
        m._flyweight->reception->enqueue(m._patient->getId());
    };

    auto reception_turn = [](fsm& m) -> bool {
        return m._flyweight->reception->is_my_turn(m._patient->getId()).has_value();
    };

    auto set_destination_reception = [](fsm& m) {
        m._destination = m._flyweight->reception->is_my_turn(m._patient->getId()).get();
    };

    auto set_reception_time = [](fsm& m) {
        m._attention_end = m._flyweight->clk->now() + m._flyweight->reception_time;
    };

    ////////////////////////////////////////////////////////////////////////////
    // TRIAGE FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto enqueue_in_triage = [](fsm& m) {
        m._flyweight->triage->enqueue(m._patient->getId());
    };

    auto triage_turn = [](fsm& m) -> bool {
        return m._flyweight->triage->is_my_turn(m._patient->getId()).has_value();
    };

    auto set_destination_triage = [](fsm& m) {
        m._destination = m._flyweight->triage->is_my_turn(m._patient->getId()).get();
    };

    auto set_triage_time = [](fsm& m) {
        m._attention_end = m._flyweight->clk->now() + m._flyweight->triage_duration;
    };

    auto get_diagnosis = [](fsm& m) {
        m._diagnosis = m._flyweight->triage->diagnose();
    };

    auto to_doctor = [](fsm& m) {
        return m._diagnosis.area_assigned == sti::triage::triage_diagnosis::DIAGNOSTIC::DOCTOR;
    };

    auto to_icu = [](fsm& m) {
        return m._diagnosis.area_assigned == sti::triage::triage_diagnosis::DIAGNOSTIC::ICU;
    };

    ////////////////////////////////////////////////////////////////////////////
    // DOCTOR FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto enqueue_in_doctor = [](fsm& m) {
        m._flyweight->doctors->queues()->enqueue(m._diagnosis.doctor_assigned,
                                                 m._patient->getId(),
                                                 m._diagnosis.attention_time_limit);
    };

    auto doctor_turn = [](fsm& m) {
        const auto response = m._flyweight->doctors->queues()->is_my_turn(m._diagnosis.doctor_assigned,
                                                                          m._patient->getId());
        return response.is_initialized();
    };

    auto set_doctor_destination = [](fsm& m) {
        const auto response = m._flyweight->doctors->queues()->is_my_turn(m._diagnosis.doctor_assigned,
                                                                          m._patient->getId());
        m._destination      = response.get();
    };

    auto doctor_timeout = [](fsm& m) {
        return m._diagnosis.attention_time_limit < m._flyweight->clk->now();
    };

    auto set_doctor_time = [](fsm& m) {
        m._attention_end = m._flyweight->clk->now() + m._flyweight->doctors->get_attention_duration(m._diagnosis.doctor_assigned);
    };

    ////////////////////////////////////////////////////////////////////////////
    // EXIT TRANSITION
    ////////////////////////////////////////////////////////////////////////////

    auto set_exit_motive_and_destination = [](fsm& m) {
        m._destination = m._flyweight->hospital->exit().location.continuous();
        m._last_state  = state_2_string(m._current_state);
    };

    ////////////////////////////////////////////////////////////////////////////
    // TRANSITION TABLE
    ////////////////////////////////////////////////////////////////////////////
    // clang-format off
    
    table[STATE::ENTRY] = {
    //    GUARD           ACTION          DESTINATION
    //  +---------------+---------------+-----------------------+
        { always_true   , request_chair , STATE::WAIT_CHAIR_1   },
    };

    table[STATE::WAIT_CHAIR_1] = {
    //    GUARD                   ACTION                              DESTINATION
    //  +-----------------------+-----------------------------------+---------------------------+
        { no_chair_available    , set_exit_motive_and_destination   , STATE::WALK_TO_EXIT       },
        { got_chair             , set_destination_chair             , STATE::WALK_TO_CHAIR_1    },
    };

    table[STATE::WALK_TO_CHAIR_1] = {
    //    GUARD           ACTION                  DESTINATION
    //  +---------------+-----------------------+-------------------------------+
        { not_arrived   , walk                  , STATE::WALK_TO_CHAIR_1        },
        { arrived       , enqueue_in_reception  , STATE::WAIT_RECEPTION_TURN    },
    };

    table[STATE::WAIT_RECEPTION_TURN] = {
    //    GUARD                   ACTION                      DESTINATION
    //  +-----------------------+---------------------------+-------------------------------+
        { reception_turn        , set_destination_reception , STATE::WALK_TO_RECEPTION      },
    };

    table[STATE::WALK_TO_RECEPTION] = {
    //    GUARD           ACTION                  DESTINATION
    //  +---------------+-----------------------+-------------------------------+
        { not_arrived   , walk                  , STATE::WALK_TO_RECEPTION      },
        { arrived       , set_reception_time    , STATE::WAIT_IN_RECEPTION      },
    }; 

    table[STATE::WAIT_IN_RECEPTION] = {
    //    GUARD               ACTION          DESTINATION
    //  +-------------------+---------------+---------------------------+
        { time_elapsed      , request_chair , STATE::WAIT_CHAIR_2       },
    };

    table[STATE::WAIT_CHAIR_2] = {
    //    GUARD                   ACTION                              DESTINATION
    //  +-----------------------+-----------------------------------+---------------------------+
        { no_chair_available    , set_exit_motive_and_destination   , STATE::WALK_TO_EXIT       },
        { got_chair             , set_destination_chair             , STATE::WALK_TO_CHAIR_2    },
    };

    table[STATE::WALK_TO_CHAIR_2] = {
    //    GUARD           ACTION             DESTINATION
    //  +---------------+-------------------+---------------------------+
        { not_arrived   , walk              , STATE::WALK_TO_CHAIR_2    },
        { arrived       , enqueue_in_triage , STATE::WAIT_TRIAGE_TURN   },
    };

    table[STATE::WAIT_TRIAGE_TURN] = {
    //    GUARD               ACTION                      DESTINATION
    //  +-------------------+---------------------------+---------------------------+
        { triage_turn       , set_destination_triage    , STATE::WALK_TO_TRIAGE     },
    };

    table[STATE::WALK_TO_TRIAGE] = {
    //    GUARD           ACTION                  DESTINATION
    //  +---------------+-----------------------+-----------------------+
        { not_arrived   , walk                  , STATE::WALK_TO_TRIAGE },
        { arrived       , set_triage_time       , STATE::WAIT_IN_TRIAGE },
    }; 

    table[STATE::WAIT_IN_TRIAGE] = {
    //    GUARD               ACTION              DESTINATION
    //  +-------------------+-------------------+---------------------------+
        { time_elapsed      , get_diagnosis     , STATE::DISPATCH       },
    };

    table[STATE::DISPATCH] = {
    //    GUARD           ACTION                              DESTINATION
    //  +---------------+-----------------------------------+---------------------------+
        { to_doctor     , request_chair                     , STATE::WAIT_CHAIR_3       },
        { to_icu        , set_exit_motive_and_destination   , STATE::WALK_TO_EXIT       }
    };

    table[STATE::WAIT_CHAIR_3] = {
    //    GUARD                   ACTION                              DESTINATION
    //  +-----------------------+-----------------------------------+---------------------------+
        { no_chair_available    , set_exit_motive_and_destination   , STATE::WALK_TO_EXIT       },
        { got_chair             , set_destination_chair             , STATE::WALK_TO_CHAIR_3    },
    };

    table[STATE::WALK_TO_CHAIR_3] = {
    //    GUARD           ACTION             DESTINATION
    //  +---------------+-------------------+---------------------------+
        { not_arrived   , walk              , STATE::WALK_TO_CHAIR_3    },
        { arrived       , enqueue_in_doctor , STATE::WAIT_FOR_DOCTOR    },
    };

    table[STATE::WAIT_FOR_DOCTOR] = {
    //    GUARD               ACTION                      DESTINATION
    //  +-------------------+---------------------------+---------------------------+
        { doctor_turn       , set_doctor_destination    , STATE::WALK_TO_DOCTOR      },
        { doctor_timeout    , empty                     , STATE::NO_ATTENTION        }
    };

    table[STATE::WALK_TO_DOCTOR] = {
    //    GUARD           ACTION                  DESTINATION
    //  +---------------+-----------------------+-----------------------+
        { not_arrived   , walk                  , STATE::WALK_TO_DOCTOR },
        { arrived       , set_doctor_time       , STATE::WAIT_IN_DOCTOR },
    }; 

    table[STATE::WAIT_IN_DOCTOR] = {
    //    GUARD               ACTION                              DESTINATION
    //  +-------------------+-----------------------------------+---------------------------+
        { time_elapsed      , set_exit_motive_and_destination   , STATE::WALK_TO_EXIT       },
    };

    table[STATE::NO_ATTENTION] = {
    //    GUARD           ACTION                              DESTINATION
    //  +---------------+-----------------------------------+---------------------------+
        { always_true   , set_exit_motive_and_destination   , STATE::WALK_TO_EXIT       },
    };

    table[STATE::WALK_TO_EXIT] = {
    //    GUARD           ACTION      DESTINATION
    //  +---------------+-----------+---------------------------+
        { not_arrived   , walk      , STATE::WALK_TO_EXIT       },
        { arrived       , empty     , STATE::AWAITING_DELETION  },
    };

    table[STATE::AWAITING_DELETION] = {
        { always_true, [&](fsm& m) { std::cout << m._patient->getId() << " NOT REMOVED\n";}, STATE::AWAITING_DELETION}
    };

    // clang-format on

    return table;
}

/// @brief Set the entry action for each state
sti::patient_fsm::entry_list create_entry_actions()
{
    // using fsm   = sti::patient_fsm;
    // using STATE = sti::patient_fsm::STATE;

    auto entries = sti::patient_fsm::entry_list {};

    return entries;
}

/// @brief Set the exit action for each state
sti::patient_fsm::exit_list create_exit_actions()
{
    using fsm   = sti::patient_fsm;
    using STATE = sti::patient_fsm::STATE;

    auto exits = sti::patient_fsm::exit_list {};

    // Set the exit action for the corresponding states
    auto release_chair = [](fsm& m) {
        m._flyweight->chairs->release_chair(m._destination);
    };

    // Return the chairs
    exits[STATE::WAIT_RECEPTION_TURN] = release_chair;
    exits[STATE::WAIT_TRIAGE_TURN]    = release_chair;
    exits[STATE::WAIT_FOR_DOCTOR]     = release_chair;

    // Release the reception
    exits[STATE::WAIT_IN_RECEPTION] = [](fsm& m) {
        m._flyweight->reception->dequeue(m._patient->getId());
    };

    // Release the triage
    exits[STATE::WAIT_IN_TRIAGE] = [](fsm& m) {
        m._flyweight->triage->dequeue(m._patient->getId());
    };

    // Dequeue form the doctor
    auto dequeue_from_doctor = [](fsm& m) {
        m._flyweight->doctors->queues()->dequeue(m._diagnosis.doctor_assigned, m._patient->getId());
    };

    exits[STATE::NO_ATTENTION]   = dequeue_from_doctor;
    exits[STATE::WAIT_IN_DOCTOR] = dequeue_from_doctor;

    return exits;
}

} // namespace

/// @brief Default construct an empty FSM, starting in the initial state
/// @param fw The patient flyweight
/// @param patient The patient associated with this FSM
sti::patient_fsm::patient_fsm(patient_flyweight* fw, patient_agent* patient)
    : _flyweight { fw }
    , _patient { patient }
    , _transition_table { create_transition_table() }
    , _entries { create_entry_actions() }
    , _exits { create_exit_actions() }
    , _current_state(STATE::ENTRY)
    , _destination {}
    , _attention_end {}
{
}

/// @brief Execute the FSM logic, change states,
void sti::patient_fsm::tick()
{

    if constexpr (sti::debug::fsm_debug_patient) {
        // if (_patient->getId() == repast::AgentId(838, 0, 3, 0)) {
        //     volatile auto dummy_var = true;
        // }
    }

    // The logic is: iterate over the current state looking for a guard
    // returning true, when found, execute the exit action (if any), then
    // execute the transition action, and finally update the state and return
    for (const auto& transition : _transition_table.at(_current_state)) {

        // Guard triggered, execute the plan
        if (transition.guard(*this)) {

            // Execute the exit action
            if (_exits.find(_current_state) != _exits.end()) {
                _exits.at(_current_state)(*this);
            }

            // Execute the transition action
            transition.action(*this);

            // Update the state, run the entry action and break the loop to
            // avoid checking fot guards in the new state
            _current_state = transition.destination;
            if (_entries.find(_current_state) != _entries.end()) {
                _entries.at(_current_state)(*this);
            }
            break;
        }
    }
}