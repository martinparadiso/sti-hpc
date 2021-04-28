#include "object_infection.hpp"

#include <boost/json/object.hpp>
#include <repast_hpc/Random.h>
#include <repast_hpc/Grid.h>
#include <repast_hpc/Moore2DGridQuery.h>
#include <repast_hpc/VN2DGridQuery.h>
#include <repast_hpc/Point.h>
#include <sstream>
#include <string>

#include "infection_cycle.hpp"
#include "../contagious_agent.hpp"
#include "../json_serialization.hpp"

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct an empty object, flyweight is still needed
/// @param fw The flyweight containing shared data
/// @param type The object type, i.e. chair, bed
sti::object_infection::object_infection(flyweights_ptr fw)
    : _flyweights { fw }
    , _stage { STAGE::CLEAN }
{
}

/// @brief Construct an object infection logic
/// @param fw The object flyweight
/// @param uint_id Unsigned int giving the agent an id
/// @param type The object type, i.e. chair, bed
/// @param is The initial stage of the object
sti::object_infection::object_infection(
    flyweights_ptr     fw,
    id_type            id,
    const object_type& type,
    STAGE              is)
    : _flyweights { fw }
    , _id { id }
    , _object_type { type }
    , _stage { is }
    , _next_clean { fw->at(_object_type).clock->now() + fw->at(_object_type).cleaning_interval }
{
}

////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Get an ID/string to identify the object in post-processing
/// @return A string identifying the object
std::string sti::object_infection::get_id() const
{
    return _object_type + '.' + std::to_string(_id.first) + '.' + std::to_string(_id.second);
}

/// @brief Clean the object, removing contamination and resetting the state
void sti::object_infection::clean()
{
    _stage = STAGE::CLEAN;
}

/// @brief Get the probability of contaminating an object
/// @return A value in the range [0, 1)
sti::infection_cycle::precission sti::object_infection::get_contamination_probability() const
{
    // An object can't contaminate other objects
    return 0.0;
}

/// @brief Get the probability of infecting humans
/// @param position The requesting agent position, to determine the probability
/// @return A value in the range [0, 1)
sti::infection_cycle::precission sti::object_infection::get_infect_probability(coordinates<double> /*unused*/) const
{
    return _flyweights->at(_object_type).infect_chance;
}

/// @brief Make the object interact with another infectious cycle
/// @details Note that this can infect/contaminate both agents depending on
/// the current status of each  cycle
/// @param human A reference to the human infection interacting with this object
void sti::object_infection::interact_with(const infection_cycle& other)
{
    // If the object is already contaminated do nothing
    if (_stage == STAGE::CONTAMINATED) return;

    // Generate a random number and compare with the contamination
    // probability of the other cycle
    const auto random_number = repast::Random::instance()->nextDouble();
    if (random_number < other.get_contamination_probability()) {
        // The object got contaminated, change state and record the source
        _stage = STAGE::CONTAMINATED;
        _infected_by.push_back({ other.get_id(),
                                 _flyweights->at(_object_type).clock->now() });
    }
}

/// @brief Perform the periodic logic, i.e. clean the object
void sti::object_infection::tick()
{
    if (_next_clean <= _flyweights->at(_object_type).clock->now()) {
        clean();
        _next_clean = _next_clean + _flyweights->at(_object_type).cleaning_interval;
    }
}

/// @brief Get statistics about the infection
/// @return A Boost.JSON value containing relevant statistics
boost::json::value sti::object_infection::stats() const
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
        { "infection_id", get_id() },
        { "infection_model", "object" },
        { "infection_stage", to_string(_stage) },
        { "infections", _infected_by }
    };
}