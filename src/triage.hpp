/// @file triage.hpp
/// @brief Implements the triage queue
#pragma once

#include <boost/mpi/communicator.hpp>
#include <map>
#include <memory>

#include "clock.hpp"
#include "coordinates.hpp"
#include "queue_manager.hpp"

// Fw. declarations
namespace boost {
template <typename T>
class optional;
namespace json {
    class object;
} // namespace boost
} // namespace boost

namespace repast {
class Properties;
class AgentId;
} // namespace repastr

namespace sti {
class hospital_plan;
} // namespace sti

namespace sti {

class triage {

public:
    using agent_id                = repast::AgentId;
    using properties_type         = repast::Properties;
    using communicator_ptr        = boost::mpi::communicator*;
    using probability_precission  = double;
    using doctor_type             = std::string;
    using attention_waittime_type = sti::datetime;
    using triage_level_type       = std::int32_t;

    /// @brief Triage statistics: patients assigned and such
    struct statistic;

    struct triage_diagnosis {

        /// @brief Represent the two possible outcomes of the triage: go to a doctor or ICU
        enum class DIAGNOSTIC {
            DOCTOR,
            ICU
        }; // enum class DIAGNOSE

        DIAGNOSTIC              area_assigned;
        doctor_type             doctor_assigned;
        attention_waittime_type attention_time_limit;

        static auto icu()
        {
            return triage_diagnosis { DIAGNOSTIC::ICU, "", attention_waittime_type {} };
        }

        static auto doctor(const doctor_type& type, const attention_waittime_type& wt)
        {
            return triage_diagnosis { DIAGNOSTIC::DOCTOR, type, wt };
        }

        template <typename Archive>
        void serialize(Archive& ar, const unsigned int /*unused*/)
        {
            ar& area_assigned;
            ar& doctor_assigned;
            ar& attention_time_limit;
        } // void serialize()

    }; // struct triage_diagnosis

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a triage
    /// @param execution_props The execution/repast properties
    /// @param hospital_props The hospital properties
    /// @param comm MPI Communicator
    /// @param clock The simulation clock
    /// @param plan Hospital plan
    triage(const properties_type&     execution_props,
           const boost::json::object& hospital_props,
           communicator_ptr           comm,
           const clock*               clock,
           const hospital_plan&       plan);

    triage(const triage&) = delete;
    triage& operator=(const triage&) = delete;

    triage(triage&&) = delete;
    triage& operator=(triage&&) = delete;

    ~triage();

    ////////////////////////////////////////////////////////////////////////////
    // QUEUE MANAGEMENT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Enqueue an agent into the triage queue
    /// @param id The agent id
    void enqueue(const agent_id& id);

    /// @brief Remove an agent into the triage queue
    /// @param id The agent id
    void dequeue(const agent_id& id);

    /// @brief Check if an agent has a triage assigned
    /// @param id The id of the agent to check
    /// @return If the agent has a triage assigned, the triage location
    boost::optional<sti::coordinates<double>> is_my_turn(const agent_id& id);

    /// @brief Synchronize the queues between the process
    void sync();

    ////////////////////////////////////////////////////////////////////////////
    // REAL TRIAGE BEHAVIOR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Diagnose a patient, randomly select a doctor or ICU
    /// @return The diagnostic
    triage_diagnosis diagnose();

    ////////////////////////////////////////////////////////////////////////////
    // SAVE STATISTICS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Save the stadistics/metrics to a file
    /// @param filepath The path to the folder where
    void save(const std::string& folderpath) const;

private:
    std::unique_ptr<sti::queue_manager> _queue_manager;
    int                                 _this_rank;
    const clock*                        _clock;

    std::unique_ptr<statistic> _stats;

    probability_precission                        _icu_probability;
    std::map<doctor_type, probability_precission> _doctors_probabilities;

    std::map<triage_level_type, probability_precission> _levels_probabilities;
    std::map<triage_level_type, timedelta>              _levels_time_limit;

}; // class triage

} // namespace sti

// Enum serialization
namespace boost {
namespace serialization {
    template <typename Archive>
    void serialize(Archive& ar, sti::triage::triage_diagnosis::DIAGNOSTIC diagnose, const unsigned int /*unsued*/)
    {
        ar& diagnose;
    } // void serialize()
} // namespace serialization
} // namespace boost
