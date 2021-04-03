/// @brief Clock class, to simulate passsage of time
#pragma once

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
    constexpr auto epoch() const
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
    void sync();

    /// @brief Get the time inside the simulation
    /// @return A datetime object with this instant of time
    datetime now() const
    {
        return datetime { static_cast<timedelta::resolution>(_tick) * _seconds_per_tick };
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
    return datetime { lho.epoch() + rho.length() };
}

/// @brief Compare two instants of time
/// @return True if right time is older, false otherwise
constexpr bool operator<(const datetime& lho, const datetime& rho)
{
    return lho.epoch() < rho.epoch();
}

/// @brief Compare two instants of time
/// @return True if right time is older or equal, false otherwise
constexpr bool operator<=(const datetime& lho, const datetime& rho)
{
    return lho.epoch() <= rho.epoch();
}

} // namespace sti
