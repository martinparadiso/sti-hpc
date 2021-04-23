/// @file infection_logic/infection_cycle.hpp
/// @brief Base class for infection logics
#pragma once

#include <string>

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

    /// @brief Get the probability of contaminating an object
    /// @return A value in the range [0, 1)
    [[nodiscard]] virtual precission get_contamination_probability() const = 0;

    /// @brief Get the probability of infecting humans
    /// @param position The requesting agent position, to determine the distance
    /// @return A value in the range [0, 1)
    [[nodiscard]] virtual precission get_infect_probability(coordinates<double> position) const = 0;

    /// @brief Get an ID/string to identify the object in post-processing
    /// @return A string identifying the object
    virtual std::string get_id() const = 0;

    /// @brief Make the cycle interact with another cycle
    /// @param other A reference to the other cycle
    virtual void interact_with(const infection_cycle& other) = 0;

}; // class infection_cycle

} // namespace sti