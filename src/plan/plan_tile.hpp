/// @file plan_tile.hpp
/// @brief Represents a tile in the building plan
#pragma once

#include <array>
#include <cstdint>
#include <exception>
#include <type_traits>

namespace sti {

struct bad_tile_cast : public std::exception {

    const char* what() const noexcept override
    {
        return "Exception: Bad tile cast";
    }
};

/// @brief Represents a tile in the building plan
/// @details Represents a tile in the hospital building plan, includes medical
///          personnel.
class plan_tile {

public:
    /// @brief The available tile types
    enum class TILE_ENUM : std::uint8_t { FLOOR,
                                          WALL,

                                          CHAIR,

                                          ENTRY,
                                          EXIT,
                                          TRIAGE,
                                          ICU,

                                          RECEPTIONIST,
                                          DOCTOR,
    };

    /// @brief Create a new tile object, by default a floor
    plan_tile(TILE_ENUM tile_type = TILE_ENUM::FLOOR, std::uint8_t data = 0)
        : _type { tile_type }
        , _data { data }
    {
    }

    /// @brief Get the type of the tile
    /// @return An enum representing the tile type
    constexpr auto get_type() const
    {
        return _type;
    }

    /// @brief Performs a conversion to a type T
    /// @details Performs a conversion to a type T, allowing safe access to the
    /// @throws bad_tile_cast If the tile cannot be casted to type T
    /// @return An object of type T
    template <typename T>
    T get() const = delete;

    // Behaviour ===============================================================

    /// @brief Check if the tile can be walk over or is a solid object
    /// @details Note: the concept of "solid" object is completely arbitrary
    /// @return True if the person can walk over it
    bool is_walkable() const {

        constexpr auto solids = std::array{
            TILE_ENUM::WALL,
            TILE_ENUM::RECEPTIONIST,
            TILE_ENUM::DOCTOR,
        };

        const auto contains = [&](auto type) {
            for (auto tile : solids) {
                if (type == tile) return true;
            }
            return false;
        };

        return contains(_type);
    }

private:
    TILE_ENUM    _type;
    std::uint8_t _data;
};

/// @brief A tile where a doctor is located
/// @details A tile where a doctor is located, contains information about the
///          doctor discipline/specialty
struct doctor_tile {

    /// @brief The medical specialty of the doctor assigned to this location
    std::uint8_t specialty_id = 0;
};

/// @brief Performs a conversion to doctor_tile
/// @details Convert a generic tile to a doctor_tile, allowing doctor-specific
///          behaviour
/// @throws bad_tile_cast If the tile cannot be casted to doctor_tile
/// @return An object of type doctor_tile
template <>
inline doctor_tile plan_tile::get<doctor_tile>() const
{

    // Check if the tile is a doctor
    if (_type != TILE_ENUM::DOCTOR) throw bad_tile_cast {};

    // TODO: Check if the _data/specialty_id is in the range of valid doctor
    // specialties

    return doctor_tile { _data };
}

} // namespace sti