#include "../doctors.hpp"

#include <boost/json/object.hpp>
#include <boost/mpi/communicator.hpp>
#include <cstdint>
#include <fstream>
#include <fstream>
#include <map>
#include <memory>
#include <repast_hpc/Properties.h>
#include <repast_hpc/Random.h>
#include <sstream>

#include "../hospital_plan.hpp"
#include "../json_serialization.hpp"
#include "real_doctors.hpp"
#include "proxy_doctors.hpp"

/// @brief Construct a doctors manager
/// @param execution_props Execution properties stored in repast::Properties
/// @param simulation_props Simulation properties, from the hospital.json
/// @param communicator The MPI communicator
/// @param hospital_plan The hospital plan
sti::doctors::doctors(repast::Properties&        execution_props,
                      const boost::json::object& simulation_props,
                      communicator_ptr           communicator,
                      const hospital_plan&       hospital_plan)
    : _this_rank { communicator->rank() }
    , _attention_time { [&]() {
        auto       attention = decltype(_attention_time) {};
        const auto data      = simulation_props.at("parameters").at("doctors").as_object();
        for (const auto& [key, value] : data) {
            attention[key.to_string()] = boost::json::value_to<timedelta>(value.at("attention_duration"));
        }
        return attention;
    }() }
    , _doctors { [&]() -> decltype(_doctors) {
        // This is the manager
        const auto real_rank = boost::lexical_cast<int>(execution_props.getProperty("doctors.manager.rank"));
        if (communicator->rank() == real_rank) {
            return std::make_unique<real_doctors>(communicator, 4322, hospital_plan);
        }
        return std::make_unique<proxy_doctors>(communicator, real_rank, 4322);
    }() }
{
}

sti::doctors::~doctors() = default;

/// @brief Get the time period a takes a certain doctor appointment/attention
/// @param type The doctor type to get the attention time
/// @return The time period
sti::timedelta sti::doctors::get_attention_duration(const doctor_type& type) const
{
    return _attention_time.at(type);
}

/// @brief Get the doctors queue
/// @return A pointer to the doctor queues
sti::doctors_queue* sti::doctors::queues()
{
    return _doctors.get();
}
