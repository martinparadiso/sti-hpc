#include "patient_fsm.hpp"

#include <algorithm>
#include <repast_hpc/AgentId.h>
#include <variant>

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
#include "icu.hpp"
#include "icu/icu_admission.hpp"
#include "icu/real_icu.hpp"

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
    case STATE::WAIT_ICU:
        return "WAIT_ICU";
    case STATE::WALK_TO_ICU:
        return "WALK_TO_ICU";
    case STATE::SLEEP:
        return "SLEEP";
    case STATE::RESOLVE:
        return "RESOLVE";
    case STATE::LEAVE_ICU:
        return "LEAVE_ICU";
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
        m.patient_flyweight->chairs->request_chair(m.patient->getId());
    };

    auto got_chair = [](fsm& m) {
        auto response_peek = m.patient_flyweight->chairs->peek_response(m.patient->getId());
        if (response_peek.is_initialized()) {
            if (response_peek->chair_location.is_initialized()) {
                return true;
            }
        }
        return false;
    };

    auto set_destination_chair = [](fsm& m) {
        auto response = m.patient_flyweight->chairs->get_response(m.patient->getId());
        m.destination = response->chair_location.get();
    };

    auto no_chair_available = [](fsm& m) {
        auto response = m.patient_flyweight->chairs->peek_response(m.patient->getId());
        if (response.is_initialized()) {
            if (!response->chair_location.is_initialized()) {
                // Remove the response from the queue
                m.patient_flyweight->chairs->get_response(m.patient->getId());
                return true;
            }
        }
        return false;
    };

    ////////////////////////////////////////////////////////////////////////////
    // TIME WAIT FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto time_elapsed = [](fsm& m) {
        return m.attention_end < m.patient_flyweight->clk->now();
    };

    ////////////////////////////////////////////////////////////////////////////
    // WALK AND MOVEMENT FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto arrived = [](fsm& m) {
        const auto patient_location = sti::coordinates<double> {
            m.patient_flyweight->space->get_continuous_location(m.patient->getId())
        };
        return m.destination == patient_location;
    };

    auto not_arrived = [](fsm& m) {
        const auto patient_location = sti::coordinates<double> {
            m.patient_flyweight->space->get_continuous_location(m.patient->getId())
        };
        return m.destination != patient_location;
    };

    auto walk = [](fsm& m) {
        auto movement_left    = m.patient_flyweight->walk_speed * m.patient_flyweight->clk->seconds_per_tick();
        auto current_location = m.patient_flyweight->space->get_continuous_location(m.patient->getId());

        while (movement_left > 0.0 && current_location != m.destination) {
            const auto next_cell         = m.patient_flyweight->hospital->get_pathfinder()->next_step(current_location.discrete(),
                                                                                              m.destination.discrete());
            const auto new_location      = m.patient_flyweight->space->move_towards(m.patient->getId(),
                                                                               next_cell.continuous(),
                                                                               movement_left);
            const auto distance_traveled = sti::sq_distance(new_location, current_location);
            current_location             = new_location;
            movement_left -= distance_traveled;
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    // RECEPTION FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto enqueue_in_reception = [](fsm& m) {
        m.patient_flyweight->reception->enqueue(m.patient->getId());
    };

    auto reception_turn = [](fsm& m) -> bool {
        return m.patient_flyweight->reception->is_my_turn(m.patient->getId()).has_value();
    };

    auto set_destination_reception = [](fsm& m) {
        m.destination = m.patient_flyweight->reception->is_my_turn(m.patient->getId()).get();
    };

    auto set_reception_time = [](fsm& m) {
        m.attention_end = m.patient_flyweight->clk->now() + m.patient_flyweight->reception_time;
    };

    ////////////////////////////////////////////////////////////////////////////
    // TRIAGE FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto enqueue_in_triage = [](fsm& m) {
        m.patient_flyweight->triage->enqueue(m.patient->getId());
    };

    auto triage_turn = [](fsm& m) -> bool {
        return m.patient_flyweight->triage->is_my_turn(m.patient->getId()).has_value();
    };

    auto set_destination_triage = [](fsm& m) {
        m.destination = m.patient_flyweight->triage->is_my_turn(m.patient->getId()).get();
    };

    auto set_triage_time = [](fsm& m) {
        m.attention_end = m.patient_flyweight->clk->now() + m.patient_flyweight->triage_duration;
    };

    auto get_diagnosis = [](fsm& m) {
        m.diagnosis = m.patient_flyweight->triage->diagnose();
    };

    auto to_doctor = [](fsm& m) {
        return sti::triage::holds_doctor_diagnosis(m.diagnosis);
    };

    auto to_icu = [](fsm& m) {
        return sti::triage::holds_icu_diagnosis(m.diagnosis);
    };

    ////////////////////////////////////////////////////////////////////////////
    // DOCTOR FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto enqueue_in_doctor = [](fsm& m) {
        using doc_diagnosis              = sti::triage::doctor_diagnosis;
        const auto& doctor_assigned      = boost::get<doc_diagnosis>(m.diagnosis).doctor_assigned;
        const auto& attention_time_limit = boost::get<doc_diagnosis>(m.diagnosis).attention_time_limit;
        m.patient_flyweight->doctors->queues()->enqueue(doctor_assigned,
                                                        m.patient->getId(),
                                                        attention_time_limit);
    };

    auto doctor_turn = [](fsm& m) {
        const auto& doctor_assigned = boost::get<sti::triage::doctor_diagnosis>(m.diagnosis).doctor_assigned;
        const auto  response        = m.patient_flyweight->doctors->queues()->is_my_turn(doctor_assigned,
                                                                                 m.patient->getId());
        return response.is_initialized();
    };

    auto set_doctor_destination = [](fsm& m) {
        const auto& doctor_assigned = boost::get<sti::triage::doctor_diagnosis>(m.diagnosis).doctor_assigned;
        const auto  response        = m.patient_flyweight->doctors->queues()->is_my_turn(doctor_assigned,
                                                                                 m.patient->getId());
        m.destination               = response.get();
    };

    auto doctor_timeout = [](fsm& m) {
        return boost::get<sti::triage::doctor_diagnosis>(m.diagnosis).attention_time_limit < m.patient_flyweight->clk->now();
    };

    auto set_doctor_time = [](fsm& m) {
        const auto& doctor_assigned = boost::get<sti::triage::doctor_diagnosis>(m.diagnosis).doctor_assigned;
        m.attention_end             = m.patient_flyweight->clk->now() + m.patient_flyweight->doctors->get_attention_duration(doctor_assigned);
    };

    ////////////////////////////////////////////////////////////////////////////
    // ICU FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    auto request_icu = [](fsm& m) {
        m.patient_flyweight->icu->admission().request_bed(m.patient->getId());
    };

    auto icu_available = [](fsm& m) {
        // First "peek" the response without removing it. Only remove the
        // response from the pool if the response is affirmative. If a negative
        // response is removed, the "icu_full" guard will never find the response
        const auto peek_response = m.patient_flyweight->icu->admission().peek_response(m.patient->getId());

        if (peek_response.is_initialized()) {
            if (peek_response.get()) {
                m.patient_flyweight->icu->admission().get_response(m.patient->getId());
                return true;
            }
        }
        return false;
    };

    auto icu_full = [](fsm& m) {
        // First "peek" the response without removing it. Only remove the
        // response from the pool if the response is negative. If a positive
        // response is removed, the "icu_available" guard will never find the
        // response
        const auto peek_response = m.patient_flyweight->icu->admission().peek_response(m.patient->getId());

        if (peek_response.is_initialized()) {
            if (!peek_response.get()) {
                m.patient_flyweight->icu->admission().get_response(m.patient->getId());
                return true;
            }
        }
        return false;
    };

    auto set_icu_destination = [](fsm& m) {
        m.destination = m.patient_flyweight->hospital->icu().location.continuous();
    };

    auto enter_icu = [](fsm& m) {
        m.patient_flyweight->icu->get_real_icu()->get().insert(m.patient);
        m.attention_end = m.patient_flyweight->clk->now() + boost::get<sti::triage::icu_diagnosis>(m.diagnosis).sleep_time;
    };

    auto leave_icu = [](fsm& m) {
        m.patient_flyweight->icu->get_real_icu()->get().remove(m.patient);
    };

    auto alive = [](fsm& m) {
        return boost::get<sti::triage::icu_diagnosis>(m.diagnosis).survives;
    };

    auto dead = [](fsm& m) {
        return !boost::get<sti::triage::icu_diagnosis>(m.diagnosis).survives;
    };

    auto kill = [](fsm& m) {
        m.last_state = state_2_string(m.current_state);
    };

    ////////////////////////////////////////////////////////////////////////////
    // EXIT TRANSITION
    ////////////////////////////////////////////////////////////////////////////

    auto set_exit_motive_and_destination = [](fsm& m) {
        m.destination = m.patient_flyweight->hospital->exit().location.continuous();
        m.last_state  = state_2_string(m.current_state);
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
    //    GUARD           ACTION              DESTINATION
    //  +---------------+-------------------+-----------------------+
        { to_doctor     , request_chair     , STATE::WAIT_CHAIR_3   },
        { to_icu        , request_icu       , STATE::WAIT_ICU       }
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

    table[STATE::WAIT_ICU] = {
    //    GUARD           ACTION                              DESTINATION
    //  +---------------+-----------------------------------+-----------------------+
        { icu_available , set_icu_destination               , STATE::WALK_TO_ICU    },
        { icu_full      , set_exit_motive_and_destination   , STATE::WALK_TO_ICU    },
    };

    table[STATE::WALK_TO_ICU] = {
    //    GUARD           ACTION              DESTINATION
    //  +---------------+-------------------+-----------------------+
        { not_arrived   , walk              , STATE::WALK_TO_ICU    },
        { arrived       , enter_icu         , STATE::SLEEP          },
    };

    table[STATE::SLEEP] = {
    //    GUARD           ACTION              DESTINATION
    //  +---------------+-------------------+-------------------+
        { time_elapsed  , empty             , STATE::RESOLVE   },
    };

    table[STATE::RESOLVE] = {
    //    GUARD           ACTION                              DESTINATION
    //  +---------------+-----------------------------------+-----------------------+
        { alive         , leave_icu                         , STATE::LEAVE_ICU      },
        { dead          , empty                             , STATE::MORGUE         }
    };

    table[STATE::LEAVE_ICU] = {
    //    GUARD           ACTION                              DESTINATION
    //  +---------------+-----------------------------------+-----------------------+
        { always_true   , set_exit_motive_and_destination   , STATE::WALK_TO_EXIT   }
    };

    table[STATE::MORGUE] = {
    //    GUARD           ACTION      DESTINATION
    //  +---------------+-----------+-----------------------+
        { always_true   , kill     , STATE::AWAITING_DELETION   }
    };

    table[STATE::WALK_TO_EXIT] = {
    //    GUARD           ACTION      DESTINATION
    //  +---------------+-----------+---------------------------+
        { not_arrived   , walk      , STATE::WALK_TO_EXIT       },
        { arrived       , empty     , STATE::AWAITING_DELETION  },
    };

    table[STATE::AWAITING_DELETION] = {
        
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
        m.patient_flyweight->chairs->release_chair(m.destination);
    };

    // Return the chairs
    exits[STATE::WAIT_RECEPTION_TURN] = release_chair;
    exits[STATE::WAIT_TRIAGE_TURN]    = release_chair;
    exits[STATE::WAIT_FOR_DOCTOR]     = release_chair;

    // Release the reception
    exits[STATE::WAIT_IN_RECEPTION] = [](fsm& m) {
        m.patient_flyweight->reception->dequeue(m.patient->getId());
    };

    // Release the triage
    exits[STATE::WAIT_IN_TRIAGE] = [](fsm& m) {
        m.patient_flyweight->triage->dequeue(m.patient->getId());
    };

    // Dequeue from the doctor
    auto dequeue_from_doctor = [](fsm& m) {
        const auto doctor_assigned = boost::get<sti::triage::doctor_diagnosis>(m.diagnosis).doctor_assigned;
        m.patient_flyweight->doctors->queues()->dequeue(doctor_assigned, m.patient->getId());
    };

    exits[STATE::NO_ATTENTION]   = dequeue_from_doctor;
    exits[STATE::WAIT_IN_DOCTOR] = dequeue_from_doctor;

    return exits;
}

} // namespace

/// @brief Default construct the FSM transitions
sti::patient_fsm::flyweight::flyweight()
    : transitions { create_transition_table() }
    , entries { create_entry_actions() }
    , exits { create_exit_actions() }
{
}

/// @brief Default construct an empty FSM, starting in the initial state
/// @param fw The patient flyweight
/// @param patient The patient associated with this FSM
sti::patient_fsm::patient_fsm(patient_flyweight_ptr fw, patient_agent* patient)
    : patient_flyweight { fw }
    , patient { patient }
    , current_state(STATE::ENTRY)
    , destination {}
{
}

/// @brief Execute the FSM logic, change states,
void sti::patient_fsm::tick()
{

    if constexpr (sti::debug::fsm_debug_patient) {
        // if (patient->getId() == repast::AgentId(838, 0, 3, 0)) {
        //     volatile auto dummy_var = true;
        // }
    }

    // The logic is: iterate over the current state looking for a guard
    // returning true, when found, execute the exit action (if any), then
    // execute the transition action, and finally update the state and return
    for (const auto& transition : patient_flyweight->fsm.transitions.at(current_state)) {

        // Guard triggered, execute the plan
        if (transition.guard(*this)) {

            // Execute the exit action
            if (patient_flyweight->fsm.exits.find(current_state) != patient_flyweight->fsm.exits.end()) {
                patient_flyweight->fsm.exits.at(current_state)(*this);
            }

            // Execute the transition action
            transition.action(*this);

            // Update the state, run the entry action and break the loop to
            // avoid checking fot guards in the new state
            current_state = transition.destination;
            if (patient_flyweight->fsm.entries.find(current_state) != patient_flyweight->fsm.entries.end()) {
                patient_flyweight->fsm.entries.at(current_state)(*this);
            }
            break;
        }
    }
}