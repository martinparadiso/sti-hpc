/// @file json_serialization.hpp
/// @brief Contains functions for serializing classes to JSON with Boost
#pragma once

#include <boost/json.hpp>
#include <repast_hpc/Point.h>
#include <repast_hpc/AgentId.h>
#include <sstream>
#include <string>

#include "clock.hpp"
#include "model.hpp"
#include "hospital_plan.hpp"
#include "contagious_agent.hpp"

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

template <typename T>
void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const coordinates<T>& c)
{
    jv = {
        { "x", c.x },
        { "y", c.y }
    };
}

inline void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const timedelta& td)
{
    jv = {
        { "time", td.length() }
    };
}

inline void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const datetime& td)
{
    jv                = {
        { "time", td.epoch() }
    };
}

inline void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const process_metrics::metrics& pm)
{
    jv = {
        { "current_agents", pm.current_agents },
        { "mpi_sync_ns", pm.mpi_sync_ns },
        { "rhpc_sync_ns", pm.rhpc_sync_ns },
        { "logic_ns", pm.logic_ns },
        { "tick_start_time", pm.tick_start_time }
    };
}

// From JSON

template <typename T>
coordinates<T> tag_invoke(const boost::json::value_to_tag<coordinates<T>>& /*unused*/, const boost::json::value& jv)
{
    const auto& obj = jv.as_object();
    return coordinates<T> {
        boost::json::value_to<T>(obj.at("x")),
        boost::json::value_to<T>(obj.at("y")),
    };
}

inline datetime tag_invoke(const boost::json::value_to_tag<datetime>& /*unused*/, const boost::json::value& jv)
{
    const auto& obj = jv.as_object();
    return datetime {
        static_cast<datetime::resolution>(boost::json::value_to<int>(obj.at("day"))),
        static_cast<datetime::resolution>(boost::json::value_to<int>(obj.at("hour"))),
        static_cast<datetime::resolution>(boost::json::value_to<int>(obj.at("minute"))),
        static_cast<datetime::resolution>(boost::json::value_to<int>(obj.at("second")))
    };
}

inline timedelta tag_invoke(const boost::json::value_to_tag<timedelta>& /*unused*/, const boost::json::value& jv)
{
    const auto& obj = jv.as_object();
    return timedelta {
        static_cast<datetime::resolution>(boost::json::value_to<int>(obj.at("days"))),
        static_cast<datetime::resolution>(boost::json::value_to<int>(obj.at("hours"))),
        static_cast<datetime::resolution>(boost::json::value_to<int>(obj.at("minutes"))),
        static_cast<datetime::resolution>(boost::json::value_to<int>(obj.at("seconds")))
    };
}

} // namespace sti

namespace repast {

inline void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const AgentId& id)
{
    auto os = std::ostringstream {};
    os << id;
    jv = {
        { "id", os.str() }
    };
}
}
