/// @file hospital.hpp
/// @brief Hospital abstraction, for accessing all hospital related stuff
#pragma once

#include <map>
#include <memory>
#include <repast_hpc/Point.h>
#include <vector>

// Fw. declarations
namespace repast {
class Properties;
}

namespace boost {
namespace mpi {
    class communicator;
}
namespace json {
    class value;
}
}

namespace sti {

// Fw. declarations
class chair_manager;
class hospital_entry;
class hospital_exit;
class hospital_plan;

struct coordinates {
    using length_t = int;

    length_t x;
    length_t y;

    // Serialization
    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& x;
        ar& y;
    }
};

struct point {
    using length_t = double;

    length_t x;
    length_t y;

    point() = default;

    point(length_t x, length_t y)
        : x { x }
        , y { y }
    {
    }

    /// @brief Cast from repast point
    point(const repast::Point<double>& rp)
        : x { rp.getX() }
        , y { rp.getY() }
    {
    }

    /// @brief Cast to repast point
    explicit operator repast::Point<double>() const
    {
        return { x, y };
    }

    // Serialization
    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& x;
        ar& y;
    }
};

/// @brief Hospital abstaction, provides access to all hospital-related functions
class hospital_plan {
public:
    enum class TYPES {
        FLOOR,
        WALL,
        CHAIR,
        ENTRY,
        EXIT,
        TRIAGE,
        ICU,
        RECEPTIONIST,
        DOCTOR
    };
    using length_t = coordinates::length_t;
    using tile_t   = TYPES;
    using plan_t   = std::vector<std::vector<tile_t>>;

    using chair_manager_owning_ptr  = std::unique_ptr<sti::chair_manager>;
    using hospital_entry_owning_ptr = std::unique_ptr<sti::hospital_entry>;
    using hospital_exit_owning_ptr  = std::unique_ptr<sti::hospital_exit>;

    /// @brief Load a hospital from a file
    /// @param tree The json properties
    /// @return A hospital
    friend hospital_plan load_hospital(const boost::json::value& tree);

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    hospital_plan() = delete;

    hospital_plan(const hospital_plan&) = delete;
    hospital_plan& operator=(const hospital_plan&) = delete;

    hospital_plan(hospital_plan&&) noexcept;
    hospital_plan& operator=(hospital_plan&&) noexcept;

    ~hospital_plan();

    ////////////////////////////////////////////////////////////////////////////
    // HOSPITAL PLAN
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the width of the hospital
    auto width() const
    {
        return _width;
    }

    /// @brief Get the height of the hospital
    auto height() const
    {
        return _height;
    }

    /// @brief Insert a new tile in a specific position
    /// @param position The location to insert
    /// @param t The type to insert
    void insert(const coordinates& position, tile_t t);

    /// @brief Get the tile type in a specific point
    /// @param position The position to query
    /// @return The tile type located in that position
    tile_t get(const coordinates& position) const;

    /// @brief Get all the specified tiles of type e present in the file
    /// @param e The desired type
    /// @return A vector containing the coordinates of all the specified tiles
    const std::vector<coordinates>& get_all(TYPES e) const;

private:
    ////////////////////////////////////////////////////////////////////////////
    // PRIVATE COMPLEX INITIALIZATION
    ////////////////////////////////////////////////////////////////////////////
    hospital_plan(length_t width, length_t height);

    /// @brief Create a new empty hospital
    /// @param width The width of the hospital
    /// @param height The height of the hospital

    plan_t                                          _plan;
    std::map<tile_t, std::vector<sti::coordinates>> _special_tiles;

    length_t _width;
    length_t _height;
};

/// @brief Load a hospital from a file
/// @param tree The json properties
/// @return A hospital
hospital_plan load_hospital(const boost::json::value& tree);

} // namespace sti

bool operator==(const sti::coordinates& lo, const sti::coordinates& ro);

std::ostream& operator<<(std::ostream& os, const sti::coordinates& c);
