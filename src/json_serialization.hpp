/// @file json_serialization.hpp
/// @brief Contains functions for serializing classes to JSON with Boost
#pragma once

#include <boost/json.hpp>
#include <repast_hpc/AgentId.h>
#include <string>

namespace sti {

////////////////////////////////////////////////////////////////////////////////
// TO STRING
////////////////////////////////////////////////////////////////////////////////

inline std::string to_string(const repast::AgentId& id)
{
    return std::to_string(id.id()) + "." + std::to_string(id.startingRank()) + "." + std::to_string(id.agentType());
}

////////////////////////////////////////////////////////////////////////////////
// TO JSON
////////////////////////////////////////////////////////////////////////////////

} // namespace sti

namespace repast {

inline void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const AgentId& id)
{
    jv = {
        { "repast_id", sti::to_string(id) }
    };
}
}
