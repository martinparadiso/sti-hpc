/// @file patient_fsm.hpp
/// @brief Implement the patient circulation logic with a state machine
#pragma once

#include "clock.hpp"
#include "coordinates.hpp"
#include <boost/serialization/access.hpp>
#include <functional>
#include <map>
#include <memory>
#include <utility>

// Fw. declarations
namespace sti {
class patient_agent;
struct patient_flyweight;
}

namespace sti {

struct patient_fsm {

    ////////////////////////////////////////////////////////////////////////////
    // STATES
    ////////////////////////////////////////////////////////////////////////////

    /// @brief The states of the FSM
    enum class STATE {
        ENTRY,
        WAIT_CHAIR_1,
        WALK_TO_CHAIR_1,
        WAIT_RECEPTION_TURN,
        WALK_TO_RECEPTION,
        WAIT_IN_RECEPTION,
        WAIT_CHAIR_2,
        WALK_TO_CHAIR_2,
        WAIT_TRIAGE_TURN,
        WALK_TO_TRIAGE,
        WAIT_IN_TRIAGE,
        WAIT_CHAIR_3,
        WALK_TO_CHAIR_3,
        WAIT_FOR_DOCTOR,
        WALK_TO_DOCTOR,
        WAIT_IN_DOCTOR,
        WAIT_IN_ICU,
        WALK_TO_ICU,
        SLEEP,
        WALK_TO_EXIT,
        AWAITING_DELETION,
    };

    ////////////////////////////////////////////////////////////////////////////
    // TYPES
    ////////////////////////////////////////////////////////////////////////////

    using guard_signature  = std::function<bool(patient_fsm&)>;
    using action_signature = std::function<void(patient_fsm&)>;
    struct transition_type {
        guard_signature  guard;
        action_signature action;
        STATE            destination;
    };
    using transition_table = std::map<STATE, std::vector<transition_type>>;

    using entry_signature = std::function<void(patient_fsm&)>;
    using exit_signature  = std::function<void(patient_fsm&)>;

    using entry_list = std::map<STATE, entry_signature>;
    using exit_list  = std::map<STATE, exit_signature>;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Default construct an empty FSM, starting in the initial state
    /// @param fw The patient flyweight
    /// @param patient The patient associated with this FSM
    patient_fsm(patient_flyweight* fw, patient_agent* patient);

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Execute the FSM logic, change states,
    void tick();

    ////////////////////////////////////////////////////////////////////////////
    // INTERNAL STATE
    ////////////////////////////////////////////////////////////////////////////

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _current_state;
        ar& _destination;
        ar& _attention_end;
        ar& _last_state;
    } // void serialize()

    patient_flyweight* _flyweight;
    patient_agent*     _patient;
    transition_table   _transition_table;
    entry_list         _entries;
    exit_list          _exits;

    // Internal serializable state
    STATE               _current_state;
    coordinates<double> _destination;
    datetime            _attention_end;
    std::string         _last_state;
}; // class patient_fsm

} // namespace sti

// State enum serialization
namespace boost {
namespace serialization {

    template <class Archive>
    void serialize(Archive& ar, sti::patient_fsm::STATE& s, const unsigned int /*unused*/)
    {
        ar& s;
    } // void serialize(...)

} // namespace serialization
} // namespaces boost
