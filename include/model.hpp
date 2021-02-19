/// @brief Simulation model
#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include <boost/lexical_cast.hpp>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/Properties.h>
#include <repast_hpc/SharedContext.h>
#include <repast_hpc/SharedDiscreteSpace.h>
#include <repast_hpc/GridComponents.h>
#include <repast_hpc/GridDimensions.h>
#include <repast_hpc/Point.h>
#include <repast_hpc/RepastProcess.h>
#include <repast_hpc/Schedule.h>
#include <repast_hpc/Utilities.h>
#include <repast_hpc/initialize_random.h>

#include "agent_creator.hpp"
#include "clock.hpp"
#include "contagious_agent.hpp"
#include "entry.hpp"
#include "plan/plan.hpp"
#include "parallel_agent.hpp"
#include "parallel_agent_package.hpp"

namespace sti {

/// @brief The Repast HPC simulation
class model {

public:
    model(const std::string& props_file, int argc, char** argv, boost::mpi::communicator* comm)
        : _props { new repast::Properties(props_file, argc, argv, comm) }
        , _context(comm)
        , _rank { repast::RepastProcess::instance()->rank() }
        , _stop_at { repast::strToInt(_props->getProperty("stop.at")) }
        , _clock{ std::make_unique<clock>(boost::lexical_cast<std::uint64_t>(_props->getProperty("seconds.per.tick"))) }
    {

        // TODO: document this
        repast::initializeRandom(*_props, comm);

        // Repast map specification
        const auto origin             = repast::Point<double> { 0, 0 };
        const auto extent             = repast::Point<double> { static_cast<double>(_plan.width()), static_cast<double>(_plan.height()) };
        const auto grid_dimensions    = repast::GridDimensions { origin, extent };
        const auto process_dimensions = std::vector<int> { repast::strToInt(_props->getProperty("x.process")),
                                                           repast::strToInt(_props->getProperty("y.process")) };

        _discrete_space = new repast::SharedDiscreteSpace<parallel_agent, repast::StrictBorders, repast::SimpleAdder<parallel_agent>>(
            "ParallelAgentDiscreteSpace",
            grid_dimensions,
            process_dimensions,
            1,
            comm);
        _context.addProjection(_discrete_space);
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

    // TODO: Remove this
    /// @brief Add a new agent to the simulation
    /// @param ca A unique_ptr to a contagious agent
    /// @param pos The position to put the agent
    // void create_new_agent(std::unique_ptr<contagious_agent> ca, plan::coordinates pos)
    // {
    //     // Create the agent
    //     const auto      type = to_int(ca->get_type());
    //     repast::AgentId id { _agents_created++, _rank, type, _rank };
    //     auto*           pa = new parallel_agent { id, std::move(ca) };

    //     // Add the agent to the context
    //     _context.addAgent(pa);

    //     // Set the agent initial position
    //     const auto initial_location = repast::Point<int> { static_cast<int>(pos.x), static_cast<int>(pos.y) };
    //     _discrete_space->moveTo(id, initial_location);
    // }

private:
    repast::Properties*                   _props;
    repast::SharedContext<parallel_agent> _context;
    const int                             _rank;
    int                                   _stop_at;

    repast::SharedDiscreteSpace<parallel_agent, repast::StrictBorders, repast::SimpleAdder<parallel_agent>>* _discrete_space;
    std::unique_ptr<parallel_agent_provider>                                                                 _provider;
    std::unique_ptr<parallel_agent_receiver>                                                                 _receiver;

    std::unique_ptr<clock> _clock;

    plan       _plan{};
    plan::zone _local_zone{};

    // Extra classes that the model may or may not have depending on the
    // location
    std::unique_ptr<hospital_entry> _entry{};
    std::unique_ptr<patient_factory> _patient_factory{};
}; // class model

} // namespace sti
