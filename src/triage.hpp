/// @file triage.hpp
/// @brief Implements the triage queue
#pragma once

#include <boost/json/object.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/variant.hpp>
#include <map>
#include <memory>

#include "clock.hpp"
#include "coordinates.hpp"
#include "queue_manager.hpp"

// Fw. declarations
namespace boost {
template <typename T>
class optional;
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

    ////////////////////////////////////////////////////////////////////////////
    // DIAGNOSIS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Represents a doctor diagnosis, contains doctor assigned and priority
    struct doctor_diagnosis {
        doctor_type             doctor_assigned;
        triage_level_type       level;
        attention_waittime_type attention_time_limit;

        template <typename Archive>
        void serialize(Archive& ar, const unsigned int /*unused*/)
        {
            ar& doctor_assigned;
            ar& level;
            ar& attention_time_limit;
        } // void serialize(...)

        /// @brief Get a JSON object containing the information about this diagnosis
        boost::json::object stats() const
        {
            return {
                { "type", "doctor" },
                { "specialty", doctor_assigned },
                { "triage_level", level},
                { "attention_datetime_limit", attention_time_limit }
            };
        }

    }; // struct doctor

    /// @brief Represents a ICU diagnosis, contains the internation time and the outcome
    struct icu_diagnosis {
        timedelta sleep_time;
        bool      survives {};

        template <typename Archive>
        void serialize(Archive& ar, const unsigned int /*unused*/)
        {
            ar& sleep_time;
            ar& survives;
        }

        /// @brief Get a JSON object containing the information about this diagnosis
        boost::json::object stats() const
        {
            return {
                { "type", "icu" },
                { "sleep_time", sleep_time },
                { "survives", survives }
            };
        }

    }; // struct icu

    using triage_diagnosis = boost::variant<doctor_diagnosis, icu_diagnosis>;

    // Helper function because boost::variant doesn't have holds_alternative

    /// @brief Check if the variant holds and ICU diagnosis
    /// @return True if the variant holds an object of type icu_diagnosis
    static bool holds_doctor_diagnosis(const triage_diagnosis& td)
    {
        return td.which() == 0;
    }

    /// @brief Check if the variant holds and ICU diagnosis
    /// @return True if the variant holds an object of type icu_diagnosis
    static bool holds_icu_diagnosis(const triage_diagnosis& td)
    {
        return td.which() == 1;
    }

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

    probability_precission                                      _icu_probability;
    std::vector<std::pair<doctor_type, probability_precission>> _doctors_probabilities;

    std::vector<std::pair<triage_level_type, probability_precission>> _levels_probabilities;
    std::map<triage_level_type, timedelta>                            _levels_time_limit;

    std::vector<std::pair<timedelta, probability_precission>> _icu_sleep_times;
    probability_precission                                    _icu_death_probability;

}; // class triage

} // namespace sti

BOOST_CLASS_IMPLEMENTATION(sti::triage::doctor_diagnosis, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(sti::triage::doctor_diagnosis, boost::serialization::track_never)

BOOST_CLASS_IMPLEMENTATION(sti::triage::icu_diagnosis, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(sti::triage::icu_diagnosis, boost::serialization::track_never)