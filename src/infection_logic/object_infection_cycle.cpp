#include <repast_hpc/Random.h>
#include <repast_hpc/Grid.h>
#include <repast_hpc/Moore2DGridQuery.h>
#include <repast_hpc/VN2DGridQuery.h>
#include <repast_hpc/Point.h>

#include "object_infection_cycle.hpp"
#include "../contagious_agent.hpp"

void sti::object_infection_cycle::tick()
{
    // If the object is clean, try to get infected
    if (_stage == STAGE::CLEAN) {
        const auto my_location = _flyweight->space->get_continuous_location(_id);
        const auto near_agents = _flyweight->space->agents_around(my_location, _flyweight->infect_distance);

        for (const auto& agent : near_agents) {
            // Get the chance of get infected
            const auto* infection = agent->get_infection_logic();
            const auto  p         = infection->get_probability(my_location);

            // *Roll the dice*
            const auto dice = static_cast<precission>(repast::Random::instance()->nextDouble());
            if (p > dice) {
                // Oh no, I got infected!
                _stage          = STAGE::INFECTED;

            }
        }
    }

    // If the object is already infected, do nothing
    else if (_stage == STAGE::INFECTED) {
        
    }
}
