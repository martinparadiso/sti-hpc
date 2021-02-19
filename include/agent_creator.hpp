/// @brief Agent creation
#pragma once

#include <exception>
#include <memory>

#include <repast_hpc/AgentId.h>
#include <repast_hpc/Point.h>

#include "contagious_agent.hpp"
#include "parallel_agent.hpp"
#include "plan/plan.hpp"

namespace sti {



/// @brief Help with the construction of agents
/// @details Agents have a complex initialization, private and shared
///          properties, to solve this problem a factory is used. The factory is
///          created once with the shared properties, and every time a new agent
///          is created the private attributes/properties must be supplied.
class patient_factory {

public:
    using agent_ptr    = parallel_agent*;
    using flyweight    = patient_agent::flyweight;

    using context_ptr = repast::SharedContext<parallel_agent>*;
    using space_ptr   = repast::SharedDiscreteSpace<parallel_agent, repast::StrictBorders, repast::SimpleAdder<parallel_agent>>*;

    using serial_data = contagious_agent::serial_data;

    /// @brief Create a new patient factory
    /// @param context The repast context, to insert the agent
    /// @param discrete_space The repast shared space
    /// @param c The clock
    /// @param fw The flyweight containing all the agent shared attributes
    patient_factory(context_ptr      context,
                    space_ptr        discrete_space,
                    clock*           c,
                    const flyweight& fw)
        : _context { context }
        , _discrete_space { discrete_space }
        , _clock { c }
        , _patient_flyweight { std::make_unique<flyweight>(fw) }
    {
    }

    /// @brief Construct a patient from serialized data
    /// @param 
    agent_ptr deserialize(const repast::AgentId& id, const serial_data& data) {
        auto patient = std::make_unique<patient_agent>(_patient_flyweight.get(), data);
        return new parallel_agent{id, std::move(patient)};
    }

    /// @brief Construct a new patient
    /// @param pos The position where to insert the patients
    /// @return A raw pointer to the parallel agent created
    agent_ptr make_agent(plan::coordinates pos)
    {
        const auto rank = repast::RepastProcess::instance()->rank();
        const auto type = to_int(contagious_agent::type::PATIENT);
        const auto i = static_cast<int>(_agents_created++);
        const auto id   = repast::AgentId(i, rank, type, rank);

        auto  patient = std::make_unique<patient_agent>(_patient_flyweight.get(), _clock->now());
        auto* pa      = new parallel_agent { id, std::move(patient) };

        const auto loc = repast::Point<int> { static_cast<int>(pos.x), static_cast<int>(pos.y) };

        _context->addAgent(pa);
        _discrete_space->moveTo(id, loc);

        return pa;
    }

private:
    context_ptr                _context;
    space_ptr                  _discrete_space;
    clock*                     _clock; // To indicate the patient creation time
    unsigned _agents_created;
    std::unique_ptr<flyweight> _patient_flyweight; // Shared attributes
};

} // namespace sti
