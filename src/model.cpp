#include <memory>

#include <repast_hpc/Schedule.h>

#include "clock.hpp"
#include "entry.hpp"
#include "model.hpp"
#include "plan/plan_file.hpp"

/// @brief Initialize the model
/// @details Loads the map
void sti::model::init()
{

    // Load the building plan
    const auto plan_path = _props->getProperty("plan.path");
    _plan                = load_plan(plan_path);

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

    // Check if the entry is in this process
    const auto en = _plan.get(plan_tile::TILE_ENUM::ENTRY).at(0);
    if (_discrete_space->dimensions().contains(std::vector { static_cast<int>(en.x), static_cast<int>(en.y) })) {
        auto patient_distribution = load_patient_distribution(_props->getProperty("patients.path"));
        _entry = std::make_unique<hospital_entry>(_clock.get(), std::move(patient_distribution));
    }

    // TODO: Check the rest of the stuff?
}

/// @brief Initialize the scheduler
/// @param runner The repast schedule runner
void sti::model::init_schedule(repast::ScheduleRunner& runner) {
    runner.scheduleEvent(1, 1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<model>(this, &model::tick)));
    runner.scheduleEndEvent(repast::Schedule::FunctorPtr(new repast::MethodFunctor<model>(this, &model::finish)));
    runner.scheduleStop(_stop_at);
}

/// @brief Periodic funcion
void sti::model::tick() {
    // TODO: Everything
}

/// @brief Final function for data collection and such
void sti::model::finish() {

    if (_rank == 0) {
        _props->putProperty("Result", "Passed");
        
        auto key_oder = std::vector<std::string>{"RunNumber", "stop.at", "Result"};
        _props->writeToSVFile("./output/results.csv", key_oder);
    }

}
