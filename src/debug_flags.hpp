/// @file debug_flags.hpp
/// @brief Contains flags for conditional compiling and execution of debug statements
#pragma once

namespace sti {

/// @brief Contains debug flags for conditional compiling of debuging prints and metrics
namespace debug {

    /// @brief Patient state machine prints to stdout
    constexpr auto fsm_prints = false;

    /// @brief Doctors queue front prints
    constexpr auto doctors_print_front = false;

    /// @brief Record performance metrics of the main loop
    constexpr auto per_tick_performance = false;

    /// @brief Record the patients locations/movement
    constexpr auto track_movements = false;

    /// @brief Disable any cross-process synchronization
    /// @warning The simulation must run in only one process, otherwise it will crash
    constexpr auto disable_mp_sync = false;

    /// @brief Add a dummy line to use as breakpoint in the FSM logic, to monitor a specific patient
    constexpr auto fsm_debug_patient = false;

    /// @brief Collect pathfinding statistics
    constexpr auto pathfinder_statistics = false;

} // namespace debug
} // namespace sti
