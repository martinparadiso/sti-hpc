#include "clock.hpp"

#include <iomanip>
#include <sstream>

/// @brief Get the date in string format
/// @details The format returned is [Day, Hours/Minutes/Seconds]
std::string sti::timedelta::str() const
{
    const auto human = this->human();
    auto       os    = std::ostringstream {};
    os << "[Day "
       << std::setfill('0') << std::setw(3)
       << human.days << ", "
       << std::setfill('0') << std::setw(2)
       << human.hours << ":"
       << std::setfill('0') << std::setw(2)
       << human.minutes << ":"
       << std::setfill('0') << std::setw(2)
       << human.seconds << "]";
    return os.str();
}

/// @brief Get the date in string format
/// @details The format returned is [Day, Hours/Minutes/Seconds]
std::string sti::datetime::str() const
{
    return _timedelta.str();
}

/// @brief Adjust time, must be executed every tick
/// @param tick The current tick the simulation is running
void sti::clock::sync(double tick)
{
    _tick = tick;
}
