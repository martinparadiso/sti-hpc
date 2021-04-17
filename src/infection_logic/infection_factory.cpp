#include "infection_factory.hpp"

#include <boost/json/object.hpp>
#include <repast_hpc/RepastProcess.h>

#include "../json_serialization.hpp"
#include "../space_wrapper.hpp"
#include "../clock.hpp"
#include "ghost_object_cycle.hpp"
#include "human_infection_cycle.hpp"
#include "object_infection_cycle.hpp"

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct an infection factory, used to create infection cycles
/// @param hospital_params The boost.JSON object containing the hospital paramters
/// @param space The space wrapper
/// @param clock The simulation clock
sti::infection_factory::infection_factory(const boost::json::object& hospital_props,
                                          const space_wrapper*       space,
                                          const clock*               clock)
    : _human_flyweight {
        space,
        clock,
        hospital_props.at("parameters").at("human").at("infect_probability").as_double(),
        hospital_props.at("parameters").at("human").at("infect_distance").as_double(),
        hospital_props.at("parameters").at("human").at("contamination_probability").as_double(),
        boost::json::value_to<sti::timedelta>(hospital_props.at("parameters").at("human").at("incubation_time"))
    }
    , _object_flyweights { [&]() {
        auto map = decltype(_object_flyweights) {};

        const auto object_types = hospital_props.at("parameters").at("objects").as_object();
        for (const auto& [object_name, value] : object_types) {
            map[object_name.to_string()] = {
                space,
                clock,
                value.at("infect_probability").as_double(),
                value.at("radius").as_double()
            };
        }
        return map;
    }() }
    , _ghost_objects {0}
{
}

////////////////////////////////////////////////////////////////////////////
// HUMAN INFECTION CYCLE CREATION
////////////////////////////////////////////////////////////////////////////

/// @brief Default construct a human infection cycle
/// @return A default constructed human infected cycle
sti::human_infection_cycle sti::infection_factory::make_human_cycle() const
{
    return { &_human_flyweight };
}

/// @brief Get a new human infection cycle
/// @param id The agent id associated with this cycle
/// @param is Initial stage of the cycle
/// @param mode The "mode" of the cycle
/// @param infection_time The time of infection
/// @return A human infection cycle object
sti::human_infection_cycle sti::infection_factory::make_human_cycle(
    const agent_id&              id,
    human_infection_cycle::STAGE is,
    human_infection_cycle::MODE  mode,
    datetime                     infection_time) const
{
    return { &_human_flyweight, id, is, mode, infection_time };
}

////////////////////////////////////////////////////////////////////////////
// OBJECT INFECTION CYCLE CREATION
////////////////////////////////////////////////////////////////////////////

/// @brief Default construct an object infection
/// @return A default constructed object infection cycle
sti::object_infection_cycle sti::infection_factory::make_object_cycle() const
{
    return { &_object_flyweights };
}

/// @brief Construct an object infection cycle
/// @param type The object type, normally 'chair' or 'bed'
/// @param id The agent id associated with this cycle/logic
/// @param is Initial stage of the cycle
/// @return An object infection cycle object
sti::object_infection_cycle sti::infection_factory::make_object_cycle(
    const object_type&            type,
    const agent_id&               id,
    object_infection_cycle::STAGE is) const
{
    return { &_object_flyweights, id, type, is };
}

////////////////////////////////////////////////////////////////////////////
// GHOST INFECTION CYCLE CREATION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct an empty object infection
/// @return A default constructed object infection cycle
sti::ghost_object_cycle sti::infection_factory::make_ghost_object_cycle()
{
    return { &_object_flyweights };
}

/// @brief Construct an object infection cycle with no repast relationship
/// @param type The object type, normally 'chair' or 'bed'
/// @param is Initial stage of the cycle
/// @return An object infection cycle object
sti::ghost_object_cycle sti::infection_factory::make_ghost_object_cycle(
    const object_type&        type,
    ghost_object_cycle::STAGE is)
{
    const auto id = ghost_object_cycle::id_type{repast::RepastProcess::instance()->rank(), _ghost_objects++};
    return { &_object_flyweights, id, type, is};
}