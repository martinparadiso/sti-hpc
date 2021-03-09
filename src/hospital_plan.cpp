/// @file hospital.cpp
/// @brief Hospital definitions
#include "hospital_plan.hpp"

#include <array>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <boost/json/src.hpp>
#include <repast_hpc/Properties.h>
#include <vector>

////////////////////////////////////////////////////////////////////////////
// HOSPITAL // CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

sti::hospital_plan::hospital_plan(hospital_plan&&) noexcept = default;
sti::hospital_plan& sti::hospital_plan::operator=(sti::hospital_plan&&) noexcept = default;

// Destructor definition required by unique_ptr
sti::hospital_plan::~hospital_plan() = default;

////////////////////////////////////////////////////////////////////////////
// HOSPITAL // PLAN
////////////////////////////////////////////////////////////////////////////

/// @brief Insert a new tile in a specific position
/// @param position The location to insert
/// @param t The type to set
void sti::hospital_plan::insert(const sti::coordinates& position, sti::hospital_plan::tile_t t)
{
    const auto x = static_cast<decltype(_plan)::size_type>(position.x);
    const auto y = static_cast<decltype(_plan)::size_type>(position.y);

    const auto not_special = std::array { tile_t::FLOOR, tile_t::WALL };
    const auto is_special  = [&](auto tile) -> bool {
        for (auto not_special_tile : not_special) {
            if (not_special_tile == tile) return false;
        }
        return true;
    };

    if (is_special(t)) {
        _special_tiles[t].push_back(position);
    }

    _plan.at(x).at(y) = t;
}

/// @brief Get the tile in a specific point
/// @param position The position to query
/// @return The tile located in that position
sti::hospital_plan::tile_t sti::hospital_plan::get(const sti::coordinates& position) const
{
    const auto x = static_cast<decltype(_plan)::size_type>(position.x);
    const auto y = static_cast<decltype(_plan)::size_type>(position.y);

    return _plan.at(x).at(y);
}

/// @brief Get all the specified tiles of type e present in the file
/// @return A vector containing the coordinates of all the specified tiles
const std::vector<sti::coordinates>& sti::hospital_plan::get_all(sti::hospital_plan::TYPES e) const
{
    return _special_tiles.at(e);
}

////////////////////////////////////////////////////////////////////////////
// PRIVATE COMPLEX INITIALIZATION
////////////////////////////////////////////////////////////////////////////

/// @brief Create a new empty hospital
/// @param width The width of the hospital
/// @param height The height of the hospital
sti::hospital_plan::hospital_plan(length_t width, length_t height)
    : _width { width }
    , _height { height }
{

    const auto y = static_cast<std::vector<tile_t>::size_type>(height);

    for (auto x = 0; x < width; x++) {
        _plan.push_back(std::vector<tile_t> { y, tile_t::FLOOR });
    }
}

////////////////////////////////////////////////////////////////////////////////
// HOSPITAL LOADING
////////////////////////////////////////////////////////////////////////////////

/// @brief Load a hospital from a file
/// @param tree The json properties
/// @return A hospital
sti::hospital_plan sti::load_hospital(const boost::json::value& tree)
{

    namespace json = boost::json;
    using tile_t   = sti::hospital_plan::tile_t;

    // First load the hospital plan
    const auto width  = json::value_to<int>(tree.at("building").at("width"));
    const auto height = json::value_to<int>(tree.at("building").at("height"));

    auto hospital = sti::hospital_plan { width, height };

    // Load all the tiles, first tiles that can appear multiple times, then
    // single use tiles
    
    const auto multi_tiles = std::vector<std::pair<std::string, tile_t>> {
        { "walls", tile_t::WALL },
        { "chairs", tile_t::CHAIR },
        { "receptionists", tile_t::RECEPTIONIST },
        { "doctors", tile_t::DOCTOR },
    };

    const auto unique_tile = std::vector<std::pair<std::string, tile_t>> {
        { "entry", tile_t::ENTRY },
        { "exit", tile_t::EXIT },
        { "triage", tile_t::TRIAGE },
        { "ICU", tile_t::ICU },
    };

    for (const auto& [key, type] : multi_tiles) {
        for (const auto& location : json::value_to<std::vector<json::value>>(tree.at("building").at(key))) {
            const auto& x = json::value_to<int>(location.at("x"));
            const auto& y = json::value_to<int>(location.at("y"));

            hospital.insert({ x, y }, type);
        }
    }

    for (const auto& [key, type] : unique_tile) {
        const auto& x = json::value_to<int>(tree.at("building").at(key).at("x"));
        const auto& y = json::value_to<int>(tree.at("building").at(key).at("y"));

        hospital.insert({ x, y }, type);
    }

    return hospital;
}

////////////////////////////////////////////////////////////////////////////////
// OPERATOR OVERLOADING
////////////////////////////////////////////////////////////////////////////////

bool operator==(const sti::coordinates& lo, const sti::coordinates& ro)
{
    return (lo.x == ro.x) && (lo.y == ro.y);
}

std::ostream& operator<<(std::ostream& os, const sti::coordinates& c)
{
    os << "["
       << c.x
       << ","
       << c.y
       << "]";
    return os;
}