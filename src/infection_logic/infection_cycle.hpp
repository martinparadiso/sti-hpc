/// @file infection_logic/infection_cycle.hpp
/// @brief Base class for infection logics
#pragma once

// Fw. declarations
namespace repast {
class AgentId;
} // namespace repast

namespace sti {
template <typename T>
struct coordinates;

} // namespace sti

namespace sti {
class contagious_agent;

/// @brief Base class/interface for the different types of infections
class infection_cycle {

public:
    using precission = double;
    using distance_t = double;
    using agent_id   = repast::AgentId;
    using position_t = sti::coordinates<double>;

    infection_cycle()                       = default;
    infection_cycle(const infection_cycle&) = default;
    infection_cycle& operator=(const infection_cycle&) = default;

    infection_cycle(infection_cycle&&) = default;
    infection_cycle& operator=(infection_cycle&&) = default;

    virtual ~infection_cycle() = default;

    /// @brief Get the probability of infecting humans
    /// @param position The requesting agent position, to determine the distance
    /// @return A value in the range [0, 1)
    [[nodiscard]] virtual precission get_infect_probability(coordinates<double> position) const = 0;

    /// @brief Get the AgentId associated with this cycle
    /// @return A reference to the agent id
    virtual repast::AgentId& id() = 0;

    /// @brief Get the AgentId associated with this cycle
    /// @return A reference to the agent id
    virtual const repast::AgentId& id() const = 0;

}; // class infection_cycle

} // namespace sti