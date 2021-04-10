/// @file infection_logic/infection_factory.hpp
/// @brief Infection factory, to ease the creation of infection
#pragma once

#include <map>

#include "human_infection_cycle.hpp"
#include "object_infection_cycle.hpp"
#include "ghost_object_cycle.hpp"

// Fw. declarations
namespace boost {
namespace json {
    class object;
} // namespace json
} // namespace boost

namespace repast {
class AgentId;
}

namespace sti {
class parallel_agent;
class clock;
class space_wrapper;
} // namespace sti

namespace sti {

/// @brief Stores infection flyweights and creates new instances
class infection_factory {

public:
    using human_flyweight  = human_infection_cycle::flyweight;
    using object_flyweight = object_infection_cycle::flyweight;
    using agent_id         = repast::AgentId;

    /// @brief Store the type of object as a string, for instance: chair, bed.
    using object_type = object_infection_cycle::object_type;

    /// @brief Construct an infection factory, used to create infection cycles
    /// @param hospital_props The boost.JSON object containing the hospital paramters
    /// @param space The space wrapper
    /// @param clock The simulation clock
    infection_factory(const boost::json::object& hospital_props,
                      const space_wrapper*       space,
                      const clock*               clock);

    ////////////////////////////////////////////////////////////////////////////
    // HUMAN INFECTION CYCLE CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Default construct a human infection cycle
    /// @return A default constructed human infected cycle
    human_infection_cycle make_human_cycle() const;

    /// @brief Get a new human infection cycle
    /// @param id The agent id associated with this cycle
    /// @param is Initial stage of the cycle
    /// @param mode The "mode" of the cycle
    /// @param infection_time The time of infection
    /// @return A human infection cycle object
    human_infection_cycle make_human_cycle(const agent_id&              id,
                                           human_infection_cycle::STAGE is,
                                           human_infection_cycle::MODE  mode,
                                           datetime                     infection_time) const;

    ////////////////////////////////////////////////////////////////////////////
    // OBJECT INFECTION CYCLE CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct an empty object infection
    /// @return A default constructed object infection cycle
    object_infection_cycle make_object_cycle() const;

    // /// @brief Default construct an object infection
    // /// @param type The object type, normally 'chair' or 'bed'
    // /// @return A default constructed object infection cycle
    // object_infection_cycle make_object_cycle(const object_type& type) const;

    /// @brief Construct an object infection cycle
    /// @param type The object type, normally 'chair' or 'bed'
    /// @param id The agent id associated with this cycle/logic
    /// @param is Initial stage of the cycle
    /// @return An object infection cycle object
    object_infection_cycle make_object_cycle(const object_type&            type,
                                             const agent_id&               id,
                                             object_infection_cycle::STAGE is) const;

    ////////////////////////////////////////////////////////////////////////////
    // GHOST INFECTION CYCLE CREATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct an empty object infection
    /// @return A default constructed object infection cycle
    ghost_object_cycle make_ghost_object_cycle();

    /// @brief Construct an object infection cycle with no repast relationship
    /// @param type The object type, normally 'chair' or 'bed'
    /// @param is Initial stage of the cycle
    /// @return An object infection cycle object
    ghost_object_cycle make_ghost_object_cycle(const object_type&        type,
                                               ghost_object_cycle::STAGE is);

private:
    human_flyweight                         _human_flyweight;
    std::map<object_type, object_flyweight> _object_flyweights;

    unsigned _ghost_objects;
};

} // namespace sti
