#include "pathfinder.hpp"
#include <cassert>
#include <coordinates.hpp>
#include <iostream>
#include <limits>
#include <vector>
#include "clock.hpp"

void print(const std::vector<std::vector<bool>>&     map,
           const sti::coordinates<int>&              start_1,
           const sti::coordinates<int>&              goal_1,
           const std::vector<sti::coordinates<int>>& path)
{
    const auto ncols = map.size();
    const auto nrows = map.at(0).size();
    for (auto y = nrows - 1; y != std::numeric_limits<decltype(nrows)>::max(); --y) {
        for (auto x = 0UL; x < ncols; ++x) {
            auto       cell    = sti::coordinates<int> { static_cast<int>(x), static_cast<int>(y) };
            const auto is_path = std::find(path.begin(), path.end(), cell);

            if (is_path != path.end()) {
                std::cout << '*';
            } else if (start_1.x == x && start_1.y == y) {
                std::cout << 'S';
            } else if (goal_1.x == x && goal_1.y == y) {
                std::cout << 'G';
            } else {
                std::cout << (map.at(x).at(y) ? ' ' : '#');
            }
            std::cout << ' ';
        }
        std::cout << '\n';
    }
}
int main(int argc, char* argv[])
{
    auto args = std::vector(argv, argv + argc);
    // Create the obstacles map, 10x10 grid completley walkable
    const auto ncols = 10UL;
    const auto nrows = 10UL;
    auto       map   = std::vector<std::vector<bool>> {};
    for (auto x = 0UL; x < ncols; ++x) {
        map.push_back(std::vector<bool>(nrows, true));
    }

    // Then add the walls in the border
    for (auto x = 0UL; x < ncols; ++x) {
        for (auto y = 0UL; y < nrows; ++y) {
            if (x == 0 || x == ncols - 1) map.at(x).at(y) = false;
            if (y == 0 || y == nrows - 1) map.at(x).at(y) = false;
        }
    }

    // and add the middle diagonal wall
    for (auto i = 2UL; i < nrows - 2; ++i) {
        map.at(i).at(i) = false;
    }

    // The path starts at (1, 8), the goal is (8, 1)
    const auto start_1 = sti::coordinates<int> { 1, 8 };
    const auto goal_1  = sti::coordinates<int> { 8, 1 };

    // Create the pathfinder
    auto clock      = sti::clock { 60 };
    auto pathfinder = sti::pathfinder { &map, &clock };

    // Generate the path to go from start to end
    auto path_1 = std::vector<sti::coordinates<int>> {};
    for (auto current = pathfinder.next_step(start_1, goal_1); current != goal_1; current = pathfinder.next_step(current, goal_1)) {
        path_1.push_back(current);
    }

    // Print the map to stdout
    print(map, start_1, goal_1, path_1);

    // Try again from the same spot, there should be no cache misses, update
    // the clock to simulate another tick
    clock.sync(1.0);
    const auto start_2 = sti::coordinates<int> { 1, 8 };
    const auto goal_2  = sti::coordinates<int> { 8, 1 };
    auto       path_2  = std::vector<sti::coordinates<int>> {};
    for (auto current = pathfinder.next_step(start_2, goal_2); current != goal_2; current = pathfinder.next_step(current, goal_2)) {
        path_2.push_back(current);
    }
    print(map, start_2, goal_2, path_2);

    assert(stats_1->cache_misses == stats_2->cache_misses);

    // Save the stats
    pathfinder.save(args.at(1), 0);

    return 0;
}
