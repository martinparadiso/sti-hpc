/// @file infection_logic/infection_cycle.hpp
/// @brief Base class for infection logics
#pragma once

#include <repast_hpc/AgentId.h>
#include <repast_hpc/Point.h>

namespace sti {

/// @brief Base class/interface for the different types of infections
class infection_cycle {

public:
    using precission = float;
    using distance_t = float;
    using agent_id   = repast::AgentId;
    using position_t = repast::Point<int>;

    infection_cycle()                       = default;
    infection_cycle(const infection_cycle&) = default;
    infection_cycle& operator=(const infection_cycle&) = default;

    infection_cycle(infection_cycle&&) = default;
    infection_cycle& operator=(infection_cycle&&) = default;

    virtual ~infection_cycle() = default;

    /// @brief Get the probability of infecting others
    /// @param distance The distance to the other agent, in meters
    /// @return A value between 0 and 1
    [[nodiscard]]
    virtual precission get_probability(const position_t& position) const = 0;

    /// @brief Run the infection algorithm, polling nearby agents trying to get infected
    virtual void tick() = 0;
}; // class infection_cycle

} // namespace sti