/// @file infection_logic/infection_factory.hpp
/// @brief Infection factory, to ease the creation of infection
#pragma once

#include <repast_hpc/AgentId.h>

#include "../clock.hpp"
#include "human_infection_cycle.hpp"
#include "object_infection_cycle.hpp"

namespace sti {

class parallel_agent;

/// @brief Stores infection flyweights and creates new instances
class infection_factory {

public:
    using human_flyweight  = human_infection_cycle::flyweight;
    using object_flyweight = object_infection_cycle::flyweight;
    using agent_id         = repast::AgentId;

    /// @brief Construct an infection factory, used to create infection cycles
    /// @param hf Human infection cycle flyweight
    /// @param of Object infection cycle flyweight
    infection_factory(const human_flyweight& hf, const object_flyweight& of)
        : _human_flyweight { hf }
        , _object_flyweight { of }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // HUMAN INFECTION CYCLE CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get a new human infection cycle
    /// @param id The agent id associated with this cycle
    /// @param is Initial stage of the cycle
    /// @param t The time of infection
    /// @return A human infection cycle object
    human_infection_cycle make_human_cycle(const agent_id&              id,
                                           human_infection_cycle::STAGE is) const
    {
        return human_infection_cycle { &_human_flyweight, id, is };
    }

    /// @brief Get a new human infection cycle
    /// @param id The agent id associated with this cycle
    /// @param is Initial stage of the cycle
    /// @param t The time of infection
    /// @return A human infection cycle object
    human_infection_cycle make_human_cycle(const agent_id&              id,
                                           human_infection_cycle::STAGE is,
                                           clock::date_t                t) const
    {
        return human_infection_cycle { &_human_flyweight, id, is, t };
    }

    /// @brief Construct a human infection with serialized data
    /// @param id The agent id associated with this cycle
    /// @param sp The serialized data
    /// @return A human infection cycle object
    human_infection_cycle make_human_cycle(const agent_id&                                  id,
                                           const human_infection_cycle::serialization_pack& sp) const
    {
        return human_infection_cycle { &_human_flyweight, id, sp };
    }

    ////////////////////////////////////////////////////////////////////////////
    // OBJECT INFECTION CYCLE CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct an object infection cycle
    /// @param id The agent id associated with this cycle/logic
    /// @param is Initial stage of the cycle
    /// @return An object infection cycle object
    object_infection_cycle make_object_cycle(const agent_id&               id,
                                             object_infection_cycle::STAGE is) const
    {
        return object_infection_cycle { &_object_flyweight, id, is };
    }

    /// @brief Get a new human infection cycle
    /// @param id The agent id associated with this cycle
    /// @param sp The serialized data
    /// @return An object infection cycle object
    object_infection_cycle make_object_cycle(const agent_id&                                   id,
                                             const object_infection_cycle::serialization_pack& sp) const
    {
        return object_infection_cycle { &_object_flyweight, id, sp };
    }

private:
    human_flyweight  _human_flyweight;
    object_flyweight _object_flyweight;
};

} // namespace sti
