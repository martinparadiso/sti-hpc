/// @brief Simulation model
#pragma once

#include <cstdint>
#include <memory>
#include <repast_hpc/SharedContext.h>
#include <repast_hpc/SharedContinuousSpace.h>
#include <repast_hpc/GridComponents.h>
#include <repast_hpc/SharedDiscreteSpace.h>
#include <boost/json/object.hpp>

#include "agent_package.hpp"
#include "contagious_agent.hpp"
#include "hospital_plan.hpp"
#include "space_wrapper.hpp"

// Fw. declarations
namespace boost {
namespace mpi {
    class communicator;
} // namespace mpi
} // namespace boost

namespace repast {
class Properties;
class ScheduleRunner;
} // namespace repast

namespace sti {
class agent_factory;
class triage;
class doctors;
class icu;
} // namespace sti

namespace sti {

/// @brief Process metrics: local agents, sync and run time.
struct process_metrics {

    /// @brief The metrics to collect
    struct metrics { // clang-format off
        std::int32_t current_agents;    // Number of agents in this process
        std::int64_t mpi_sync_ns;       // Nanoseconds spent in non-repast sync
        std::int64_t rhpc_sync_ns;      // Nanoseconds spent in Repast sync
        std::int64_t logic_ns;          // Nanoseconds spent executing agent logic
        std::int64_t tick_start_time;   // The instant of time the tick started executing, epoch is the start of the simulation
    }; // clang-format on

    std::int64_t         simulation_epoch;
    std::vector<metrics> values;

    /// @brief Save the metrics to a file as a csv
    /// @param path The folder to save the metrics to
    /// @param process The process number
    void save(const std::string& folder, int process) const;
};

/// @brief The Repast HPC simulation
class model {

public:
    using agent            = contagious_agent;
    using continuous_space = repast::SharedContinuousSpace<agent, repast::StrictBorders, repast::SimpleAdder<agent>>;
    using discrete_space   = repast::SharedDiscreteSpace<agent, repast::StrictBorders, repast::SimpleAdder<agent>>;

    model(const std::string& props_file, int argc, char** argv, boost::mpi::communicator* comm);

    model(const model&) = delete;
    model& operator=(const model&) = delete;

    model(model&&) = delete;
    model& operator=(model&&) = delete;

    ~model();

    ////////////////////////////////////////////////////////////////////////////
    // REPAST FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Initialize the model
    /// @details Loads the map
    void init();

    /// @brief Initialize the scheduler
    /// @param runner The repast schedule runner
    void init_schedule(repast::ScheduleRunner& runner);

    /// @brief Periodic funcion
    void tick();

    /// @brief Final function for data collection and such
    void finish();

    /// @brief Remove all the agents that are still in the simulation
    /// @details Remove all the agents in the simulation and collect their
    /// metrics into a file
    void remove_remnants();

private:
    boost::mpi::communicator*    _communicator;
    repast::Properties*          _props;
    repast::SharedContext<agent> _context;
    const int                    _rank;
    int                          _stop_at;

    boost::json::object _hospital_props;
    sti::hospital_plan  _hospital;

    space_wrapper                   _spaces;
    std::unique_ptr<agent_provider> _provider;
    std::unique_ptr<agent_receiver> _receiver;

    std::unique_ptr<clock> _clock;

    process_metrics _pmetrics;

    std::unique_ptr<agent_factory> _agent_factory {}; // Properly initalized in init()

    std::unique_ptr<chair_manager> _chair_manager {}; // Properly initalized in init()
    std::unique_ptr<reception>     _reception {}; // Propertly initialized in init()
    std::unique_ptr<triage>        _triage {}; // Propertly initialized in init()
    std::unique_ptr<doctors>       _doctors {}; // Propertly initialized in init()
    std::unique_ptr<icu> _icu{};

    std::unique_ptr<hospital_entry> _entry {}; // Properly initalized in init()
    std::unique_ptr<hospital_exit>  _exit {}; // Properly initalized in init()

}; // class model

} // namespace sti
