#include "exit.hpp"

#include <boost/json.hpp>
#include <repast_hpc/AgentId.h>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "contagious_agent.hpp"
#include "print.hpp"
#include "space_wrapper.hpp"

// TODO: Proper output
#include <iostream>

/// @brief Pointer to implementation struct
struct sti::hospital_exit::impl {
    boost::json::object agent_output_data;
};

////////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////////

/// @brief Construct the hospital exit
/// @param context Repast context
/// @param space Space wrapper
/// @param clk The world clock
/// @param location The tile the exit is located
sti::hospital_exit::hospital_exit(repast_context_ptr context,
                                  space_ptr          space,
                                  clock_ptr          clk,
                                  sti::coordinates   location)

    : _context { context }
    , _space { space }
    , _clock { clk }
    , _location { location }
    , _pimpl { std::make_unique<sti::hospital_exit::impl>() }
{
}

sti::hospital_exit::~hospital_exit() = default;

////////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////////

/// @brief Perform all the exit actions, must be called once by tick
void sti::hospital_exit::tick()
{
    // Check if there are any agents in this location
    const auto agents = _space->agents_in_cell({_location.x, _location.y});

    // Remove all the agents in the exit position
    for (const auto& agent : agents) {
        const auto& id = agent->getId();
        const auto& data = agent->kill_and_collect();
        auto os = std::ostringstream{};
        os << id;
        _pimpl->agent_output_data[os.str()] = data;
    }
}

/// @brief Perform all the finishing actions, saving data and such
std::string sti::hospital_exit::finish() {
    // Print all the agent data
    auto os = std::ostringstream{};
    os << _pimpl->agent_output_data;
    return os.str();
}
