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
class staff_manager;
class triage;
class doctors;
class icu;
} // namespace sti

namespace sti {

/// @brief Process metrics: local agents, sync and run time
class process_metrics;

/// @brief The Repast HPC simulation
class model {

public:
    /// @brief Collect misc. stats durning the execution
    class statistics;

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
    /// @param folderpath The folder to write the file to
    /// @details Remove all the agents in the simulation and collect their
    /// metrics into a file
    void remove_remnants(const std::string& folderpath);

private:
    boost::mpi::communicator*    _communicator;
    repast::Properties*          _props;
    repast::SharedContext<agent> _context;
    const int                    _rank;
    int                          _stop_at;

    boost::json::object    _hospital_props;
    std::unique_ptr<clock> _clock;
    sti::hospital_plan     _hospital;

    space_wrapper                   _spaces;
    std::unique_ptr<agent_provider> _provider;
    std::unique_ptr<agent_receiver> _receiver;

    std::unique_ptr<process_metrics> _pmetrics;
    std::unique_ptr<statistics>      _stats;

    std::unique_ptr<agent_factory> _agent_factory {}; // Properly initalized in init()

    std::unique_ptr<chair_manager> _chair_manager {}; // Properly initalized in init()
    std::unique_ptr<reception>     _reception {}; // Properly initialized in init()
    std::unique_ptr<triage>        _triage {}; // Properly initialized in init()
    std::unique_ptr<doctors>       _doctors {}; // Properly initialized in init()
    std::unique_ptr<icu>           _icu {}; // Properly initialized in init()
    std::unique_ptr<staff_manager> _staff_manager {};

    std::unique_ptr<hospital_entry> _entry {}; // Properly initalized in init()
    std::unique_ptr<hospital_exit>  _exit {}; // Properly initalized in init()

}; // class model

} // namespace sti
