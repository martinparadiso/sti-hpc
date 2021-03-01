#include <repast_hpc/Random.h>
#include <repast_hpc/Grid.h>
#include <repast_hpc/Moore2DGridQuery.h>
#include <repast_hpc/VN2DGridQuery.h>
#include <repast_hpc/Point.h>

#include "human_infection_cycle.hpp"
#include "../contagious_agent.hpp"

// TODO: remove
#include <sstream>
#include "../print.hpp"

void sti::human_infection_cycle::tick()
{
    // If healthy try to get infected
    if (_stage == STAGE::HEALTHY) {
        const auto my_location = [&]() {
            auto vec = std::vector<int> {};
            _flyweight->repast_space->getLocation(_id, vec);
            return repast::Point<int>(vec);
        }();

        // Get nearby agents
        const auto near_agents = [&]() {
            auto                                    buf = std::vector<contagious_agent*> {};
            repast::VN2DGridQuery<contagious_agent> VN2DQuery(_flyweight->repast_space);
            VN2DQuery.query(my_location, // Search center
                            _flyweight->infect_distance, // Circle radius
                            true, // Include agents in the center
                            buf); // Output
            return buf;
        }();

        for (const auto& agent : near_agents) {
            // Get the chance of get infected
            const auto* infection = agent->get_infection_logic();
            const auto  p         = infection->get_probability(my_location);

            // *Roll the dice*
            const auto dice = static_cast<precission>(repast::Random::instance()->nextDouble());
            if (p > dice) {
                // Oh no, I got infected!
                _stage          = STAGE::INCUBATING;
                _infection_time = _flyweight->clk->now();

                // TODO: Remove this
                auto msg = std::stringstream{};
                msg << "["
                   << _id.id() << ","
                   << _id.startingRank() << ","
                   << _id.agentType() << ","
                   << _id.currentRank() << "] "
                   << "Infected";
                print(msg.str());

                // No need to keep iterating over the remaining agents
                break;

            }
        }
    }

    // If incubating, check if incubation period is over
    else if (_stage == STAGE::INCUBATING) {
        const auto time_since_infection = _flyweight->clk->now() - _infection_time;
        if (time_since_infection >= _flyweight->incubation_time) {
            _stage = STAGE::SICK;   
        }
    }

    // If sick, do nothing
    else if (_stage == STAGE::SICK) {
    }
}