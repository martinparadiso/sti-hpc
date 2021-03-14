#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpi/communicator.hpp>
#include <fstream>
#include <iomanip>
#include <memory>

#include <repast_hpc/RepastProcess.h>
#include <repast_hpc/Schedule.h>
#include <sstream>
#include <string>

#include "agent_factory.hpp"
#include "agent_package.hpp"
#include "chair_manager.hpp"
#include "clock.hpp"
#include "contagious_agent.hpp"
#include "entry.hpp"
#include "exit.hpp"
#include "hospital_plan.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "infection_logic/object_infection_cycle.hpp"
#include "model.hpp"
#include "patient.hpp"
#include "print.hpp"

/// @brief Initialize the model
/// @details Loads the map
void sti::model::init()
{

    // Create the chair manager
    const auto chair_mgr_rank = boost::lexical_cast<int>(_props->getProperty("chair.manager.rank"));
    if (_rank == chair_mgr_rank) {
        // The chair manager is in this process, create the real one
        print("Creating chair manager server");
        _chair_manager.reset(new real_chair_manager { _communicator, _hospital });
    } else {
        print("Creating chair manager proxy");
        _chair_manager.reset(new proxy_chair_manager { _communicator, chair_mgr_rank });
    }

    // Create the agent factory
    print("Creating the agent factory...");
    _agent_factory.reset(new agent_factory { &_context,
                                             &_spaces,
                                             _clock.get(),
                                             &_hospital,
                                             _chair_manager.get(),
                                             _hospital_props });

    // Create the package provider and receiver
    _provider = std::make_unique<agent_provider>(&_context);
    _receiver = std::make_unique<agent_receiver>(&_context, _agent_factory.get());

    // Create the entry logic, if the entry is in this process, and send the
    // rest of the processes the ticks to execute
    const auto en = _hospital.get_all(hospital_plan::tile_t::ENTRY).at(0);
    if (_spaces.local_dimensions().contains(std::vector { en.x, en.y })) {
        print("Creating entry...");
        auto       patient_distribution = load_patient_distribution(_hospital_props);
        const auto days                 = patient_distribution->days();
        _entry.reset(new sti::hospital_entry { en, _clock.get(), std::move(patient_distribution), _agent_factory.get(), _hospital_props });

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
    const auto ex = _hospital.get_all(hospital_plan::tile_t::EXIT).at(0);
    if (_spaces.local_dimensions().contains(std::vector { ex.x, ex.y })) {
        print("Creating exit...");
        _exit.reset(new sti::hospital_exit(&_context, &_spaces, _clock.get(), ex));
    }

    // Create chairs (if the chair is in the local space)
    const auto& chairs = _hospital.get_all(hospital_plan::tile_t::CHAIR);
    for (const auto& [x, y] : chairs) {
        const auto chair_loc = repast::Point<int>(static_cast<int>(x), static_cast<int>(y));
        if (_spaces.local_dimensions().contains(chair_loc)) {
            _agent_factory->insert_new_object({ x, y }, object_infection_cycle::STAGE::CLEAN);
        }
    }

    // Create medical personnel
    auto        personnel = _hospital.get_all(hospital_plan::tile_t::DOCTOR);
    const auto& recept    = _hospital.get_all(hospital_plan::tile_t::RECEPTIONIST);
    personnel.insert(personnel.end(), recept.begin(), recept.end());
    for (const auto& [x, y] : personnel) {
        const auto per_loc = repast::Point<int>(static_cast<int>(x), static_cast<int>(y));
        if (_spaces.local_dimensions().contains(per_loc)) {
            _agent_factory->insert_new_person({ x, y }, human_infection_cycle::STAGE::HEALTHY);
        }
    }

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
    // Sync the clock, print time
    _clock->sync();
    if (_rank == 0) print(_clock->now().str());

    ////////////////////////////////////////////////////////////////////////////
    // INTER-PROCESS SYNCHRONIZATION
    ////////////////////////////////////////////////////////////////////////////

    _chair_manager->sync(); // Sync the chair pool
    _spaces.balance(); // Move the agents accross processes

    repast::RepastProcess::instance()->synchronizeAgentStatus<sti::contagious_agent, agent_package, agent_provider, agent_receiver>(_context, *_provider, *_receiver, *_receiver);
    repast::RepastProcess::instance()->synchronizeProjectionInfo<sti::contagious_agent, agent_package, agent_provider, agent_receiver>(_context, *_provider, *_receiver, *_receiver);
    repast::RepastProcess::instance()->synchronizeAgentStates<agent_package, agent_provider, agent_receiver>(*_provider, *_receiver);

    // Check if agents are pending creation
    if (_entry) {
        _entry->generate_patients();
    }

    // Check if agents are pending exit
    if (_exit) {
        _exit->tick();
    }

    // Iterate over all the agents to perform their actions
    for (auto it = _context.localBegin(); it != _context.localEnd(); ++it) {
        (*it)->act();
    }
}

/// @brief Final function for data collection and such
void sti::model::finish()
{

    if (_rank == 0) {
        _props->putProperty("Result", "Passed");

        auto key_oder = std::vector<std::string> { "RunNumber", "stop.at", "Result" };
        _props->writeToSVFile("./output/results.csv", key_oder);
    }
    if (_exit) {
        const auto& data      = _exit->finish();
        auto        exit_file = std::ofstream { "./exit.json" };
        exit_file << data;
        exit_file.close();
    }

    // Transmit the entry information
    if (_entry) {
        const auto& data       = boost::json::serialize(_entry->statistics());
        auto        entry_file = std::ofstream { "./entry.str" };
        entry_file << data;
    }
}