/// @brief Agent capable of infecting others
#pragma once

#include <cstdint>
#include <queue>

#include <boost/variant.hpp>
#include <repast_hpc/AgentId.h>
#include <string>
#include <utility>

#include "clock.hpp"
#include "infection_logic/infection_factory.hpp"

namespace sti {

using variant     = boost::variant<bool, float, double, std::int32_t, std::uint32_t, std::uint64_t, std::int64_t, repast::AgentId>;
using serial_data = std::queue<variant>;

/// @brief An virtual class representing an agent capable of infecting others
class contagious_agent {

public:
    contagious_agent()                        = default;
    contagious_agent(const contagious_agent&) = default;
    contagious_agent(contagious_agent&&)      = default;
    contagious_agent& operator=(const contagious_agent&) = default;
    contagious_agent& operator=(contagious_agent&&) = default;
    virtual ~contagious_agent()                     = default;

    /// @brief The different type of contagious agents
    enum class type {
        CHAIR,
        FIXED_PERSON,
        OBJECT,
        PATIENT,
    };

    using id_t = repast::AgentId;

    ////////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////////

    contagious_agent(const id_t& id)
        : _id { id }
    {
    }

    ////////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the internal state of the infection
    /// @param queue The queue to store the data
    virtual void serialize(serial_data& queue) const = 0;

    /// @brief Deserialize the data and update the agent data
    /// @param queue The queue containing the data
    virtual void deserialize(serial_data& queue) = 0;

    ////////////////////////////////////////////////////////////////////////////////
    // REPAST REQUIRED METHODS
    ////////////////////////////////////////////////////////////////////////////////

    /// @brief Get the agent id
    /// @return The repast id
    repast::AgentId& getId()
    {
        return _id;
    }

    /// @brief Get the agent id
    /// @return The repast id
    const repast::AgentId& getId() const
    {
        return _id;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////////

    /// @brief Update the id
    /// @param id The id
    void id(const id_t& id)
    {
        _id = id;
    }

    /// @brief Get the type of this agent
    /// @return The type of the agent
    virtual type get_type() const = 0;

    /// @brief Perform the actions this agent is supposed to
    virtual void act() = 0;

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    virtual const infection_cycle* get_infection_logic() const = 0;

    /// @brief Remove the agent from the simulation and collect the relevant data
    /// @return A list of key-value pairs, representing output relevant data
    virtual std::vector<std::pair<std::string, std::string>> kill_and_collect() = 0;

private:
    id_t _id;
};

////////////////////////////////////////////////////////////////////////////////
// INT <-> ENUM
////////////////////////////////////////////////////////////////////////////////

/// @brief Error trying to convert from int to enum
struct unknown_agent_type : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: The int value does not correspond to any known agent";
    }
};

/// @brief Vector containing all the agent types as enum
/// @details Repast IDs have a type attribute, which is encoded as an int. To
///          avoid using an int as a type identifier, an enum is used and casted
///          to/from int when interfacing with repast.
inline const auto agent_type_lut = std::vector {
    contagious_agent::type::CHAIR,
    contagious_agent::type::FIXED_PERSON,
    contagious_agent::type::OBJECT,
    contagious_agent::type::PATIENT
};

/// @brief Convert an agent enum to int
/// @return The int corresponding
inline int to_int(contagious_agent::type type)
{
    int i = 0;
    for (auto t : agent_type_lut) {
        if (t == type) return i;
        ++i;
    }
    throw unknown_agent_type {};
}

/// @brief Convert an int to a agent enum
inline contagious_agent::type to_agent_enum(int i)
{
    const auto index = static_cast<decltype(agent_type_lut)::size_type>(i);

    if (index < agent_type_lut.size()) {
        return agent_type_lut.at(index);
    }

    throw unknown_agent_type {};
}

} // namespace sti