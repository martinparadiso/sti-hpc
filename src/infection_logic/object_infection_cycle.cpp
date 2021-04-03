#include "object_infection_cycle.hpp"

#include <boost/json/object.hpp>
#include <repast_hpc/Random.h>
#include <repast_hpc/Grid.h>
#include <repast_hpc/Moore2DGridQuery.h>
#include <repast_hpc/VN2DGridQuery.h>
#include <repast_hpc/Point.h>
#include <sstream>

#include "infection_cycle.hpp"
#include "../contagious_agent.hpp"
#include "../json_serialization.hpp"

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct an empty object, flyweight is still needed
/// @param fw The flyweight containing shared data
/// @param type The object type, i.e. chair, bed
sti::object_infection_cycle::object_infection_cycle(flyweights_ptr fw)
    : _flyweights { fw }
    , _id {}
    , _object_type {  }
    , _stage { STAGE::CLEAN }
    , _infected_by {}
{
}

/// @brief Construct an object infection logic
/// @param id The id of the agent associated with this cycle
/// @param fw The object flyweight
/// @param type The object type, i.e. chair, bed
/// @param is The initial stage of the object
sti::object_infection_cycle::object_infection_cycle(
    flyweights_ptr     fw,
    const agent_id&    id,
    const object_type& type,
    STAGE              is)
    : _flyweights { fw }
    , _id { id }
    , _object_type{type}
    , _stage { is }
    , _infected_by {}
{
}

////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Get the AgentId associated with this cycle
/// @return A reference to the agent id
repast::AgentId& sti::object_infection_cycle::id()
{
    return _id;
}

/// @brief Get the AgentId associated with this cycle
/// @return A reference to the agent id
const repast::AgentId& sti::object_infection_cycle::id() const
{
    return _id;
}

/// @brief Clean the object, removing contamination and resetting the state
void sti::object_infection_cycle::clean()
{
    _stage = STAGE::CLEAN;
}

/// @brief Get the probability of infecting humans
/// @param position The requesting agent position, to determine the probability
/// @return A value in the range [0, 1)
sti::infection_cycle::precission sti::object_infection_cycle::get_infect_probability(coordinates<double> position) const
{
    // If the patient is overlaping with me, return the infection chance
    const auto my_position = _flyweights->at(_object_type).space->get_continuous_location(_id);
    const auto distance    = sq_distance(my_position, position);
    if (distance <= _flyweights->at(_object_type).object_radius) return _flyweights->at(_object_type).infect_chance;

    // Otherwise the patient if too far away, return 0
    return 0.0;
}

/// @brief Make the object interact with a person, this can contaminate it
/// @param human A reference to the human infection cycle interacting with this object
void sti::object_infection_cycle::interact_with(const human_infection_cycle* human)
{
    // If the object is already contaminated don't waste time
    if (_stage == STAGE::CONTAMINATED) return;

    // Generate a random number and compare with the contamination probability
    const auto random_number = repast::Random::instance()->nextDouble();
    if (random_number < human->get_contamination_probability()) {
        // The object got contaminated, change state and record the source
        _stage = STAGE::CONTAMINATED;
        _infected_by.push_back({ human->id(),
                                 _flyweights->at(_object_type).clock->now() });
    }
}

/// @brief Try to get infected with nearby/overlaping patients
void sti::object_infection_cycle::contaminate_with_nearby()
{
    // If the object is already dirty, nothing to do here
    if (_stage != STAGE::CLEAN) return;

    const auto my_location = _flyweights->at(_object_type).space->get_continuous_location(_id);
    const auto near_agents = _flyweights->at(_object_type).space->agents_around(my_location, _flyweights->at(_object_type).object_radius);

    for (const auto& agent : near_agents) {

        // Get the chance of getting infected by that agent
        const auto infect_probability = agent->get_infection_logic()->get_infect_probability(my_location);

        // *Roll the dice*
        const auto random_number = repast::Random::instance()->nextDouble();
        if (random_number < infect_probability) {
            // Got infected
            _stage = STAGE::CONTAMINATED;
            _infected_by.push_back({ agent->get_infection_logic()->id(),
                                     _flyweights->at(_object_type).clock->now() });

            // No need to keep iterating over the remaining agents
            break;
        }
    }
}

/// @brief Get statistics about the infection
/// @return A Boost.JSON value containing relevant statistics
boost::json::value sti::object_infection_cycle::stats() const
{

    auto to_string = [](STAGE stage) {
        switch (stage) {
        case STAGE::CLEAN:
            return "clean";
        case STAGE::CONTAMINATED:
            return "contaminated";
        }
    };

    return {
        { "stage", to_string(_stage) },
        { "infections", _infected_by }
    };
}