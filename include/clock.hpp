/// @brief Clock class, to simulate passsage of time
#pragma once

#include <cstdint>

#include <repast_hpc/RepastProcess.h>

namespace sti {

/// @brief The clock class
class clock {

public:
    using resolution = std::uint64_t;
    
    /// @brief Date presented in a fancy format with days, hours, minutes and seconds
    class fancy_date {

    public:
        fancy_date(resolution epoch)
        {
            _seconds = epoch;

            _minutes = _seconds / 60;
            _seconds = _seconds % 60;

            _hours   = _minutes / 60;
            _minutes = _minutes % 60;

            _days  = _hours / 24;
            _hours = _hours % 24;
        }

        auto seconds() const
        {
            return _seconds;
        }

        auto minutes() const
        {
            return _minutes;
        }

        auto hours() const
        {
            return _hours;
        }

        auto days() const
        {
            return _days;
        }

    private:
        resolution _seconds;
        resolution _minutes;
        resolution _hours;
        resolution _days;
    };

    /// @brief Basic date format, with a maximum size of days
    class date_t {

    public:
        using resolution = std::uint64_t;

        /// @brief Construct a date, by default in time 0
        date_t(resolution epoch = 0)
            : _epoch { epoch }
        {
        }

        /// @brief Get the date in a fancy format
        fancy_date get_fancy() const {
            return fancy_date{_epoch};
        }

        /// @brief Get the number of seconds represented by this date
        auto epoch() const {
            return _epoch;
        }

    private:
        resolution _epoch;
    };

    /// @brief Create a new clock, starting the count in this instant
    /// @details The time is measured with the repast tick system. Using a
    ///          constant to convert from ticks to seconds.
    clock(date_t::resolution seconds_per_tick)
        : _tick { 0 }
        , _seconds_per_tick { seconds_per_tick }
    {
    }

    /// @brief Adjust time, must be executed every tick
    void sync()
    {
        _tick = repast::RepastProcess::instance()->getScheduleRunner().currentTick();
    }

    /// @brief Get the simulated seconds since the start of the simulation
    date_t now() const
    {
        return static_cast<date_t::resolution>(_tick) * _seconds_per_tick;
    }

    /// @brief Get the time passed in the simulation, in fancy format
    /// @return The date
    fancy_date date() const
    {
        return now().epoch();
    }

private:
    double                   _tick;
    const date_t::resolution _seconds_per_tick;
};

} // namespace sti