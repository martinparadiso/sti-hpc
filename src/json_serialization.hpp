/// @file json_serialization.hpp
/// @brief Contains functions for serializing classes to JSON with Boost
#pragma once

#include <boost/json/detail/value_from.hpp>
#include <boost/json/value.hpp>
#include <repast_hpc/Point.h>
#include <sstream>

#include "clock.hpp"
#include "hospital_plan.hpp"

namespace sti {

void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const coordinates& c)
{
    jv = {
        { "x", c.x },
        { "y", c.y }
    };
}

void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const point& p)
{
    jv = {
        { "x", p.x },
        { "y", p.y }
    };
}

void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const timedelta& td)
{
    const auto& human = td.human();
    auto        ss    = std::ostringstream {};
    ss << std::setfill('0') << std::setw(2)
       << human.hours << ":"
       << std::setfill('0') << std::setw(2)
       << human.minutes << ":"
       << std::setfill('0') << std::setw(2)
       << human.seconds;
    jv = {
        { "day", human.days },
        { "time", ss.str() }
    };
}

void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const datetime& td)
{
    const auto& human = td.human();
    auto        ss    = std::ostringstream {};
    ss << std::setfill('0') << std::setw(2)
       << human.hours << ":"
       << std::setfill('0') << std::setw(2)
       << human.minutes << ":"
       << std::setfill('0') << std::setw(2)
       << human.seconds;
    jv = {
        { "day", human.days },
        { "time", ss.str() }
    };
}

} // namespace sti
