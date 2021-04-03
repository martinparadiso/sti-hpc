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

    /// @brief Cast a discrete coordinates to double
    coordinates<double> continuous() const
    {
        static_assert(std::is_same_v<int, T>, "Can't convert from double to double");
        return {
            static_cast<double>(x) + 0.5,
            static_cast<double>(y) + 0.5,
        };
    }

    /// @brief Cast a discrete coordinates to double
    coordinates<int> discrete() const
    {
        static_assert(std::is_same_v<double, T>, "Can't convert from int to int");
        return {
            static_cast<int>(x),
            static_cast<int>(y)
        };
    }

    // Serialization
    template <typename Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& x;
        ar& y;
    }
};


template <typename T>
coordinates<T> operator+(const coordinates<T>& lo, const coordinates<T>& ro)
{
    return { lo.x + ro.x, lo.y + ro.y };
}

template <typename T>
coordinates<T> operator-(const coordinates<T>& lo, const coordinates<T>& ro)
{
    return { lo.x - ro.x, lo.y - ro.y };
}

template <typename T>
bool operator==(const coordinates<T>& lo, const coordinates<T>& ro)
{
    return lo.x == ro.x && lo.y == ro.y;
}

template <typename T>
bool operator!=(const coordinates<T>& lo, const coordinates<T>& ro)
{
    return !(lo == ro);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const coordinates<T>& c)
{
    os << "["
       << c.x
       << ","
       << c.y
       << "]";
    return os;
}

} // namespace sti
