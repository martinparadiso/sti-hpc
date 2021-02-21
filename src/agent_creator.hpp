/// @brief Agent creation
#pragma once

#include <exception>
#include <memory>

#include <repast_hpc/AgentId.h>
#include <repast_hpc/Point.h>

#include "contagious_agent.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "infection_logic/object_infection_cycle.hpp"
#include "patient.hpp"
#include "person.hpp"
#include "object.hpp"
#include "plan/plan.hpp"

namespace sti {

/// @brief Help with the construction of agents
/// @details Agents have a complex initialization, private and shared
///          properties, to solve this problem a factory is used. The factory is
///          created once with the shared properties, and every time a new agent
///          is created the private attributes/properties must be supplied.
class agent_factory {

public:
    using agent     = contagious_agent;
    using agent_ptr = agent*;

    using patient_flyweight = patient_agent::flyweight;
    using person_flyweight  = person_agent::flyweight;
    using object_flyweight  = object_agent::flyweight;

    using context_ptr = repast::SharedContext<agent>*;
    using space_ptr   = repast::SharedDiscreteSpace<agent, repast::StrictBorders, repast::SimpleAdder<agent>>*;

    using serial_data = contagious_agent::serial_data;

    /// @brief Create a new patient factory
    /// @param context The repast context, to insert the agent
    /// @param discrete_space The repast shared space
    /// @param c The clock
    /// @param patient_fw The flyweight containing all the patient shared attributes
    /// @param person_fw The flyweight containing all the person shared attributes
    /// @param object_fw The flyweight containing all the object shared attributes
    agent_factory(context_ptr              context,
                  space_ptr                discrete_space,
                  clock*                   c,
                  const patient_flyweight& patient_fw,
                  const person_flyweight&  person_fw,
                  const object_flyweight&  object_fw,
                  const infection_factory& inf_factory)
        : _context { context }
        , _discrete_space { discrete_space }
        , _clock { c }
        , _agents_created {}
        , _patient_flyweight { patient_fw }
        , _person_flyweight { person_fw }
        , _object_flyweight { object_fw }
        , _infection_factory { inf_factory }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // PATIENT CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a brand new patient, with a new id, insert it into the context
    /// @param pos The position where to insert the patients
    /// @param st The stage of the patient infection
    /// @return A raw pointer to the contagious agent created
    agent_ptr insert_new_patient(plan::coordinates            pos,
                                 human_infection_cycle::STAGE st)
    {
        const auto rank = repast::RepastProcess::instance()->rank();
        const auto type = to_int(contagious_agent::type::PATIENT);
        const auto i    = static_cast<int>(_agents_created++);
        const auto id   = repast::AgentId(i, rank, type, rank);

        const auto hic     = _infection_factory.make_human_cycle(id, st);
        auto*      patient = new patient_agent { id, &_patient_flyweight, _clock->now(), hic };

        // Move the agent into position, add it to the repast contexts
        _context->addAgent(patient);
        const auto loc = repast::Point<int> { static_cast<int>(pos.x), static_cast<int>(pos.y) };
        _discrete_space->moveTo(id, loc);

        return patient;
    }

    /// @brief Recreate a serialized patient, with an existing id
    /// @param id The agent id
    /// @param data The serialized data
    /// @return A pointer to the newly created object
    agent_ptr recreate_patient(const repast::AgentId& id,
                               const serial_data&     data) const
    {
        auto* patient = new patient_agent { id,
                                            &_patient_flyweight,
                                            data,
                                            &_infection_factory };
        return patient;
    };

    ////////////////////////////////////////////////////////////////////////////
    // PERSON CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a brand new patient, with a new id, insert it into the context
    /// @param pos The position where to insert the patients
    /// @param st The stage of the patient infection
    /// @return A raw pointer to the contagious agent created
    agent_ptr insert_new_person(plan::coordinates            pos,
                                human_infection_cycle::STAGE st)
    {
        const auto rank = repast::RepastProcess::instance()->rank();
        const auto type = to_int(contagious_agent::type::FIXED_PERSON);
        const auto i    = static_cast<int>(_agents_created++);
        const auto id   = repast::AgentId(i, rank, type, rank);

        const auto hic    = _infection_factory.make_human_cycle(id, st);
        auto*      person = new person_agent { id, &_person_flyweight, hic };

        // Move the agent into position, add it to the repast contexts
        _context->addAgent(person);
        const auto loc = repast::Point<int> { static_cast<int>(pos.x), static_cast<int>(pos.y) };
        _discrete_space->moveTo(id, loc);

        return person;
    }

    /// @brief Recreate a serialized patient, with an existing id
    /// @param id The agent id
    /// @param data The serialized data
    /// @return A pointer to the newly created object
    agent_ptr recreate_person(const repast::AgentId& id,
                              const serial_data&     data) const
    {
        auto* patient = new person_agent { id,
                                           &_person_flyweight,
                                           data,
                                           &_infection_factory };
        return patient;
    };

    ////////////////////////////////////////////////////////////////////////////
    // OBJECT CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a brand new patient, with a new id, insert it into the context
    /// @param pos The position where to insert the patients
    /// @param st The stage of the patient infection
    /// @return A raw pointer to the contagious agent created
    agent_ptr insert_new_object(plan::coordinates             pos,
                                object_infection_cycle::STAGE st)
    {
        const auto rank = repast::RepastProcess::instance()->rank();
        const auto type = to_int(contagious_agent::type::OBJECT);
        const auto i    = static_cast<int>(_agents_created++);
        const auto id   = repast::AgentId(i, rank, type, rank);

        const auto hic    = _infection_factory.make_object_cycle(id, st);
        auto*      object = new object_agent { id, &_object_flyweight, hic };

        // Move the agent into position, add it to the repast contexts
        _context->addAgent(object);
        const auto loc = repast::Point<int> { static_cast<int>(pos.x), static_cast<int>(pos.y) };
        _discrete_space->moveTo(id, loc);

        return object;
    }

    /// @brief Recreate a serialized patient, with an existing id
    /// @param id The agent id
    /// @param data The serialized data
    /// @return A pointer to the newly created object
    agent_ptr recreate_object(const repast::AgentId& id,
                              const serial_data&     data) const
    {
        auto* patient = new object_agent { id,
                                           &_object_flyweight,
                                           data,
                                           &_infection_factory };
        return patient;
    };

private:
    context_ptr _context;
    space_ptr   _discrete_space;
    clock*      _clock; // To indicate the patient creation time
    unsigned    _agents_created;

    patient_flyweight _patient_flyweight;
    person_flyweight  _person_flyweight;
    object_flyweight  _object_flyweight;

    // Infection
    infection_factory _infection_factory;
};

} // namespace sti
