/// @brief Agent creation
#pragma once

#include <cstdint>

#include "infection_logic/infection_factory.hpp"
#include "patient.hpp"
#include "person.hpp"

// Fw. declarations
namespace boost {
namespace json {
    class object;
} // namespace json
} // namespace boost

namespace repast {
template <typename T>
class SharedContext;
} // namespace repast

namespace sti {
class space_wrapper;
} // namespace sti

namespace sti {

/// @brief Help with the construction of agents
/// @details Agents have a complex initialization, private and shared
/// properties, to solve this problem a factory is used. The factory is created
/// once with the shared properties, and every time a new agent is created the
/// private attributes/properties must be supplied.
class agent_factory {

public:
    using agent       = contagious_agent;
    using agent_ptr   = agent*;
    using person_ptr  = person_agent*;
    using patient_ptr = patient_agent*;

    using patient_flyweight = patient_agent::flyweight;
    using person_flyweight  = person_agent::flyweight;

    using communicator_ptr = boost::mpi::communicator*;
    using context_ptr      = repast::SharedContext<agent>*;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a new patient factory
    /// @param comm The MPI communicator
    /// @param context A pointer to the repast context, to insert the agent
    /// @param space A pointer to the space_wrapper
    /// @param clock A pointer to the simulation clock
    /// @param hospital_plan A pointer to the hospital plan
    /// @param chair A pointer to the chair manager
    /// @param reception A pointer to the reception
    /// @param triage A pointer to the triage
    /// @param doctors A pointer to the doctors manager
    /// @param icu A pointer to the ICU
    /// @param hospital_props The JSON object containing the simulation properties
    agent_factory(communicator_ptr           comm,
                  context_ptr                context,
                  sti::space_wrapper*        space,
                  sti::clock*                clock,
                  sti::hospital_plan*        hospital_plan,
                  sti::chair_manager*        chairs,
                  sti::reception*            reception,
                  sti::triage*               triage,
                  sti::doctors*              doctors,
                  sti::icu*                  icu,
                  const boost::json::object& hospital_props);

    ////////////////////////////////////////////////////////////////////////////
    // GETTERS
    ////////////////////////////////////////////////////////////////////////////

    infection_factory* get_infection_factory();

    ////////////////////////////////////////////////////////////////////////////
    // PATIENT CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a brand new patient, with a new id, insert it into the context
    /// @param pos The position where to insert the patients
    /// @param st The stage of the patient infection
    /// @return A raw pointer to the contagious agent created
    patient_ptr insert_new_patient(const coordinates<double>&   pos,
                                   human_infection_cycle::STAGE st);

    /// @brief Recreate a serialized patient, with an existing id
    /// @param id The agent id
    /// @param data The serialized data
    /// @return A pointer to the newly created object
    patient_ptr recreate_patient(const repast::AgentId& id,
                                 serial_data&           data);

    ////////////////////////////////////////////////////////////////////////////
    // PERSON CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a brand new patient, with a new id, insert it into the context
    /// @param pos The position where to insert the patients
    /// @param type The person type/rol, for post processing
    /// @param st The stage of the patient infection
    /// @param immune The person immunity, True if is immune
    /// @return A raw pointer to the contagious agent created
    person_ptr insert_new_person(const coordinates<double>&       pos,
                                 const person_agent::person_type& type,
                                 human_infection_cycle::STAGE     st,
                                 bool                             immune);

    /// @brief Recreate a serialized patient, with an existing id
    /// @param id The agent id
    /// @param data const The serialized data&
    /// @return A pointer to the newly created object
    person_ptr recreate_person(const repast::AgentId& id,
                               serial_data&           data) const;

private:
    communicator_ptr _communicator;
    context_ptr      _context;
    space_wrapper*   _space;
    clock*           _clock;
    std::uint32_t    _agents_created;

    // Infection factory
    infection_factory _infection_factory;

    // Agent flyweights
    patient_flyweight _patient_flyweight;
    person_flyweight  _person_flyweight;
};

} // namespace sti
