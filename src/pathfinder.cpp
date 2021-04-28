#include "pathfinder.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "coordinates.hpp"
#include "debug_flags.hpp"
#include "clock.hpp"

/// @brief Collect pathfinding statistics
class sti::pathfinder::statistics {

public:
    struct entry {
        datetime      datetime;
        std::uint32_t cache_miss {};
        std::uint32_t cache_hit {};
    }; // struct entry

    /// @brief Construct a new statistics object
    statistics(const clock* clock)
        : _clock { clock }
    {
    }

    /// @brief Notify of a cache hit
    /// @details The pathfinder saves all previous calculated paths in a cache,
    /// this method is used to count the number of cache hits
    void cache_hit()
    {
        if constexpr (sti::debug::pathfinder_statistics) {
            if (_cache.begin() == _cache.end() || (_cache.end() - 1)->datetime != _clock->now()) {
                _cache.push_back({ _clock->now() });
            }
            const auto last = _cache.end() - 1;
            last->cache_hit += 1;
        }
    } // void cache_hit()

    /// @brief Notify of a cache miss
    /// @details The pathfinder saves all previous calculated paths in a cache,
    /// this method is used to count the number of cache misses
    void cache_miss()
    {
        if constexpr (sti::debug::pathfinder_statistics) {
            if (_cache.begin() == _cache.end() || (_cache.end() - 1)->datetime != _clock->now()) {
                _cache.push_back({ _clock->now() });
            }
            const auto last = _cache.end() - 1;
            last->cache_miss += 1;
        }
    } // void cache_miss()

    /// @brief Save the statistics to a file
    /// @param folderpath The folder to save the file to
    /// @param rank The rank of this process
    void save(const std::string& folderpath, int rank) const
    {
        if constexpr (sti::debug::pathfinder_statistics) {

            auto os = std::ostringstream {};
            os << folderpath
               << "/pathfinder.p"
               << rank
               << ".csv";

            auto file = std::ofstream { os.str() };
            file << "datetime,hits,misses\n";

            for (const auto& entry : _cache) {
                file << entry.datetime.epoch() << ','
                     << entry.cache_hit << ','
                     << entry.cache_miss << '\n';
            }
        }
    } // void save(...)

private:
    const clock*       _clock;
    std::vector<entry> _cache;
}; // struct sti::pathfinder::statistics

/// @brief Construct a pathfinder
/// @param obstacles The map with the obstacles
/// @param clock The simulation clock, for statistics collection
sti::pathfinder::pathfinder(const obstacles_map* obstacles,
                            const clock*         clock)
    : _obstacles { obstacles }
    , _stats { std::make_unique<statistics>(clock) }
{
} // sti::pathfinder::pathfinder(...)

/// @brief Destruct defined later due to fw. declaration of statistics
sti::pathfinder::~pathfinder() = default;

/// @brief Helper functions for the pathfinding
namespace {

/// @brief Calculate the distance between two cells
/// @param a One point
/// @param b Another point
inline double distance(const sti::coordinates<int>& a, const sti::coordinates<int>& b)
{
    auto d = a - b;
    d.x    = std::abs(d.x);
    d.y    = std::abs(d.y);
    return static_cast<double>(d.x + d.y);
}

/// @brief Calculate the distance to the goal, A.K.A. the heuristic function
/// @param from Origin point
/// @param goal Destination point
inline double heuristic(const sti::coordinates<int>& from, const sti::coordinates<int>& goal)
{
    return distance(from, goal);
}

/// @brief Get the neighbors of a given cell
/// @param obstacles The map with the obstacles
/// @param cell The cell to look for walkable neighbors
/// @param goal The destination, it is needed to determine if a normally
/// non-walkable cell should be inserted into the neighbors. For instance, the
/// exit is non-walkable because it absorbs all the patients walking over it,
/// but if the destination is the exit, the patient needs to step on it
inline std::vector<sti::coordinates<int>> adjacents(
    const sti::pathfinder::obstacles_map& obstacles,
    const sti::coordinates<int>&          cell,
    const sti::coordinates<int>&          goal)
{

    auto       neighbors = std::vector<sti::coordinates<int>> {};
    const auto diffs     = std::array {
        sti::coordinates<int> { 0, 1 },
        sti::coordinates<int> { 0, -1 },
        sti::coordinates<int> { 1, 0 },
        sti::coordinates<int> { -1, 0 }
    };
    for (const auto& diff : diffs) {
        const auto& neighbor = cell + diff;
        if (neighbor.x < 0 || neighbor.y < 0) continue;
        if (static_cast<std::size_t>(neighbor.x) >= obstacles.size()
            || static_cast<std::size_t>(neighbor.y) >= obstacles.at(0).size()) continue;

        // If the neighbor is walkable or is the goal, add it to the return
        // vector
        const auto is_walkable = obstacles.at(static_cast<std::size_t>(neighbor.x))
                                     .at(static_cast<std::size_t>(neighbor.y));
        if (is_walkable || neighbor == goal) {
            neighbors.push_back(neighbor);
        }
    }

    return neighbors;
}

/// @brief Structure used in the heap queue for A*
struct heap_element {
    sti::coordinates<int> cell;
    double                f_value;
}; // struct node

/// @brief Given a container queue, push element following min heap criteria
template <typename T>
void push_heap(T& queue, const heap_element& value)
{
    queue.push_back(value);
    std::push_heap(queue.begin(), queue.end(),
                   [](const auto& lo, const auto& ro) {
                       return lo.f_value > ro.f_value;
                   });
} // void push_heap(...)

/// @brief Given a container queue, pop element following min heap criteria
template <typename T>
auto pop_heap(T& queue)
{
    std::pop_heap(queue.begin(), queue.end(),
                  [](const auto& lo, const auto& ro) {
                      return lo.f_value > ro.f_value;
                  });
    const auto& value = *(queue.end() - 1);
    queue.pop_back();
    return value;
} // auto pop_heap(..)

/// @brief Store the generated path into the cache
/// @param cache The cache were paths are saved, in the format 'current_cell' -> 'next_cell'
/// @param came_from A map in the form of 'cell' -> 'previous_cell'
/// @param goal The final cell of the path, to start the unwinging of came_from
void save_path(
    std::map<sti::coordinates<int>, sti::coordinates<int>>&       cache,
    const std::map<sti::coordinates<int>, sti::coordinates<int>>& came_from,
    const sti::coordinates<int>                                   goal)
{
    for (auto it = came_from.find(goal);
         it != came_from.end();
         it = came_from.find(it->second)) {
        cache[it->second] = it->first;
    }
} // void save_path(...)

/// @brief Exception thrown when no path is found between two cells
struct no_path : public std::exception {

    std::string msg;

    no_path(const sti::coordinates<int>& a, const sti::coordinates<int>& b)
    {
        auto os = std::ostringstream {};
        os << "Exception: No path between " << a
           << " and " << b << '\n';
        msg = os.str();
    };

    const char* what() const noexcept override
    {
        return msg.c_str();
    }
}; // struct no_path

} // namespace

/// @brief Get the next point in the path from start to goal
/// @throws no_path If there is no path between the points
/// @param start The initial point
/// @param goal The destination point
/// @return The next cell to go to in order to arrive to the destination
sti::coordinates<int> sti::pathfinder::next_step(const coordinates<int>& start,
                                                 const coordinates<int>& goal)
{

    /// @brief Check if a given path is cached
    /// @param paths The map of cached paths
    /// @param start_cell The starting point of the path
    /// @param goal_cell The end point of the path
    const auto search_cache = [](const auto& paths,
                                 const auto& start_cell,
                                 const auto& goal_cell,
                                 auto&       stats)
        -> std::optional<coordinates<int>> {
        const auto has_goal_cell = paths.find(goal_cell);
        if (has_goal_cell != paths.end()) {
            const auto has_start_cell = has_goal_cell->second.find(start_cell);
            if (has_start_cell != has_goal_cell->second.end()) {
                stats.cache_hit();
                return has_start_cell->second;
            }
        }
        stats.cache_miss();
        return {};
    }; // const auto search_cache

    // First check if the start-goal have been previously calculated
    const auto is_og_cached = search_cache(_paths, start, goal, *_stats);
    if (is_og_cached) {
        return *is_og_cached;
    }

    // Otherwise, the path start -> goal doesn't exists, perform the search
    auto open_set = std::vector<heap_element> {};
    push_heap(open_set, { start, 0 });
    auto came_from = std::map<sti::coordinates<int>, sti::coordinates<int>> {};
    auto g_score   = std::map<sti::coordinates<int>, double> {};
    g_score[start] = 0.0;
    auto f_score   = std::map<sti::coordinates<int>, double> {};
    f_score[start] = heuristic(start, goal);

    // Search the set of nodes until is empty or the optimal path is found
    while (!open_set.empty()) {
        const auto& [current, h] = pop_heap(open_set);

        if (current == goal) {
            save_path(_paths[goal], came_from, goal);
            break;
        }

        // If the current cell is in the cache, the path from here to the goal
        // already exists, so reutilize that
        const auto is_cached = search_cache(_paths, start, goal, *_stats);
        if (is_cached) {
            save_path(_paths[goal], came_from, goal);
            break;
        }

        for (const auto& neighbor : adjacents(*_obstacles, current, goal)) {

            const auto tentative_gscore = g_score.at(current) + distance(current, neighbor);

            const auto& has_gscore      = g_score.find(neighbor);
            const auto  existing_gscore = has_gscore == g_score.end()
                ? std::numeric_limits<double>::infinity()
                : has_gscore->second;

            if (tentative_gscore < existing_gscore) {
                came_from[neighbor] = current;
                g_score[neighbor]   = tentative_gscore;
                f_score[neighbor]   = tentative_gscore + heuristic(neighbor, goal);

                const auto exists = std::find_if(open_set.begin(), open_set.end(),
                                                 [&](const auto& element) {
                                                     return element.cell == neighbor;
                                                 });
                if (exists == open_set.end()) {
                    push_heap(open_set, { neighbor, f_score.at(neighbor) });
                }
            } // if (tentative_gscore < existing_gscore)
        } // for (const auto& neighbor : adjacents(*_obstacles, current, goal))
    } // while (!open_set.empty())

    // Down here the element should be in the cache, find it and return it
    try {
        return _paths.at(goal).at(start);
    } catch (const std::out_of_range& /*unused*/) {
        throw no_path { start, goal };
    }

} // sti::coordinates<int> sti::pathfinder::next_step(...)

////////////////////////////////////////////////////////////////////////////
// SAVE STATISTICS
////////////////////////////////////////////////////////////////////////////

/// @brief Save the stadistics/metrics to a file
/// @param filepath The path to the folder where
/// @param rank The rank of the process
void sti::pathfinder::save(const std::string& folderpath, int rank) const
{
    _stats->save(folderpath, rank);
}