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
#include "../space_wrapper.hpp"

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct an empty human cycle with no internal state
sti::human_infection_cycle::human_infection_cycle(flyweight_ptr fw, environment_ptr env)
    : _flyweight { fw }
    , _environment { env }
    , _stage { STAGE::HEALTHY }
    , _mode { MODE::NORMAL }
    , _infect_location {}
{
}

/// @brief Construct a cycle starting in a given state, specifing the time of infection
/// @param id The id of the agent associated with this patient
/// @param fw The flyweight containing the shared attributes
/// @param stage The initial stage
/// @param mode The "mode"
/// @param infection_time The time of infection
/// @param env The infection environment this human resides in
sti::human_infection_cycle::human_infection_cycle(flyweight_ptr          fw,
                                                  const repast::AgentId& id,
                                                  STAGE                  stage,
                                                  MODE                   mode,
                                                  datetime               infection_time,
                                                  environment_ptr        env)
    : _flyweight { fw }
    , _environment { env }
    , _id { id }
    , _stage { stage }
    , _mode { mode }
    , _infection_time { infection_time }
    , _infect_location {}
{
}

////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Change the infection mode
/// @param new_mode The new mode to use in this infection
void sti::human_infection_cycle::mode(MODE new_mode)
{
    _mode = new_mode;
}

/// @brief Get an ID/string to identify the object in post-processing
/// @return A string identifying the object
std::string sti::human_infection_cycle::get_id() const
{
    return "human." + std::to_string(_id.id()) + "." + std::to_string(_id.startingRank()) + "." + std::to_string(_id.agentType());
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
    if (_stage == STAGE::HEALTHY) return 0.0;
    if (_mode == MODE::IMMUNE) return 0.0;

    return _flyweight->contamination_probability;
}

/// @brief Get the probability of infecting humans
/// @param position The requesting agent position, to determine the distance
/// @return A value in the range [0, 1)
sti::infection_cycle::precission sti::human_infection_cycle::get_infect_probability(coordinates<double> position) const
{
    if (_stage == STAGE::HEALTHY) return 0.0; // If the human is healthy, can't infect
    if (_mode == MODE::IMMUNE) return 0.0; // If the human is immune, can't infect
    if (_mode == MODE::COMA) return 0.0; // If the human is in coma, can't infect

    // Otherwise calculate the distance to the other person
    const auto my_position = _flyweight->space->get_continuous_location(_id);
    const auto distance    = sq_distance(my_position, position);

    if (distance > _flyweight->infect_distance) return 0.0;

    // If the other agent is less that X meters away, and this one is either
    // sick or incubating, return the standard infect probability
    return _flyweight->infect_probability;
}

/// @brief Make the human interact with another infection logic
/// @param other The other infection cycle
void sti::human_infection_cycle::interact_with(const infection_cycle& other)
{
    // Get the chance of getting infected by that agent
    const auto my_location        = _flyweight->space->get_continuous_location(_id);
    const auto infect_probability = other.get_infect_probability(my_location);

    // *Roll the dice*
    const auto random_number = repast::Random::instance()->nextDouble();
    if (random_number < infect_probability) {
        // Got infected
        infected(other.get_id());
    }
}

/// @brief Try to get infected via the environment
void sti::human_infection_cycle::infect_with_environment()
{
    if (_environment == nullptr) return; // If there is no environment, return
    if (_stage != STAGE::HEALTHY) return; // If the agent is already infected, return
    if (_mode == MODE::IMMUNE) return; // If the agent is immune, return

    // Otherwise generate a random number and compare with the environment
    // probability of getting infected
    const auto random_number = repast::Random::instance()->nextDouble();
    if (random_number < _environment->get_probability()) {
        // The agent got infected, store the name
        infected(_environment->name());
    }
}

/// @brief Try go get infected with nearby agents
void sti::human_infection_cycle::infect_with_nearby()
{
    if (_stage != STAGE::HEALTHY) return; // If the human is already infected do nothing
    if (_mode == MODE::IMMUNE) return; // If the human is immune, can't infect
    if (_mode == MODE::COMA) return; // If the human is in coma, can't infect

    const auto my_location = _flyweight->space->get_continuous_location(_id);
    const auto near_agents = _flyweight->space->agents_around(my_location, _flyweight->infect_distance);

    for (const auto& agent : near_agents) {

        this->interact_with(*agent->get_infection_logic());
        // If got infected No need to keep iterating over the remaining agents
        if (_stage != STAGE::HEALTHY) break;
    }
}

/// @brief The infection has time-based stages, this method performs the changes
void sti::human_infection_cycle::tick()
{
    // The infection cycle must go from INCUBATING to SICK if the correpsonding
    // time has elapsed
    if (_stage == STAGE::INCUBATING) {
        if (_incubation_end < _flyweight->clk->now()) {
            _stage = STAGE::SICK;
        }
    }

    this->infect_with_environment();
    this->infect_with_nearby();
}

////////////////////////////////////////////////////////////////////////////
// DATA COLLECTION
////////////////////////////////////////////////////////////////////////////

/// @brief Get statistics about the infection
/// @return A Boost.JSON value containing relevant statistics
boost::json::value sti::human_infection_cycle::stats() const
{
    auto stos = [](STAGE stage) {
        switch (stage) {
        case STAGE::HEALTHY:
            return "healthy";
        case STAGE::INCUBATING:
            return "incubating";
        case STAGE::SICK:
            return "sick";
        }
    };

    auto mtos = [](MODE mode) {
        switch (mode) {
        case MODE::NORMAL:
            return "normal";
        case MODE::IMMUNE:
            return "immune";
        case MODE::COMA:
            return "coma";
        }
    };

    return {
        { "infection_id", get_id() },
        { "infection_model", "human" },
        { "infection_mode", mtos(_mode) },
        { "infection_stage", stos(_stage) },
        { "infection_time", _infection_time.seconds_since_epoch() },
        { "incubation_end", _incubation_end.seconds_since_epoch() },
        { "infected_by", _infected_by },
        { "infect_location", _infect_location }
    };
}

/// @brief Indicate that the patient has been infected
/// @param infected_by Who infected the agent
void sti::human_infection_cycle::infected(const std::string& infected_by)
{

    _stage = STAGE::INCUBATING;
    // The infection time is a uniform distribution in the range [min, max],
    // generate a random integer in the range [0, max - min], then offset it
    const auto range           = _flyweight->max_incubation_time.length() - _flyweight->min_incubation_time.length();
    const auto random          = static_cast<timedelta::resolution>(repast::Random::instance()->nextDouble() * static_cast<double>(range));
    const auto incubation_time = timedelta { _flyweight->min_incubation_time.length() + random };
    _infection_time            = _flyweight->clk->now();
    _incubation_end            = _infection_time + incubation_time;
    _infected_by               = infected_by;
    _infect_location           = _flyweight->space->get_discrete_location(_id);
}