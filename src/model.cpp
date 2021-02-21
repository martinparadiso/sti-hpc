#include <boost/lexical_cast.hpp>
#include <memory>

#include <repast_hpc/Schedule.h>
#include <string>

#include "agent_creator.hpp"
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

    // TODO: Load the building plan here?
    // print("Loading plan...");
    // const auto plan_path = _props->getProperty("plan.path");
    // _plan                = load_plan(plan_path);

    // Get the process local dimensions (the dimensions executed by this process)
    const auto local_dims = _discrete_space->dimensions();
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

    // Create the entry logic, if the entry is in this process
    print("Loading entry...");
    const auto en = _plan.get(plan_tile::TILE_ENUM::ENTRY).at(0);
    if (_discrete_space->dimensions().contains(std::vector { static_cast<int>(en.x), static_cast<int>(en.y) })) {
        auto patient_distribution = load_patient_distribution(_props->getProperty("patients.path"));
        _entry                    = std::make_unique<hospital_entry>(_clock.get(), std::move(patient_distribution));
    }

    print("Creating the agent factory...");
    // Create the agent factory
    const auto inf_factory = infection_factory {
        human_infection_cycle::flyweight {
            _discrete_space,
            _clock.get(),
            boost::lexical_cast<sti::infection_cycle::precission>(_props->getProperty("human.infection.chance")),
            boost::lexical_cast<int>(_props->getProperty("human.infection.distance")),
            boost::lexical_cast<clock::date_t::resolution>(_props->getProperty("human.incubation.time")) },
        object_infection_cycle::flyweight {
            _discrete_space,
            boost::lexical_cast<sti::infection_cycle::precission>(_props->getProperty("object.infection.chance")) }
    };
    _agent_factory = std::make_unique<agent_factory>(&_context,
                                                     _discrete_space,
                                                     _clock.get(),
                                                     patient_agent::flyweight {},
                                                     person_agent::flyweight {},
                                                     object_agent::flyweight {},
                                                     inf_factory);

    // Create the package provider and receiver
    _provider = std::make_unique<agent_provider>(&_context);
    _receiver = std::make_unique<agent_receiver>(&_context, _agent_factory.get());

    // TODO: Remove this, it's a test
    print("Inserting test person...");
    _agent_factory->insert_new_person({ 10, 10 }, human_infection_cycle::STAGE::HEALTHY);
    _agent_factory->insert_new_person({ 10, 11 }, human_infection_cycle::STAGE::SICK);
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
    // TODO: Everything

    // Sync the clock, print time
    _clock->sync();
    if (_rank == 0) print(_clock->now().get_fancy().str());

    // Iterate over all the agents to perform their actions
    for (const auto& agent : _context) {
        agent->act();
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
}
