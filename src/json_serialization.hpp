/// @file json_serialization.hpp
/// @brief Contains functions for serializing classes to JSON with Boost
#pragma once

#include <boost/json.hpp>
#include <repast_hpc/Point.h>
#include <sstream>

#include "clock.hpp"
#include "hospital_plan.hpp"

namespace sti {

// To JSON

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
    const auto& human = td.human();
    jv                = {
        { "days", human.days },
        { "hours", human.hours },
        { "minutes", human.minutes },
        { "seconds", human.seconds }
    };
}

inline void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const datetime& td)
{
    const auto& human = td.human();
    jv                = {
        { "day", human.days },
        { "hour", human.hours },
        { "minute", human.minutes },
        { "second", human.seconds }
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
