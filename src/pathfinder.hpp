/// @file pathfinder.hpp
/// @brief The path finder that generates the paths for the patients
#pragma once

#include <unordered_map>
#include <memory>
#include <utility>
#include <vector>

#include "coordinates.hpp"

// Fw. declarations
namespace sti {
class clock;
class datetime;
} // namespace sti

namespace sti {

/// @brief Generate and provide paths to the patients
class pathfinder {

public:
    using obstacles_map = std::vector<std::vector<bool>>;

    /// @brief Collect pathfinding statistics
    class statistics;

    /// @brief Construct a pathfinder
    /// @param obstacles The map with the obstacles
    /// @param clock The simulation clock, for statistics collection
    pathfinder(const obstacles_map* obstacles, const clock* clock);

    pathfinder(const pathfinder&) = delete;
    pathfinder& operator=(const pathfinder&) = delete;

    pathfinder(pathfinder&&) = default;
    pathfinder& operator=(pathfinder&&) = default;

    /// @brief Destruct defined later due to fw. declaration of statistics
    ~pathfinder();

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the next point in the path from start to goal
    /// @throws no_path If there is no path between the points
    /// @param start The initial point
    /// @param goal The destination point
    /// @return The next cell to go to in order to arrive to the destination
    coordinates<int> next_step(const coordinates<int>& start,
                               const coordinates<int>& goal);

    ////////////////////////////////////////////////////////////////////////////
    // SAVE STATISTICS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Save the stadistics/metrics to a file
    /// @param filepath The path to the folder where
    /// @param rank The rank of the process
    void save(const std::string& folderpath, int rank) const;

private:
    const obstacles_map* _obstacles;

    using destination_type = coordinates<int>;
    using from_type        = coordinates<int>;
    using next_step_type   = coordinates<int>;
    std::unordered_map<destination_type, std::unordered_map<from_type, next_step_type>> _paths;

    std::unique_ptr<statistics> _stats;
}; // class pathfinder

} // namespace sti