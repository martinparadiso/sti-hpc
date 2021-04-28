#include <boost/json/array.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpi/communicator.hpp>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/GridComponents.h>
#include <repast_hpc/GridDimensions.h>
#include <repast_hpc/initialize_random.h>
#include <repast_hpc/Point.h>
#include <repast_hpc/Properties.h>
#include <repast_hpc/RepastProcess.h>
#include <repast_hpc/Schedule.h>
#include <repast_hpc/SharedContext.h>
#include <repast_hpc/SharedContinuousSpace.h>
#include <repast_hpc/SharedDiscreteSpace.h>
#include <repast_hpc/Utilities.h>
#include <sstream>
#include <string>
#include <vector>

#include "agent_factory.hpp"
#include "agent_package.hpp"
#include "chair_manager.hpp"
#include "clock.hpp"
#include "contagious_agent.hpp"
#include "coordinates.hpp"
#include "debug_flags.hpp"
#include "doctors.hpp"
#include "doctors_queue.hpp"
#include "entry.hpp"
#include "exit.hpp"
#include "hospital_plan.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "infection_logic/object_infection.hpp"
#include "json_loader.hpp"
#include "json_serialization.hpp"
#include "model.hpp"
#include "staff_manager.hpp"
#include "triage.hpp"
#include "patient.hpp"
#include "reception.hpp"
#include "icu.hpp"
#include "icu/real_icu.hpp"

namespace {
auto now_in_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()).time_since_epoch()).count();
}
}

////////////////////////////////////////////////////////////////////////////////
// PROCESS METRICS
////////////////////////////////////////////////////////////////////////////////

/// @brief Process metrics: local agents, sync and run time.
class sti::process_metrics {

public:
    /// @brief Metrics of a single tick
    template <std::size_t MPIStages>
    struct tick_metrics {
        using instant_type               = std::int64_t;
        constexpr static auto mpi_stages = MPIStages;

        tick_metrics()
            : tick_start_time { now_in_ns() }
        {
        }

        std::int64_t                        tick_start_time; // Instant of time the tick started executing
        std::array<std::int64_t, MPIStages> mpi_sync_ns; // Finish time of each MPI sync stages
        std::int32_t                        current_agents {}; // Number of agents in this process
        std::int64_t                        rhpc_sync_ns {}; // Finish time of the RepastHPC sync
        std::int64_t                        logic_ns {}; // Finish time of logic execution
        std::int64_t                        tick_end_time {}; // Finish time of the tick
    };

    using per_tick_metrics = tick_metrics<5>;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a new metric collector
    /// @param mpi_stages_tags The names of the MPI stages
    process_metrics(const std::array<std::string, per_tick_metrics::mpi_stages>& mpi_stages_tags)
        : _simulation_epoch { now_in_ns() }
        , _mpi_stages_tags { mpi_stages_tags }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // GLOBAL METRICS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Indicate the number of ticks, so the metrics can preallocate enough space
    void preallocate(std::size_t ticks)
    {
        if constexpr (sti::debug::per_tick_performance) {
            _per_tick_metrics.reserve(ticks);
        }
    }

    /// @brief Indicate that the program is going to save files
    void start_save()
    {
        _presave_time = now_in_ns();
    }
    /// @brief Save the metrics to a file as a csv
    /// @param path The folder to save the metrics to
    /// @param process The process number
    void save(const std::string& folder, int process)
    {
        if constexpr (sti::debug::per_tick_performance) {
            auto tick_path = std::ostringstream {};
            tick_path << folder << "/tick_metrics.p" << process << ".csv";
            auto tick_file = std::ofstream { tick_path.str() };

            tick_file << "tick,"
                      << "start_time,"
                      << "end_time,"
                      << "agents,";
            for (const auto& tag : _mpi_stages_tags) tick_file << tag << "_sync,";
            tick_file << "rhpc_sync,"
                      << "logic\n";

            auto i = 0U;
            for (const auto& metric : _per_tick_metrics) {
                tick_file << i++ << ","
                          << metric.tick_start_time << ','
                          << metric.tick_end_time << ','
                          << metric.current_agents << ",";
                for (const auto& mpi_value : metric.mpi_sync_ns) tick_file << mpi_value << ",";
                tick_file << metric.rhpc_sync_ns << ","
                          << metric.logic_ns
                          << "\n";
            }
        }
        _end_time = now_in_ns();

        auto global_path = std::ostringstream {};
        global_path << folder << "/global_metrics.p" << process << ".csv";
        auto global_file = std::ofstream { global_path.str() };

        global_file << "epoch" << ','
                    << "presave_time" << ','
                    << "end_time" << '\n';

        global_file << _simulation_epoch << ','
                    << _presave_time << ','
                    << _end_time << '\n';
    }

    ////////////////////////////////////////////////////////////////////////////
    // PER TICK METRICS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Indicate the start of a new tick
    void new_tick()
    {
        if constexpr (sti::debug::per_tick_performance) {
            _per_tick_metrics.push_back(per_tick_metrics {});
            _current_tick = _per_tick_metrics.end() - 1;
        }
    }

    /// @brief Notify the start of the MPI synchronization
    /// @tparam StageNumber The stage of MPI to notify
    template <std::size_t StageNumber>
    void finish_mpi_stage() const
    {
        if constexpr (sti::debug::per_tick_performance) {
            _current_tick->mpi_sync_ns.at(StageNumber) = now_in_ns();
        }
    }

    /// @brief Notify the start of the RepastHPC synchronization
    void finish_rhpc_sync() const
    {
        if constexpr (sti::debug::per_tick_performance) {
            _current_tick->rhpc_sync_ns = now_in_ns();
        }
    }

    /// @brief Notify te start of the logic
    void finish_logic() const
    {
        if constexpr (sti::debug::per_tick_performance) {
            _current_tick->logic_ns = now_in_ns();
        }
    }

    /// @brief Indicate how many agents are currently in the simulation
    /// @param n The number of agents
    void agents(std::int32_t n) const
    {
        if constexpr (sti::debug::per_tick_performance) {
            _current_tick->current_agents = n;
        }
    }

    /// @brief Indicate the end of a tick
    void tick_end() const
    {
        if constexpr (sti::debug::per_tick_performance) {
            _current_tick->tick_end_time = now_in_ns();
        }
    }

private:
    std::int64_t                                          _simulation_epoch;
    std::int64_t                                          _presave_time {};
    std::int64_t                                          _end_time {};
    std::vector<per_tick_metrics>                         _per_tick_metrics {};
    decltype(_per_tick_metrics)::iterator                 _current_tick;
    std::array<std::string, per_tick_metrics::mpi_stages> _mpi_stages_tags {};
};

////////////////////////////////////////////////////////////////////////////////
// STATISTICS
////////////////////////////////////////////////////////////////////////////////

/// @brief Collect misc. stats durning the execution
class sti::model::statistics {

public:
    using agent_id_type = repast::AgentId;
    using location_type = coordinates<double>;

    /// @brief Preallocate the number of entries in the vector
    /// @param ticks The number of ticks the simulation will run
    void preallocate_ticks(std::size_t ticks)
    {
        if constexpr (sti::debug::track_movements) {
            _agents_locations.reserve(ticks);
        }
    }

    /// @brief Indicate the beggining of a new tick
    void new_tick(datetime epoch)
    {
        if constexpr (sti::debug::track_movements) {
            _agents_locations.push_back({ epoch, {} });
            _current_tick = _agents_locations.end() - 1;
        }
    }

    /// @brief Indicate the number of agents that will be stored in this tick
    /// @param nagents The number of agents for which positions will be collected
    void preallocate_agents(std::size_t nagents)
    {
        if constexpr (sti::debug::track_movements) {
            _current_tick->agents.reserve(nagents);
        }
    }

    /// @brief Add a new agent location to the current tick
    /// @param id The agent id
    /// @param location The agent location
    void add_agent_location(const repast::AgentId& id, const coordinates<double> location)
    {
        if constexpr (sti::debug::track_movements) {
            _current_tick->agents.push_back({ id, location });
        }
    }

    /// @brief Save the stats to a file as a csv
    /// @param folderpath The folder to save the metrics to
    /// @param rank The process number
    void save(const std::string& folderpath, int rank) const
    {
        if constexpr (sti::debug::track_movements) {

            auto filepath = std::ostringstream {};
            filepath << folderpath << "/agents_locations.p" << rank << ".csv";
            auto file = std::ofstream { filepath.str() };

            file << "epoch" << ','
                 << "id" << ','
                 << "x" << ','
                 << "y" << '\n';

            for (const auto& iteration : _agents_locations) {
                for (const auto& agent : iteration.agents) {
                    file << iteration.time.epoch() << ","
                         << agent.id << ","
                         << agent.location.x << ','
                         << agent.location.y << '\n';
                }
            }
        }
    }

    struct agent_location {
        agent_id_type id;
        location_type location;
    };

    struct tick_entry {
        datetime                    time;
        std::vector<agent_location> agents;
    };

private:
    std::vector<tick_entry>               _agents_locations;
    decltype(_agents_locations)::iterator _current_tick;
};

////////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////////

sti::model::model(const std::string& props_file, int argc, char** argv, boost::mpi::communicator* comm)
    : _communicator { comm }
    , _props { new repast::Properties(props_file, argc, argv, comm) }
    , _context(comm)
    , _rank { repast::RepastProcess::instance()->rank() }
    , _stop_at { 0 }
    , _hospital_props { load_json(_props->getProperty("hospital.file")) }
    , _clock { std::make_unique<clock>(boost::lexical_cast<std::uint64_t>(_props->getProperty("seconds.per.tick"))) }
    , _hospital { _hospital_props, _clock.get() }
    , _spaces { _hospital, *_props, _context, comm }
    , _pmetrics { new process_metrics { {
          "chairs",
          "reception",
          "triage",
          "doctors",
          "icu",
      } } }
    , _stats { new statistics {} }
    , _staff_manager { std::make_unique<decltype(_staff_manager)::element_type>(&_context) }
{
    // Initialize the random generation
    repast::initializeRandom(*_props, comm);
}

sti::model::~model() = default;

/// @brief Initialize the model
/// @details Loads the map
void sti::model::init()
{

    // Create the chair manager
    _chair_manager = make_chair_manager(*_props, _communicator, _hospital, &_spaces);
    _reception.reset(new reception { *_props, _communicator, _hospital });
    _triage.reset(new triage { *_props, _hospital_props, _communicator, _clock.get(), _hospital });
    _doctors = std::make_unique<doctors>(*_props, _hospital_props, _communicator, _hospital);
    _icu.reset(new icu(_communicator, *_props, _hospital_props, _hospital, &_spaces, _clock.get()));

    // Create the agent factory
    _agent_factory.reset(new agent_factory { _communicator,
                                             &_context,
                                             &_spaces,
                                             _clock.get(),
                                             &_hospital,
                                             _chair_manager.get(),
                                             _reception.get(),
                                             _triage.get(),
                                             _doctors.get(),
                                             _icu.get(),
                                             _hospital_props });

    // Create the package provider and receiver
    _provider = std::make_unique<agent_provider>(&_context, _communicator);
    _receiver = std::make_unique<agent_receiver>(&_context, _agent_factory.get(), _communicator);

    // Create the entry logic, if the entry is in this process, and send the
    // rest of the processes the ticks to execute
    const auto en = _hospital.entry();
    if (_spaces.local_dimensions().contains(std::vector { en.location.x, en.location.y })) {
        auto       patient_distribution = load_patient_distribution(_hospital_props);
        const auto days                 = patient_distribution->days();
        _entry.reset(new sti::hospital_entry { en.location, _clock.get(), std::move(patient_distribution), _agent_factory.get(), _hospital_props });

        // Calculate how many ticks and broadcast to the rest
        const auto seconds_per_tick = boost::lexical_cast<int>(_props->getProperty("seconds.per.tick"));
        const auto ticks            = (days * 86400) / seconds_per_tick;
        _stop_at                    = ticks - 1;

        // Note: can't use broadcast because it requires the "root" process
        // sending the message, which is unknown
        for (auto p = 0; p < _communicator->size(); p++) {
            if (p != _communicator->rank()) {
                _communicator->send(p, 3854, _stop_at);
            }
        }
    } else {
        // Wait for the stop value
        _communicator->recv(boost::mpi::any_source, 3854, _stop_at);
    }

    // Create the exit, if the exit is in this process
    const auto ex = _hospital.exit();
    if (_spaces.local_dimensions().contains(std::vector { ex.location.x, ex.location.y })) {
        _exit.reset(new sti::hospital_exit(&_context, &_spaces, _clock.get(), ex.location));
    }

    // Create medical personnel
    _staff_manager->create_staff(*_agent_factory, _spaces, _hospital, _hospital_props);

    // Create the beds
    if (_icu->get_real_icu()) {
        _icu->get_real_icu()->get().create_beds(*(_agent_factory->get_infection_factory()));
    }

    // Create the chairs
    _chair_manager->create_chairs(_hospital, *_agent_factory->get_infection_factory());

    // Reserve vectors to avoid reallocations
    _pmetrics->preallocate(static_cast<std::size_t>(_stop_at));
    _stats->preallocate_ticks(static_cast<std::size_t>(_stop_at));
} // sti::model::init()

/// @brief Initialize the scheduler
/// @param runner The repast schedule runner
void sti::model::init_schedule(repast::ScheduleRunner& runner)
{
    runner.scheduleEvent(1, 1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<model>(this, &model::tick)));
    runner.scheduleEndEvent(repast::Schedule::FunctorPtr(new repast::MethodFunctor<model>(this, &model::finish)));
    runner.scheduleStop(_stop_at);
}

/// @brief Periodic funcion
void sti::model::tick()
{
    _pmetrics->new_tick();

    // Sync the clock with the simulation tick
    _clock->sync(repast::RepastProcess::instance()->getScheduleRunner().currentTick());

    _stats->new_tick(_clock->now());

    ////////////////////////////////////////////////////////////////////////////
    // INTER-PROCESS SYNCHRONIZATION
    ////////////////////////////////////////////////////////////////////////////

    _chair_manager->sync();
    _pmetrics->finish_mpi_stage<0>();
    _reception->sync();
    _pmetrics->finish_mpi_stage<1>();
    _triage->sync();
    _pmetrics->finish_mpi_stage<2>();
    _doctors->queues()->sync();
    _pmetrics->finish_mpi_stage<3>();
    _icu->admission().sync();
    _pmetrics->finish_mpi_stage<4>();

    _spaces.balance(); // Move the agents accross processes
    repast::RepastProcess::instance()->synchronizeAgentStatus<sti::contagious_agent, agent_package, agent_provider, agent_receiver>(_context, *_provider, *_receiver, *_receiver);
    repast::RepastProcess::instance()->synchronizeProjectionInfo<sti::contagious_agent, agent_package, agent_provider, agent_receiver>(_context, *_provider, *_receiver, *_receiver);
    repast::RepastProcess::instance()->synchronizeAgentStates<agent_package, agent_provider, agent_receiver>(*_provider, *_receiver);
    _pmetrics->finish_rhpc_sync();

    ////////////////////////////////////////////////////////////////////////////
    // LOGIC
    ////////////////////////////////////////////////////////////////////////////

    if (_entry) _entry->generate_patients();
    if (_exit) _exit->tick();
    if (_icu->get_real_icu()) _icu->get_real_icu()->get().tick();
    _chair_manager->tick();

    // Check how many agents are currently in this process
    _pmetrics->agents(_context.size()); // Add the metric
    _stats->preallocate_agents(static_cast<std::size_t>(_context.size()));

    // Iterate over all the agents
    for (auto it = _context.localBegin(); it != _context.localEnd(); ++it) {
        (*it)->act();

        // Add the location to the log
        _stats->add_agent_location((**it).getId(), _spaces.get_continuous_location((**it).getId()));
    }
    _pmetrics->finish_logic();

    _pmetrics->tick_end();
}

/// @brief Final function for data collection and such
void sti::model::finish()
{
    _pmetrics->start_save();

    // The rank 0 creates the folder and broadcasts it
    const auto& folderpath = _props->getProperty("output.folder");

    if (_exit) _exit->save(folderpath, _rank);
    if (_entry) _entry->save(folderpath, _communicator->rank());
    _triage->save(folderpath);
    _icu->save(folderpath);
    _chair_manager->save(folderpath, _communicator->rank());
    _staff_manager->save(folderpath, _communicator->rank());
    _stats->save(folderpath, _communicator->rank());
    _hospital.get_pathfinder()->save(folderpath, _communicator->rank());

    // Remove the remaining agents
    // Iterate over all the agents to perform their actions
    remove_remnants(folderpath);

    _pmetrics->save(folderpath, _communicator->rank());
}

/// @brief Remove all the agents that are still in the simulation
/// @details Remove all the agents in the simulation and collect their
/// metrics into a file
void sti::model::remove_remnants(const std::string& folderpath)
{
    auto to_remove = std::vector<repast::AgentId> {};

    auto output = boost::json::array {};
    for (auto it = _context.localBegin(); it != _context.localEnd(); ++it) {
        output.push_back((**it).stats());
        to_remove.push_back((**it).getId());
    }

    for (const auto& id : to_remove) {
        _context.removeAgent(id);
    }

    auto os = std::ostringstream {};
    os << folderpath
       << "/agents.p"
       << _communicator->rank()
       << ".json";

    auto file = std::ofstream { os.str() };

    file << output;
}