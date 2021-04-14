/// @brief Agent capable of infecting others
#pragma once

#include <boost/variant.hpp>
#include <boost/mpi/detail/binary_buffer_oprimitive.hpp>
#include <cstdint>
#include <memory>
#include <queue>
#include <repast_hpc/AgentId.h>
#include <string>
#include <utility>
#include <vector>

#include "clock.hpp"
#include "infection_logic/infection_factory.hpp"

// Fw. declarations
namespace boost {
namespace json {
    class object;
} // namespace json
namespace mpi {
    class communicator;
} // namespace mpi
} // namespace boost

namespace sti {

// The data is serialized into a buffer with Boost.MPI packed archives
using serial_data = boost::mpi::binary_buffer_oprimitive::buffer_type;

/// @brief An virtual class representing an agent capable of infecting others
class contagious_agent {

public:

    /// @brief The different type of contagious agents
    enum class type {
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

    contagious_agent(const contagious_agent&) = default;
    contagious_agent(contagious_agent&&)      = default;
    contagious_agent& operator=(const contagious_agent&) = default;
    contagious_agent& operator=(contagious_agent&&) = default;
    virtual ~contagious_agent()                     = default;

    ////////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the agent state into a string using Boost.Serialization
    /// @param communicator The MPI communicator over which this archive will be sent
    /// @return A string with the serialized data
    virtual serial_data serialize(boost::mpi::communicator* communicator) = 0;

    /// @brief Reconstruct the agent state from a string using Boost.Serialization
    /// @param id The new AgentId
    /// @param data The serialized data
    /// @param communicator The MPI communicator over which the archive was sent
    virtual void serialize(const id_t& id,
                           serial_data& data,
                           boost::mpi::communicator* communicator) = 0;

    ////////////////////////////////////////////////////////////////////////////////
    // REPAST REQUIRED METHODS
    ////////////////////////////////////////////////////////////////////////////////

    /// @brief Get the agent id
    /// @return The repast id
    repast::AgentId& getId()
    {
        return _id;
    }

    /// @brief Get a reference to the AgentID
    /// @note Needed to properly serialize it
    repast::AgentId& id_ref()
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
    virtual infection_cycle* get_infection_logic() = 0;

    /// @brief Get the infection logic
    /// @return A const pointer to the infection logic
    virtual const infection_cycle* get_infection_logic() const = 0;

    /// @brief Return the agent statistics as a json object
    /// @return A Boost.JSON object containing relevant statistics
    virtual boost::json::object stats() const = 0;

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

/// @brief Convert an agent enum to string
inline std::string to_string(contagious_agent::type type)
{
    using E = contagious_agent::type;

    switch (type) {
    case E::FIXED_PERSON:
        return "fixed_person";
    case E::OBJECT:
        return "object";
    case E::PATIENT:
        return "patient";
    }
}

} // namespace sti