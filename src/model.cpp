#include <boost/histogram/axis/variant.hpp>
#include <boost/histogram/indexed.hpp>
#include <boost/histogram/ostream.hpp>
#include <boost/lexical_cast.hpp>
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
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "infection_logic/object_infection_cycle.hpp"
#include "model.hpp"
#include "patient.hpp"
#include "plan/plan_file.hpp"
#include "print.hpp"

/// @brief Initialize the model
/// @details Loads the map
void sti::model::init()
{

    // Get the process local dimensions (the dimensions executed by this process)
    const auto local_dims = _spaces.local_dimensions();
    const auto origin     = plan::dimensions {
        static_cast<plan::length_type>(local_dims.origin().getX()),
        static_cast<plan::length_type>(local_dims.origin().getY())
    };
    const auto extent = plan::dimensions {
        static_cast<plan::length_type>(local_dims.extents().getX()),
        static_cast<plan::length_type>(local_dims.extents().getY())
    };

    _local_zone = plan::zone {
        { origin.width, origin.height },
        { origin.width + extent.width, origin.height + origin.height }
    };

    // Create the chair manager
    const auto chair_mgr_rank = boost::lexical_cast<int>(_props->getProperty("chair.manager.rank"));
    if (_rank == chair_mgr_rank) {
        // The chair manager is in this process, create the real one
        print("Creating chair manager server");
        _chair_manager.reset(new real_chair_manager { _communicator, _plan });
    } else {
        print("Creating chair manager proxy");
        _chair_manager.reset(new proxy_chair_manager { _communicator, chair_mgr_rank });
    }

    // Create the agent factory
    print("Creating the agent factory...");
    _agent_factory.reset(new agent_factory { &_context,
                                             &_spaces,
                                             _clock.get(),
                                             &_plan,
                                             _chair_manager.get(),
                                             _props });

    // Create the package provider and receiver
    _provider = std::make_unique<agent_provider>(&_context);
    _receiver = std::make_unique<agent_receiver>(&_context, _agent_factory.get());

    // Create the entry logic, if the entry is in this process
    const auto en = _plan.get(plan_tile::TILE_ENUM::ENTRY).at(0);
    if (_spaces.local_dimensions().contains(std::vector { static_cast<int>(en.x), static_cast<int>(en.y) })) {
        print("Creating entry...");
        auto patient_distribution = load_patient_distribution(_props->getProperty("patients.file"));
        _entry.reset(new sti::hospital_entry { en, _clock.get(), std::move(patient_distribution), _agent_factory.get(), *_props });
    }

    // Create chairs (if the chair is in the local space)
    const auto& chairs = _plan.get(plan_tile::TILE_ENUM::CHAIR);
    for (const auto& [x, y] : chairs) {
        const auto chair_loc = repast::Point<int>(static_cast<int>(x), static_cast<int>(y));
        if (_spaces.local_dimensions().contains(chair_loc)) {
            _agent_factory->insert_new_object({ x, y }, object_infection_cycle::STAGE::CLEAN);
        }
    }

    // Create medical personnel
    auto        personnel = _plan.get(plan_tile::TILE_ENUM::DOCTOR);
    const auto& rist      = _plan.get(plan_tile::TILE_ENUM::RECEPTIONIST);
    personnel.insert(personnel.end(), rist.begin(), rist.end());
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
    // INTER-PROCESS SYNCHONIZATION
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

    // Get the information of the entry
    if (_entry) {
        const auto& [entry, expected] = _entry->stadistics();

        auto os = std::ostringstream {};

        // CSV Print
        os << "DAY,INTERVAL,PATIENTS_GENERATED,PATIENTS_EXPECTED\n";
        for (auto day = 0; day < entry.axis(0).size(); ++day) {
            for (auto bin = 0; bin < entry.axis(1).size(); ++bin) {
                os << day << ","
                   << bin << ","
                   << entry.at(day, bin) << ","
                   << expected.at(day, bin)
                   << "\n";
            }
        }

        print(os.str());
    }
}
