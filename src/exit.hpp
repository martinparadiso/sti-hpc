/// @file exit.hpp
/// @brief Hospital exit, in charge of removing agents
#pragma once

#include <boost/json/object.hpp>
#include <memory>

#include "hospital_plan.hpp"

// Fw. declarations
namespace repast {
template <typename T>
class SharedContext;
}

namespace sti {

// Fw. declarations

class space_wrapper;
class contagious_agent;
class clock;

/// @brief Hospital exit, in charge of removing agents, and keeping several stats
/// @details The exit has access to the repast context and spaces, it will
///          remove any agent that falls in the same tile, so be careful.
class hospital_exit {

public:
    using repast_context     = repast::SharedContext<contagious_agent>;
    using repast_context_ptr = repast_context*;
    using space_ptr          = sti::space_wrapper*;
    using clock_ptr          = sti::clock*;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct the hospital exit
    /// @param context Repast context
    /// @param space Space wrapper
    /// @param clk The world clock
    /// @param location The tile the exit is located
    /// @param entry The hospital entry, to determine how many patients will enter
    hospital_exit(repast_context_ptr    context,
                  space_ptr             space,
                  clock_ptr             clk,
                  sti::coordinates<int> location,
                  const hospital_entry& entry);

    hospital_exit(const hospital_exit&) = delete;
    hospital_exit& operator=(const hospital_exit&) = delete;

    hospital_exit(hospital_exit&&) = default;
    hospital_exit& operator=(hospital_exit&&) = default;

    ~hospital_exit();

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Perform all the exit actions, must be called once by tick
    void tick();

    ////////////////////////////////////////////////////////////////////////////
    // SAVE STATISTICS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Save the stadistics/metrics to a file
    /// @param filepath The path to the folder where
    /// @param rank The rank of the process
    void save(const std::string& folderpath, int rank) const;

private:
    repast_context_ptr    _context;
    space_ptr             _space;
    clock_ptr             _clock;
    sti::coordinates<int> _location;
    struct impl;
    std::unique_ptr<impl> _pimpl;

}; // class hospital_exit

} // namespace sti
