/// @brief Simulation model
#pragma once


#include <boost/lexical_cast.hpp>
#include <cstdint>
#include <memory>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/Properties.h>
#include <repast_hpc/SharedContext.h>
#include <repast_hpc/SharedContinuousSpace.h>
#include <repast_hpc/GridComponents.h>
#include <repast_hpc/GridDimensions.h>
#include <repast_hpc/Point.h>
#include <repast_hpc/RepastProcess.h>
#include <repast_hpc/Schedule.h>
#include <repast_hpc/SharedDiscreteSpace.h>
#include <repast_hpc/Utilities.h>
#include <repast_hpc/initialize_random.h>

#include "agent_package.hpp"
#include "chair_manager.hpp"
#include "clock.hpp"
#include "contagious_agent.hpp"
#include "entry.hpp"
#include "exit.hpp"
#include "hospital_plan.hpp"
#include "json_loader.hpp"
#include "space_wrapper.hpp"

namespace sti {

/// @brief The Repast HPC simulation
class model {

public:
    using agent            = contagious_agent;
    using continuous_space = repast::SharedContinuousSpace<agent, repast::StrictBorders, repast::SimpleAdder<agent>>;
    using discrete_space   = repast::SharedDiscreteSpace<agent, repast::StrictBorders, repast::SimpleAdder<agent>>;

    model(const std::string& props_file, int argc, char** argv, boost::mpi::communicator* comm)
        : _communicator { comm }
        , _props { new repast::Properties(props_file, argc, argv, comm) }
        , _context(comm)
        , _rank { repast::RepastProcess::instance()->rank() }
        , _stop_at { 0 }
        , _hospital_props { load_json(_props->getProperty("hospital.file")) }
        , _hospital { sti::load_hospital(_hospital_props) }
        , _spaces { _hospital, *_props, _context, comm }
        , _clock { std::make_unique<clock>(boost::lexical_cast<std::uint64_t>(_props->getProperty("seconds.per.tick"))) }
    {

        // Initialize the random generation
        repast::initializeRandom(*_props, comm);
    }

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

    ////////////////////////////////////////////////////////////////////////////
    // AUXILIARY FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////

private:
    boost::mpi::communicator*    _communicator;
    repast::Properties*          _props;
    repast::SharedContext<agent> _context;
    const int                    _rank;
    int                          _stop_at;

    boost::json::object _hospital_props;
    sti::hospital_plan _hospital;

    space_wrapper                   _spaces;
    std::unique_ptr<agent_provider> _provider;
    std::unique_ptr<agent_receiver> _receiver;

    std::unique_ptr<clock> _clock;

    std::unique_ptr<agent_factory> _agent_factory {}; // Properly initalized in init()
    std::unique_ptr<chair_manager> _chair_manager {}; // Properly initalized in init()

    // Extra classes that the model may or may not have depending on the
    // location
    std::unique_ptr<hospital_entry> _entry {}; // Properly initalized in init()
    std::unique_ptr<hospital_exit> _exit {};
}; // class model

} // namespace sti
