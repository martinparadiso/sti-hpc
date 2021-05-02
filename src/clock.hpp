/// @brief Clock class, to simulate passsage of time
#pragma once

#include <boost/json.hpp>
#include <boost/serialization/access.hpp>
#include <cstdint>
#include <string>

namespace sti {

// Fw. declarations
class clock;

/// @brief Date-time abstraction, represents a delta of time
class timedelta {

public:
    /// @brief A struct containing the date in human format, with seconds resolution
    struct human_date {
        std::uint32_t days;
        std::uint32_t hours;
        std::uint32_t minutes;
        std::uint32_t seconds;
    };

    using resolution = std::uint32_t;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a timedelta object
    /// @param length The number of seconds this delta spans
    constexpr explicit timedelta(resolution length = 0)
        : _length { length }
    {
    }

    /// @brief Create a timedelta object
    /// @param days The number of days
    /// @param hours The number of hours
    /// @param minutes The number of minutes
    /// @param seconds The number of seconds
    constexpr timedelta(resolution days, resolution hours, resolution minutes, resolution seconds)
        : _length { days * 86400 + hours * 3600 + minutes * 60 + seconds }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the length of this timedelta, in seconds
    /// @return The number of seconds
    constexpr resolution length() const
    {
        return _length;
    }

    /// @brief Get the date in human format
    /// @details A struct containing the date in a human redeable format
    constexpr human_date human() const
    {
        auto seconds = _length;

        auto minutes = seconds / 60;
        seconds      = seconds % 60;

        auto hours = minutes / 60;
        minutes    = minutes % 60;

        auto days = hours / 24;
        hours     = hours % 24;

        return { days, hours, minutes, seconds };
    }

    /// @brief Get the date in string format
    /// @details The format returned is [Day, Hours/Minutes/Seconds]
    std::string str() const;

private:
    friend class boost::serialization::access;

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _length;
    }

    resolution _length;
};

/// @brief An instant of time, counting from the start of the simulation
class datetime {

public:
    using resolution = timedelta::resolution;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a datetime object
    /// @param seconds The number of seconds passed since the start of the simulation
    constexpr explicit datetime(resolution seconds = 0)
        : _timedelta(seconds)
    {
    }

    /// @brief Create a timedelta object
    /// @param days The number of days
    /// @param hours The number of hours
    /// @param minutes The number of minutes
    /// @param seconds The number of seconds
    constexpr datetime(resolution days, resolution hours, resolution minutes, resolution seconds)
        : _timedelta { days, hours, minutes, seconds }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the length of this timedelta, in seconds
    /// @return The number of seconds
    constexpr auto seconds_since_epoch() const
    {
        return _timedelta.length();
    }

    /// @brief Get the date in human format
    /// @details A struct containing the date in a human redeable format
    constexpr timedelta::human_date human() const
    {
        return _timedelta.human();
    }

    /// @brief Get the date in string format
    /// @details The format returned is [Day, Hours/Minutes/Seconds]
    std::string str() const;

private:
    friend class boost::serialization::access;

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _timedelta;
    }

    timedelta _timedelta;
};

/// @brief A clock that encapsulates repast time
class clock {

public:
    /// @brief Create a new clock, starting the count in this instant
    /// @details The time is measured with the repast tick system. Using a
    ///          constant to convert from ticks to seconds.
    constexpr clock(timedelta::resolution seconds_per_tick)
        : _tick { 0 }
        , _seconds_per_tick { seconds_per_tick }
    {
    }

    /// @brief Adjust time, must be executed every tick
    /// @param tick The current tick
    void sync(double tick);

    /// @brief Get the time inside the simulation
    /// @return A datetime object with this instant of time
    datetime now() const
    {
        return datetime { static_cast<timedelta::resolution>(_tick) * _seconds_per_tick };
    }

    /// @brief Get the 'length' of a tick, in seconds
    /// @return The span of a tick, in seconds
    auto seconds_per_tick() const
    {
        return _seconds_per_tick;
    }

private:
    double                      _tick;
    const timedelta::resolution _seconds_per_tick;
};

////////////////////////////////////////////////////////////////////////////////
// OPERATORS
////////////////////////////////////////////////////////////////////////////////

/// @brief Compare two time peridos
/// @return True if right time is longer, false otherwise
constexpr bool operator<(const timedelta& lho, const timedelta& rho)
{
    return lho.length() < rho.length();
}

/// @brief Add two timedeltas
constexpr timedelta operator+(const timedelta& lho, const timedelta& rho)
{
    return timedelta { lho.length() + rho.length() };
}

/// @brief Add a timedelta to a datetime
constexpr datetime operator+(const datetime& lho, const timedelta& rho)
{
    return datetime { lho.seconds_since_epoch() + rho.length() };
}

/// @brief Compare two instants of time
/// @return True if right time is older, false otherwise
constexpr bool operator<(const datetime& lho, const datetime& rho)
{
    return lho.seconds_since_epoch() < rho.seconds_since_epoch();
}

/// @brief Compare two instants of time
/// @return True if right time is older or equal, false otherwise
constexpr bool operator<=(const datetime& lho, const datetime& rho)
{
    return lho.seconds_since_epoch() <= rho.seconds_since_epoch();
}

/// @brief Compare two instants of time
/// @return True if datetimes are equal, false otherwise
constexpr bool operator==(const datetime& lho, const datetime& rho)
{
    return lho.seconds_since_epoch() == rho.seconds_since_epoch();
}

/// @brief Compare two instants of time
/// @return True if datetimes are not equal, false otherwise
constexpr bool operator!=(const datetime& lho, const datetime& rho)
{
    return !(lho.seconds_since_epoch() == rho.seconds_since_epoch());
}

////////////////////////////////////////////////////////////////////////////////
// JSON DE/SERIALIZATION
////////////////////////////////////////////////////////////////////////////////

inline void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const timedelta& td)
{
    jv = {
        { "time", td.length() }
    };
}

inline void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const datetime& td)
{
    jv = {
        { "time", td.seconds_since_epoch() }
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
