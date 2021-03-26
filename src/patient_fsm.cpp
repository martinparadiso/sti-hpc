#include "patient_fsm.hpp"

#include "coordinates.hpp"
#include "patient.hpp"
#include <iostream>

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
    case STATE::WAIT_IN_ICU:
        return "WAIT_IN_ICU";
    case STATE::WALK_TO_ICU:
        return "WALK_TO_ICU";
    case STATE::SLEEP:
        return "SLEEP";
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

    // Helper functions
    auto always_true  = [](fsm& /*unused*/) { return true; };
    auto always_false = [](fsm& /*unused*/) { return false; };
    auto empty        = [](fsm& /*unused*/) {};

    // Request a chair, and check if the response was fulfilled
    auto request_chair = [](fsm& m) {
        m._flyweight->chairs->request_chair(m._patient->getId());
    };

    auto no_chair_response = [](fsm& m) {
        auto response = m._flyweight->chairs->peek_response(m._patient->getId());
        return response.is_initialized() == false;
    };

    auto got_chair = [](fsm& m) {
        auto response_peek = m._flyweight->chairs->peek_response(m._patient->getId());
        if (response_peek) {
            if (response_peek->chair_location) {
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
        if (response) {
            if (!response->chair_location) {
                // Remove the response from the queue
                m._flyweight->chairs->get_response(m._patient->getId());
                return true;
            }
        }
        return false;
    };

    // Check if the agent has arrived at the destination
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

    // Check if the time has passed
    auto time_elapsed = [](fsm& m) {
        return m._attention_end < m._flyweight->clk->now();
    };

    auto not_time_elapsed = [](fsm& m) {
        return !(m._attention_end < m._flyweight->clk->now());
    };

    // Walk to the destination
    auto walk = [](fsm& m) {
        m._flyweight->space->move_towards(
            m._patient->getId(),
            m._destination,
            m._flyweight->walk_speed);
    };

    // Reception turn check, and functions related to the reception
    auto enqueue_in_reception = [](fsm& m) {
        m._flyweight->reception->enqueue(m._patient->getId());
    };

    auto reception_turn = [](fsm& m) -> bool {
        return m._flyweight->reception->is_my_turn(m._patient->getId()).has_value();
    };

    auto not_reception_turn = [](fsm& m) -> bool {
        return !m._flyweight->reception->is_my_turn(m._patient->getId()).has_value();
    };

    auto set_destination_reception = [](fsm& m) {
        m._destination = m._flyweight->reception->is_my_turn(m._patient->getId()).get();
    };

    auto set_reception_time = [](fsm& m) {
        m._attention_end = m._flyweight->clk->now() + m._flyweight->reception_time;
    };

    // triage turn check, and functions related to the triage
    auto enqueue_in_triage = [](fsm& m) {
        m._flyweight->triage->enqueue(m._patient->getId());
    };

    auto triage_turn = [](fsm& m) -> bool {
        return m._flyweight->triage->is_my_turn(m._patient->getId()).has_value();
    };

    auto not_triage_turn = [](fsm& m) -> bool {
        return !m._flyweight->triage->is_my_turn(m._patient->getId()).has_value();
    };

    auto set_destination_triage = [](fsm& m) {
        m._destination = m._flyweight->triage->is_my_turn(m._patient->getId()).get();
    };

    auto set_triage_time = [](fsm& m) {
        m._attention_end = m._flyweight->clk->now() + m._flyweight->triage_duration;
    };

    // Exit transition action
    auto set_exit_motive_and_destination = [](fsm& m) {
        m._destination = m._flyweight->hospital->exit().location.continuous();
        m._last_state  = state_2_string(m._current_state);
    };

    // The transition table definition
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
    //    GUARD               ACTION                              DESTINATION
    //  +-------------------+-----------------------------------+---------------------------+
        { time_elapsed      , set_exit_motive_and_destination   , STATE::WALK_TO_EXIT       },
    };

    table[STATE::WALK_TO_EXIT] = {
    //    GUARD           ACTION      DESTINATION
    //  +---------------+-----------+---------------------------+
        { not_arrived   , walk      , STATE::WALK_TO_EXIT       },
        { arrived       , empty     , STATE::AWAITING_DELETION  },
    };

    // clang-format on

    return table;
}

/// @brief Set the entry action for each state
sti::patient_fsm::entry_list create_entry_actions()
{
    using fsm   = sti::patient_fsm;
    using STATE = sti::patient_fsm::STATE;

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

    // Release the reception
    exits[STATE::WAIT_IN_RECEPTION] = [](fsm& m) {
        m._flyweight->reception->dequeue(m._patient->getId());
    };

    // Release the triage
    exits[STATE::WAIT_IN_TRIAGE] = [](fsm& m) {
        m._flyweight->triage->dequeue(m._patient->getId());
    };
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