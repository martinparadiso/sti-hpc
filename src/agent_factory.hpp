/// @brief Agent creation
#pragma once

#include <boost/lexical_cast.hpp>
#include <boost/json.hpp>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/Point.h>
#include <repast_hpc/SharedContinuousSpace.h>

#include "chair_manager.hpp"
#include "contagious_agent.hpp"
#include "hospital_plan.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "infection_logic/object_infection_cycle.hpp"
#include "json_serialization.hpp"
#include "object.hpp"
#include "patient.hpp"
#include "person.hpp"
#include "reception.hpp"
#include "space_wrapper.hpp"

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
    using space_ptr   = sti::space_wrapper*;

    /// @brief Create a new patient factory
    /// @param context The repast context, to insert the agent
    /// @param discrete_space The repast shared space
    /// @param c The clock
    /// @param patient_fw The flyweight containing all the patient shared attributes
    /// @param person_fw The flyweight containing all the person shared attributes
    /// @param object_fw The flyweight containing all the object shared attributes
    agent_factory(context_ptr                context,
                  space_ptr                  space,
                  sti::clock*                clock,
                  sti::hospital_plan*        hospital_plan,
                  sti::chair_manager*        chairs,
                  sti::reception*            reception,
                  sti::triage*               triage,
                  const boost::json::object& props)
        : _context { context }
        , _space { space }
        , _clock { clock }
        , _agents_created { 0 }
        , _infection_factory {
            human_infection_cycle::flyweight {
                space,
                _clock,
                boost::json::value_to<sti::human_infection_cycle::precission>(props.at("parameters").at("human").at("infection").at("chance")),
                boost::json::value_to<double>(props.at("parameters").at("human").at("infection").at("distance")),
                timedelta { boost::json::value_to<timedelta>(props.at("parameters").at("human").at("incubation").at("time")) } },
            object_infection_cycle::flyweight {
                space,
                boost::json::value_to<sti::infection_cycle::precission>(props.at("parameters").at("object").at("infection").at("chance")),
                boost::json::value_to<double>(props.at("parameters").at("object").at("infection").at("distance")) }
        }
        , _patient_flyweight { // clang-format off
            &_infection_factory,
            context,
            space,
            clock,
            hospital_plan,
            chairs,
            reception,
            triage,
            boost::json::value_to<double>(props.at("parameters").at("patient").at("walk_speed")),
            boost::json::value_to<timedelta>(props.at("parameters").at("reception").at("attention_time")),
            boost::json::value_to<timedelta>(props.at("parameters").at("triage").at("attention_time"))
        } // clang-format on
        , _person_flyweight { &_infection_factory }
        , _object_flyweight { &_infection_factory }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // PATIENT CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a brand new patient, with a new id, insert it into the context
    /// @param pos The position where to insert the patients
    /// @param st The stage of the patient infection
    /// @return A raw pointer to the contagious agent created
    agent_ptr insert_new_patient(coordinates<int>             pos,
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
        _space->move_to(id, loc);

        return patient;
    }

    /// @brief Recreate a serialized patient, with an existing id
    /// @param id The agent id
    /// @param data The serialized data
    /// @return A pointer to the newly created object
    agent_ptr recreate_patient(const repast::AgentId& id,
                               serial_data&           data)
    {
        auto* patient = new patient_agent { id,
                                            &_patient_flyweight };
        patient->serialize(id, data);
        return patient;
    };

    ////////////////////////////////////////////////////////////////////////////
    // PERSON CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a brand new patient, with a new id, insert it into the context
    /// @param pos The position where to insert the patients
    /// @param st The stage of the patient infection
    /// @return A raw pointer to the contagious agent created
    agent_ptr insert_new_person(coordinates<int>             pos,
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
        _space->move_to(id, loc);

        return person;
    }

    /// @brief Recreate a serialized patient, with an existing id
    /// @param id The agent id
    /// @param data The serialized data
    /// @return A pointer to the newly created object
    agent_ptr recreate_person(const repast::AgentId& id,
                              serial_data&           data) const
    {
        auto* person = new person_agent { id,
                                          &_person_flyweight };
        person->serialize(id, data);
        return person;
    };

    ////////////////////////////////////////////////////////////////////////////
    // OBJECT CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a brand new patient, with a new id, insert it into the context
    /// @param pos The position where to insert the patients
    /// @param st The stage of the patient infection
    /// @return A raw pointer to the contagious agent created
    agent_ptr insert_new_object(coordinates<int>              pos,
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
        _space->move_to(id, loc);

        return object;
    }

    /// @brief Recreate a serialized patient, with an existing id
    /// @param id The agent id
    /// @param data The serialized data
    /// @return A pointer to the newly created object
    agent_ptr recreate_object(const repast::AgentId& id,
                              serial_data&           data) const
    {
        auto* object = new object_agent { id,
                                          &_object_flyweight };
        object->serialize(id, data);
        return object;
    };

private:
    context_ptr _context;
    space_ptr   _space;
    clock*      _clock; // To indicate the patient creation time
    unsigned    _agents_created;

    // Infection factory
    infection_factory _infection_factory;

    // Agent flyweights
    patient_flyweight _patient_flyweight;
    person_flyweight  _person_flyweight;
    object_flyweight  _object_flyweight;
};

} // namespace sti
