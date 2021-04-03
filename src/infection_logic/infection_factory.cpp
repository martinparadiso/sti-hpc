#include "infection_factory.hpp"

#include <boost/json/object.hpp>

#include "../json_serialization.hpp"
#include "../space_wrapper.hpp"
#include "../clock.hpp"
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
                value.at("infect_chance").as_double(),
                value.at("radius").as_double()
            };
        }
        return map;
    }() }
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
/// @param t The time of infection
/// @return A human infection cycle object
sti::human_infection_cycle sti::infection_factory::make_human_cycle(
    const agent_id&              id,
    human_infection_cycle::STAGE is,
    datetime                     infection_time) const
{
    return { &_human_flyweight, id, is, infection_time };
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

/// @brief Default construct an object infection
/// @param type The object type, normally 'chair' or 'bed'
/// @return A default constructed object infection cycle
// sti::object_infection_cycle sti::infection_factory::make_object_cycle(const object_type& type) const
// {
//     return { &_object_flyweights, type };
// }

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
