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
#include "doctors.hpp"
#include "doctors_queue.hpp"
#include "entry.hpp"
#include "exit.hpp"
#include "hospital_plan.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "infection_logic/object_infection_cycle.hpp"
#include "json_loader.hpp"
#include "json_serialization.hpp"
#include "model.hpp"
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
struct sti::process_metrics {

    constexpr static auto mpi_stages = 5;

    /// @brief The metrics to collect
    struct metrics {
        std::int32_t                         current_agents; // Number of agents in this process
        std::array<std::int64_t, mpi_stages> mpi_sync_ns; // Nanoseconds spent in non-repast sync, one entry for each sync
        std::int64_t                         rhpc_sync_ns; // Nanoseconds spent in Repast sync
        std::int64_t                         logic_ns; // Nanoseconds spent executing agent logic
        std::int64_t                         tick_start_time; // The instant of time the tick started executing, epoch is the start of the simulation
    };

    /// @brief
    process_metrics(std::int64_t                               init_time,
                    const std::array<std::string, mpi_stages>& mpi_stages_tags)
        : simulation_epoch { init_time }
        , values {}
        , mpi_stages_tags { mpi_stages_tags }
    {
    }

    std::int64_t                        simulation_epoch;
    std::vector<metrics>                values;
    std::array<std::string, mpi_stages> mpi_stages_tags;

    /// @brief Save the metrics to a file as a csv
    /// @param path The folder to save the metrics to
    /// @param process The process number
    void save(const std::string& folder, int process) const
    {
        auto filepath = std::ostringstream {};
        filepath << folder << "/profiling.p" << process << ".csv";
        auto file = std::ofstream { filepath.str() };

        file << "tick,"
             << "agents,";
        for (const auto& tag : mpi_stages_tags) file << tag << "_sync_ns,";
        file << "rhpc_sync_ns,"
             << "logic_ns,"
             << "tick_start_time\n";

        auto i = 0U;
        for (const auto& metric : values) {
            file << i++ << ","
                 << metric.current_agents << ",";
            for (const auto& mpi_value : metric.mpi_sync_ns) file << mpi_value << ",";
            file << metric.rhpc_sync_ns << ","
                 << metric.logic_ns << ","
                 << metric.tick_start_time
                 << "\n";
        }
    }
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
    , _hospital { _hospital_props }
    , _spaces { _hospital, *_props, _context, comm }
    , _clock { std::make_unique<clock>(boost::lexical_cast<std::uint64_t>(_props->getProperty("seconds.per.tick"))) }
    , _pmetrics { new process_metrics { now_in_ns(), {
                                                         "chairs",
                                                         "reception",
                                                         "triage",
                                                         "doctors",
                                                         "icu",
                                                     } } }
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
    const auto chair_mgr_rank = boost::lexical_cast<int>(_props->getProperty("chair.manager.rank"));
    if (_rank == chair_mgr_rank) {
        // The chair manager is in this process, create the real one
        _chair_manager.reset(new real_chair_manager { _communicator, _hospital });
    } else {
        _chair_manager.reset(new proxy_chair_manager { _communicator, chair_mgr_rank });
    }

    // Create the reception, triage and doctors
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

    // Create chairs (if the chair is in the local space)
    for (const auto& chair : _hospital.chairs()) {
        if (_spaces.local_dimensions().contains(chair.location)) {
            _agent_factory->insert_new_object("chair", chair.location.continuous(), object_infection_cycle::STAGE::CLEAN);
        }
    }

    // Create medical personnel
    const auto& doctors      = _hospital.doctors();
    const auto& receptionits = _hospital.receptionists();

    for (const auto& doctor : doctors) {
        if (_spaces.local_dimensions().contains(doctor.location)) {
            _agent_factory->insert_new_person(doctor.location.continuous(), human_infection_cycle::STAGE::HEALTHY, false);
        }
    }
    for (const auto& receptionist : receptionits) {
        if (_spaces.local_dimensions().contains(receptionist.location)) {
            _agent_factory->insert_new_person(receptionist.location.continuous(), human_infection_cycle::STAGE::HEALTHY, false);
        }
    }

    // Create the beds
    if (_icu->get_real_icu()) {
        _icu->get_real_icu()->get().create_beds(*(_agent_factory->get_infection_factory()));
    }

    // Preallocate the metrics vector
    _pmetrics->values.reserve(static_cast<decltype(_pmetrics->values)::size_type>(_stop_at));
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
    auto& new_metric           = _pmetrics->values.emplace_back(process_metrics::metrics {});
    new_metric.tick_start_time = now_in_ns() - _pmetrics->simulation_epoch;

    // Sync the clock with the simulation tick
    _clock->sync();

    ////////////////////////////////////////////////////////////////////////////
    // INTER-PROCESS SYNCHRONIZATION
    ////////////////////////////////////////////////////////////////////////////

    const auto pre_mpi_time = now_in_ns();

    _chair_manager->sync();
    new_metric.mpi_sync_ns.at(0) = now_in_ns();
    _reception->sync();
    new_metric.mpi_sync_ns.at(1) = now_in_ns();
    _triage->sync();
    new_metric.mpi_sync_ns.at(2) = now_in_ns();
    _doctors->queues()->sync();
    new_metric.mpi_sync_ns.at(3) = now_in_ns();
    _icu->admission().sync();
    new_metric.mpi_sync_ns.at(4) = now_in_ns();

    for (auto i = process_metrics::mpi_stages - 1UL; i > 0; --i) {
        new_metric.mpi_sync_ns.at(i) -= new_metric.mpi_sync_ns.at(i - 1);
    }
    new_metric.mpi_sync_ns.at(0) -= pre_mpi_time;

    const auto pre_rhpc_time = now_in_ns();
    _spaces.balance(); // Move the agents accross processes
    repast::RepastProcess::instance()->synchronizeAgentStatus<sti::contagious_agent, agent_package, agent_provider, agent_receiver>(_context, *_provider, *_receiver, *_receiver);
    repast::RepastProcess::instance()->synchronizeProjectionInfo<sti::contagious_agent, agent_package, agent_provider, agent_receiver>(_context, *_provider, *_receiver, *_receiver);
    repast::RepastProcess::instance()->synchronizeAgentStates<agent_package, agent_provider, agent_receiver>(*_provider, *_receiver);
    new_metric.rhpc_sync_ns = now_in_ns() - pre_rhpc_time;

    ////////////////////////////////////////////////////////////////////////////
    // LOGIC
    ////////////////////////////////////////////////////////////////////////////

    const auto pre_logic_time = now_in_ns();
    if (_entry) _entry->generate_patients();
    if (_exit) _exit->tick();
    if (_icu->get_real_icu()) _icu->get_real_icu()->get().tick();

    // Iterate over all the agents to perform their actions
    for (auto it = _context.localBegin(); it != _context.localEnd(); ++it) {
        (*it)->act();
    }
    new_metric.logic_ns = now_in_ns() - pre_logic_time;

    // Rest of the metrics
    new_metric.current_agents = _context.size();
}

/// @brief Final function for data collection and such
void sti::model::finish()
{
    // Create the folder to output the results
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto       os  = std::ostringstream {};
    os << _props->getProperty("output.folder")
       << std::put_time(std::localtime(&now), "%Y-%m-%dT%H:%M:%S")
       << "/";
    const auto folderpath = os.str();

    std::filesystem::create_directories(folderpath);

    if (_rank == 0) {
        _props->putProperty("Result", "Passed");

        auto key_oder = std::vector<std::string> { "RunNumber", "stop.at", "Result" };
        _props->writeToSVFile("./output/results.csv", key_oder);
    }
    if (_exit) {
        _exit->save(folderpath, _rank);
    }

    // Transmit the entry information
    if (_entry) {
        const auto& data       = boost::json::serialize(_entry->statistics());
        auto        entry_file = std::ofstream { "../output/entry.json" };
        entry_file << data;
    }

    _triage->save(folderpath);
    _icu->save(folderpath);
    _pmetrics->save(folderpath, _communicator->rank());

    // Remove the remaining agents
    // Iterate over all the agents to perform their actions
    remove_remnants(folderpath);
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
       << "/agents_in_process_"
       << _communicator->rank()
       << ".json";

    auto file = std::ofstream { os.str() };

    file << output;
}