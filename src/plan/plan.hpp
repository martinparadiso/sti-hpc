#pragma once

#include <cstdint>
#include <exception>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "plan_tile.hpp"

namespace sti {

struct inconsistent_column : public std::exception {

    const char* what() const noexcept override
    {
        return "Exception: Wrong number of tiles in some column";
    }
};

/// @brief Check if the tile is relevant
/// @details Check if a given tile is a relevant, the relevent tiles are those
///          used by the simulation logic, for instance doctors and chairs.
inline bool is_special(plan_tile tile)
{
    // The special tiles to look for and store
    const auto look_for = std::array {
        plan_tile::TILE_ENUM::CHAIR,

        plan_tile::TILE_ENUM::ENTRY,
        plan_tile::TILE_ENUM::EXIT,
        plan_tile::TILE_ENUM::TRIAGE,
        plan_tile::TILE_ENUM::ICU,

        plan_tile::TILE_ENUM::RECEPTIONIST,
        plan_tile::TILE_ENUM::DOCTOR,
    };

    for (auto type : look_for) {
        if (tile.get_type() == type) return true;
    }
    return false;
}

class plan {

public:
    using tile_type   = plan_tile;
    using column_type = std::vector<tile_type>;
    using key_type    = plan_tile::TILE_ENUM;
    using length_type = std::uint32_t;

    struct dimensions {
        length_type width;
        length_type height;
    };

    struct coordinates {
        length_type x;
        length_type y;
    };

    /// @brief Pair of coordinates delimiting a zone of the plan
    /// @details The bottom limit is always the bottom left corner of the rectangular zone
    struct zone {
        coordinates bottom_limit;
        coordinates upper_limit;
    };

    /// @brief Create an empty plan
    plan() = default;

    /// @brief Create a map of a given size, filled with empty spaces
    plan(length_type width, length_type height)
        : _dimensions { width, height }
    {
        for (auto i = 0U; i < width; i++) {
            _tiles.emplace_back( column_type{height, plan_tile::TILE_ENUM::FLOOR } );
        }
    }

    /// @brief Add a new column of tiles to the plan
    /// @param new_column The new column
    /// @throws inconsistent_row If the row has a different size that previous
    void add_column(column_type&& new_column)
    {
        // If there is at least one column, make sure the newly insserted column
        // matches in size
        if (!_tiles.empty()) {
            if (_tiles[0].size() != new_column.size()) throw inconsistent_column {};
        }

        _tiles.emplace_back(std::move(new_column));

        // Scan the newly inserted row for *special* tiles
        const auto x = static_cast<length_type>(_tiles.size() - 1);
        auto       y = 0U;

        for (auto tile : _tiles[_tiles.size() - 1]) {
            if (is_special(tile)) {
                _special_tiles[tile.get_type()].push_back({ x, y });
            }
            ++y;
        }

        // Re-assign width and height of the plan
        _dimensions.width = _tiles.size();
        _dimensions.height = _tiles.begin()->size();
    }

    /// @brief Access the (x,y) position
    /// @param x X position
    /// @param y Y position
    /// @return The tile in that location
    tile_type at(unsigned x, unsigned y) const
    {
        // FIXME: Change to subscript in release mode
        return _tiles.at(x).at(y);
    }

    /// @brief Access the (x,y) position
    /// @param c Coordinates
    /// @return The tile in that location
    tile_type at(coordinates c) const
    {
        // FIXME: Change to subscript in release mode
        return _tiles.at(c.x).at(c.y);
    }

    /// @brief Get a const vector containing coordinates to that points
    /// @return a const vector containing coordinates to the special tiles
    const auto& get(key_type key) const
    {
        return _special_tiles.at(key);
    }

    auto width() const
    {
        return _dimensions.width;
    }

    auto height() const
    {
        return _dimensions.width;
    }

private:
    dimensions _dimensions = {0, 0};
    // length_type                                              _width = 0;
    // length_type                                              _height = 0;
    std::vector<column_type>                                 _tiles         = {};
    std::map<plan_tile::TILE_ENUM, std::vector<coordinates>> _special_tiles = {};
};

}
