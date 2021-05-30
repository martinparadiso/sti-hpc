/// @file patient_fsm.hpp
/// @brief Implement the patient circulation logic with a state machine
#pragma once

#include <boost/serialization/access.hpp>
#include <boost/serialization/variant.hpp>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <utility>

#include "clock.hpp"
#include "coordinates.hpp"
#include "doctors.hpp"
#include "triage.hpp"

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
    enum class STATE : std::size_t {
        ENTRY = 0UL,
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
        DISPATCH,
        WAIT_CHAIR_3,
        WALK_TO_CHAIR_3,
        WAIT_FOR_DOCTOR,
        WALK_TO_DOCTOR,
        WAIT_IN_DOCTOR,
        NO_ATTENTION,
        WAIT_ICU,
        WALK_TO_ICU,
        SLEEP,
        RESOLVE,
        LEAVE_ICU,
        MORGUE,
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
    using transition_table = std::array<std::vector<transition_type>, static_cast<std::size_t>(STATE::AWAITING_DELETION) + 1>;

    using entry_signature = std::function<void(patient_fsm&)>;
    using exit_signature  = std::function<void(patient_fsm&)>;

    using entry_list = std::map<STATE, entry_signature>;
    using exit_list  = std::map<STATE, exit_signature>;

    using patient_flyweight_ptr = patient_flyweight*;

    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT / SHARED ATTRIBUTES
    ////////////////////////////////////////////////////////////////////////////

    class flyweight {
    public:
        flyweight();

        transition_table transitions;
        entry_list       entries;
        exit_list        exits;
    };

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Default construct an empty FSM, starting in the initial state
    /// @param fw The patient flyweight
    /// @param patient The patient associated with this FSM
    patient_fsm(patient_flyweight_ptr fw, patient_agent* patient);

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
        ar& current_state;
        ar& destination;
        ar& attention_end;
        ar& last_state;
        ar& diagnosis;
    } // void serialize()

    patient_flyweight* patient_flyweight;
    patient_agent*     patient;

    // Internal serializable state
    STATE                    current_state;
    coordinates<double>      destination;
    datetime                 attention_end;
    std::string              last_state;
    triage::triage_diagnosis diagnosis;
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

BOOST_CLASS_IMPLEMENTATION(sti::patient_fsm, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(sti::patient_fsm, boost::serialization::track_never)
