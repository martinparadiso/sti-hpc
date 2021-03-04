/// @file infection_logic/infection_cycle.hpp
/// @brief Base class for infection logics
#pragma once

#include <queue>
#include <boost/variant.hpp>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/Point.h>

namespace sti {
class contagious_agent;

using variant     = boost::variant<bool, float, double, std::int32_t, std::uint32_t, std::uint64_t, std::int64_t, repast::AgentId>;
using serial_data = std::queue<variant>;

/// @brief Base class/interface for the different types of infections
class infection_cycle {

public:
    using precission = float;
    using distance_t = float;
    using agent_id   = repast::AgentId;
    using position_t = repast::Point<double>;

    infection_cycle()                       = default;
    infection_cycle(const infection_cycle&) = default;
    infection_cycle& operator=(const infection_cycle&) = default;

    infection_cycle(infection_cycle&&) = default;
    infection_cycle& operator=(infection_cycle&&) = default;

    virtual ~infection_cycle() = default;

    /// @brief Get the probability of infecting others
    /// @param distance The distance to the other agent, in meters
    /// @return A value between 0 and 1
    [[nodiscard]] virtual precission get_probability(const position_t& position) const = 0;

    /// @brief Run the infection algorithm, polling nearby agents trying to get infected
    virtual void tick() = 0;

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the internal state of the infection
    /// @param queue The queue to store the data
    virtual void serialize(serial_data& queue) const = 0;

    /// @brief Deserialize the data and update the agent data
    /// @param queue The queue containing the data
    virtual void deserialize(serial_data& queue) = 0;

}; // class infection_cycle

} // namespace sti