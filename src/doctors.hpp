/// @file doctors.hpp
/// @brief Doctor dispatcher and queues
#pragma once

#include <memory>
#include <map>

namespace repast {
class Properties;
}

namespace boost {
namespace json {
    class object;
} // namespace json
namespace mpi {
    class communicator;
} // namespace communicator
} // namespace boost

namespace sti {
class timedelta;
class doctors_queue;
class hospital_plan;
} // namespace sti

namespace sti {

class doctors {
public:
    using doctor_type      = std::string;
    using communicator_ptr = boost::mpi::communicator*;
    using prob_precission  = double;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a doctors manager
    /// @param execution_props Execution properties stored in repast::Properties
    /// @param hospital_props Hospital properties, form the JSON file
    /// @param communicator The MPI communicator
    /// @param hospital_plan The hospital plan
    doctors(repast::Properties&        execution_props,
            const boost::json::object& hospital_props,
            communicator_ptr           communicator,
            const hospital_plan&       hospital_plan);

    doctors(const doctors&) = delete;
    doctors& operator=(const doctors&) = delete;

    doctors(doctors&&) = default;
    doctors& operator=(doctors&&) = default;

    ~doctors();

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////
    
    /// @brief Get the time period a takes a certain doctor appointment/attention
    /// @param type The doctor type to get the attention time
    /// @return The time period
    timedelta get_attention_duration(const doctor_type& type) const;

    /// @brief Get the doctors queue
    /// @return A pointer to the doctor queues
    doctors_queue* queues();

private:
    int                                    _this_rank;
    std::map<doctor_type, timedelta>       _attention_time;
    std::unique_ptr<doctors_queue>         _doctors;
}; // class doctors

} // namespace sti