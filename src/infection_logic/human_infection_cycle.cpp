#include "human_infection_cycle.hpp"

#include <boost/json.hpp>
#include <repast_hpc/Random.h>
#include <repast_hpc/Grid.h>
#include <repast_hpc/Moore2DGridQuery.h>
#include <repast_hpc/VN2DGridQuery.h>
#include <repast_hpc/Point.h>
#include <sstream>

#include "../contagious_agent.hpp"
#include "environment.hpp"
#include "../json_serialization.hpp"

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct an empty human cycle with no internal state
sti::human_infection_cycle::human_infection_cycle(flyweight_ptr fw, environment_ptr env)
    : _flyweight { fw }
    , _environment { env }
    , _stage { STAGE::HEALTHY }
    , _infection_time {}
    , _infected_by {}
{
}

/// @brief Construct a cycle starting in a given state, specifing the time of infection
/// @param id The id of the agent associated with this patient
/// @param fw The flyweight containing the shared attributes
/// @param stage The initial stage
/// @param infection_time The time of infection
/// @param env The infection environment this human resides in
sti::human_infection_cycle::human_infection_cycle(flyweight_ptr          fw,
                                                  const repast::AgentId& id,
                                                  STAGE                  stage,
                                                  datetime               infection_time,
                                                  environment_ptr        env)
    : _flyweight { fw }
    , _environment { env }
    , _id { id }
    , _stage { stage }
    , _infection_time { infection_time }
    , _infected_by {}
{
}

////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Get the AgentId associated with this cycle
/// @return A reference to the agent id
repast::AgentId& sti::human_infection_cycle::id()
{
    return _id;
}

/// @brief Get the AgentId associated with this cycle
/// @return A reference to the agent id
const repast::AgentId& sti::human_infection_cycle::id() const
{
    return _id;
}

/// @brief Set the infection environment this human resides
/// @param env_ptr A pointer to the environment
void sti::human_infection_cycle::set_environment(const infection_environment* env_ptr)
{
    _environment = env_ptr;
}

/// @brief Get the probability of contaminating an object
/// @return A value in the range [0, 1)
sti::infection_cycle::precission sti::human_infection_cycle::get_contamination_probability() const
{
    return _flyweight->contamination_probability;
}

/// @brief Get the probability of infecting humans
/// @param position The requesting agent position, to determine the distance
/// @return A value in the range [0, 1)
sti::infection_cycle::precission sti::human_infection_cycle::get_infect_probability(coordinates<double> position) const
{
    // If I'm healthy, the probability is 0
    if (_stage == STAGE::HEALTHY) return 0.0;

    // Calculate the distance to the other person
    const auto my_position = _flyweight->space->get_continuous_location(_id);
    const auto distance    = sq_distance(my_position, position);

    if (distance > _flyweight->infect_distance) return 0.0;

    // If the other agent is less that X meters away, and this one is either
    // sick or incubating, return the standard infect probability
    return _flyweight->infect_probability;
}

/// @brief Try to get infected via the environment
void sti::human_infection_cycle::infect_with_environment()
{
    // If there is no environment, return
    if (_environment == nullptr) return;

    // If the agent is already infected, return
    if (_stage != STAGE::HEALTHY) return;

    // Otherwise generate a random number and compare with the environment
    // probability of getting infected
    const auto random_number = repast::Random::instance()->nextDouble();
    if (random_number < _environment->get_probability()) {
        // The agent got infected, store the name
        _stage          = STAGE::INCUBATING;
        _infection_time = _flyweight->clk->now();
        _infected_by    = _environment->name();
    }
}

/// @brief Try go get infected with nearby agents
void sti::human_infection_cycle::infect_with_nearby()
{
    // If the human is already infected do nothing
    if (_stage != STAGE::HEALTHY) return;

    const auto my_location = _flyweight->space->get_continuous_location(_id);
    const auto near_agents = _flyweight->space->agents_around(my_location, _flyweight->infect_distance);

    for (const auto& agent : near_agents) {

        // Get the chance of getting infected by that agent
        const auto infect_probability = agent->get_infection_logic()->get_infect_probability(my_location);

        // *Roll the dice*
        const auto random_number = repast::Random::instance()->nextDouble();
        if (random_number < infect_probability) {
            // Got infected
            _stage          = STAGE::INCUBATING;
            _infection_time = _flyweight->clk->now();
            _infected_by = to_string(agent->getId());

            // No need to keep iterating over the remaining agents
            break;
        }
    }
}

/// @brief The infection has time-based stages, this method performs the changes
void sti::human_infection_cycle::tick()
{
    // The infection cycle must go from INCUBATING to SICK if the correpsonding
    // time has elapsed
    if (_stage == STAGE::INCUBATING) {
        if (_infection_time + _flyweight->incubation_time < _flyweight->clk->now()) {
            _stage = STAGE::SICK;
        }
    }
}

////////////////////////////////////////////////////////////////////////////
// DATA COLLECTION
////////////////////////////////////////////////////////////////////////////

/// @brief Get statistics about the infection
/// @return A Boost.JSON value containing relevant statistics
boost::json::value sti::human_infection_cycle::stats() const
{
    auto to_string = [](STAGE stage) {
        switch (stage) {
        case STAGE::HEALTHY:
            return "healthy";
        case STAGE::INCUBATING:
            return "incubating";
        case STAGE::SICK:
            return "sick";
        }
    };

    return {
        { "stage", to_string(_stage) },
        { "infection_time", _infection_time.epoch() },
        { "infected_by", _infected_by }
    };
}