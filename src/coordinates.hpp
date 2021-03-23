/// @file coordinates.hpp
/// @brief Represents discrete and continuous coordinates
#pragma once

#include <repast_hpc/Point.h>

namespace sti {

template <typename T>
struct coordinates {
    using length_t = T;

    length_t x;
    length_t y;

    coordinates() = default;

    coordinates(length_t x, length_t y)
        : x { x }
        , y { y }
    {
    }

    /// @brief Cast from repast point
    coordinates(const repast::Point<length_t>& rp)
        : x { rp.getX() }
        , y { rp.getY() }
    {
    }

    /// @brief Cast to repast point
    operator repast::Point<length_t>() const
    {
        return { x, y };
    }

    // Serialization
    template <typename Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& x;
        ar& y;
    }
};

} // namespace sti

template <typename T>
bool operator==(const sti::coordinates<T>& lo, const sti::coordinates<T>& ro)
{
    return lo.x == ro.x && lo.y == ro.y;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const sti::coordinates<T>& c)
{
    os << "["
       << c.x
       << ","
       << c.y
       << "]";
    return os;
}
