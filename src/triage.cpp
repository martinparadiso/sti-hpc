#include "triage.hpp"

#include <boost/json/object.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdint>
#include <memory>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/Properties.h>
#include <repast_hpc/Random.h>
#include <string>
#include <vector>

#include "clock.hpp"
#include "coordinates.hpp"
#include "hospital_plan.hpp"
#include "json_serialization.hpp"
#include "queue_manager.hpp"
#include "queue_manager/proxy_queue_manager.hpp"
#include "queue_manager/real_queue_manager.hpp"

////////////////////////////////////////////////////////////////////////////////
// ADDITIONAL DEFINITONS
////////////////////////////////////////////////////////////////////////////////

/// @brief Error trying to serialize or deserialize an agent
struct wrong_probability_sum : public std::exception {

    std::string msg;

    wrong_probability_sum(std::string_view which, double value)
    {
        auto os = std::ostringstream {};
        os << "Exception: Wrong" << which << "probabilities: "
           << value;
        msg = os.str();
    };

    const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

////////////////////////////////////////////////////////////////////////////////
// STATISTICS
////////////////////////////////////////////////////////////////////////////////

/// @brief Triage statistics: patients assigned and such
struct sti::triage::statistic {

    using counter_type = std::uint32_t;

    counter_type                                                     icu_diagnostics;
    std::map<doctor_type, std::map<triage_level_type, counter_type>> doctors_diagnostics;
};

////////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////////

/// @brief Construct a triage
/// @param execution_props The execution/repast properties
/// @param hospital_props The hospital properties
/// @param comm MPI Communicator
/// @param clock The simulation clock
/// @param plan Hospital plan
sti::triage::triage(const properties_type&     execution_props,
                    const boost::json::object& hospital_props,
                    communicator_ptr           comm,
                    const clock*               clock,
                    const hospital_plan&       plan)
    : _queue_manager {
        [&]() -> queue_manager* {
            const auto manager_rank = boost::lexical_cast<int>(execution_props.getProperty("triage.manager.rank"));
            if (manager_rank == comm->rank()) {
                const auto locations = [&]() {
                    auto ret = std::vector<coordinates<double>> {};
                    for (const auto& triage : plan.triages()) {
                        ret.push_back(triage.location.continuous());
                    }
                    return ret;
                }();
                return new real_queue_manager { comm, 4542, locations };
            }
            return new proxy_queue_manager { comm, 4542, manager_rank };
        }()
    }
    , _this_rank{comm->rank()}
    , _clock { clock }
    , _stats{std::make_unique<statistic>()}
    , _icu_probability { [&]() {
        return hospital_props.at("parameters").at("triage").at("icu").at("chance").as_double();
    }() }
    , _doctors_probabilities { [&]() {
        auto       probs = decltype(_doctors_probabilities) {};
        const auto data  = hospital_props.at("parameters").at("triage").at("doctors_probabilities").as_object();
        for (const auto& [key, value] : data) {
            probs[key.to_string()] = value.at("chance").as_double();
        }
        return probs;
    }() }
    , _levels_probabilities { [&]() {
        auto       probs = decltype(_levels_probabilities) {};
        const auto data  = hospital_props.at("parameters").at("triage").at("levels").as_object();
        for (const auto& [key, value] : data) {
            probs[std::stoi(key.to_string())] = value.at("chance").as_double();
        }
        return probs;
    }() }
    , _levels_time_limit { [&]() {
        auto       times = decltype(_levels_time_limit) {};
        const auto data  = hospital_props.at("parameters").at("triage").at("levels").as_object();
        for (const auto& [key, value] : data) {
            times[std::stoi(key.to_string())] = boost::json::value_to<sti::timedelta>(value.at("wait_time"));
        }
        return times;
    }() }
{
    // Make sure all probabilities sum 1
    auto doc_acc = _icu_probability;
    for (const auto& [type, prob] : _doctors_probabilities) doc_acc += prob;
    if (doc_acc != 1.0) throw wrong_probability_sum { "icu/doctors", doc_acc };

    auto sev_acc = 0.0;
    for (const auto& [sev, prob] : _levels_probabilities) sev_acc += prob;
    if (sev_acc != 1.0) throw wrong_probability_sum { "triage levels", sev_acc };
}

sti::triage::~triage() = default;

////////////////////////////////////////////////////////////////////////////////
// QUEUE MANAGEMENT
////////////////////////////////////////////////////////////////////////////////

/// @brief Enqueue an agent into the triage queue
/// @param id The agent id
void sti::triage::enqueue(const agent_id& id)
{
    _queue_manager->enqueue(id);
}

/// @brief Remove an agent into the triage queue
/// @param id The agent id
void sti::triage::dequeue(const agent_id& id)
{
    _queue_manager->dequeue(id);
}

/// @brief Check if an agent has a triage assigned
/// @param id The id of the agent to check
/// @return If the agent has a triage assigned, the triage location
boost::optional<sti::coordinates<double>> sti::triage::is_my_turn(const agent_id& id)
{
    return _queue_manager->is_my_turn(id);
}

/// @brief Synchronize the queues between the process
void sti::triage::sync()
{
    _queue_manager->sync();
}

////////////////////////////////////////////////////////////////////////////////
// DIAGNOSTIC
////////////////////////////////////////////////////////////////////////////////

/// @brief Diagnose a patient, randomly select a doctor or ICU
/// @return The diagnostic
sti::triage::triage_diagnosis sti::triage::diagnose()
{
    // The algorithm simply generates a random number and checks in which
    // bracket it falls. All probabilities sum 1.0 and the random number is in
    // the range [0, 1)

    // Roll dice
    const auto random_dispatch = repast::Random::instance()->nextDouble();
    auto       base            = 0.0;

    // If the number falls in the ICU bracket (the lower one), return an ICU
    // diagnostic
    if (random_dispatch <= _icu_probability) {
        _stats->icu_diagnostics++;
        return triage::triage_diagnosis::icu();
    }

    // Otherwise start trying in the N doctors specialties generated
    base += _icu_probability;
    const auto doctor_assigned = [&]() {
        for (const auto& [type, chance] : _doctors_probabilities) {
            if (base <= random_dispatch && random_dispatch <= base + chance) {
                return type;
            }
            base += chance;
        }

        // If for some reason none of the probabilities matched, return the last
        // doctor in the map.
        const auto last = --_doctors_probabilities.end();
        return last->first;
    }();

    const auto random_level = repast::Random::instance()->nextDouble();
    base                    = 0.0;

    const auto severity_diagnosed = [&]() {
        for (const auto& [severity, chance] : _levels_probabilities) {
            if (base <= random_level && random_level <= base + chance) {
                return severity;
            }
            base += chance;
        }
        // If for some reason none of  the probabilities matched, return the
        // last severity
        const auto last = --_levels_probabilities.end();
        return last->first;
    }();

    const auto attention_limit = _clock->now() + _levels_time_limit.at(severity_diagnosed);

    // Increment the statistics
    _stats->doctors_diagnostics[doctor_assigned][severity_diagnosed] += 1;

    // Now we have all the information, return the doctor and the wait time
    return triage_diagnosis::doctor(doctor_assigned, attention_limit);
}


/// @brief Save the stadistics/metrics to a file
/// @param filepath The path to the folder where
void sti::triage::save(const std::string& folderpath) const
{
    // Construct the file path and open the file
    auto os = std::ostringstream {};
    os << folderpath
       << "/triage_in_process_"
       << _this_rank
       << ".csv";
    auto file = std::ofstream { os.str() };

    // Write the header
    file << "dispatched_to" << ","
         << "severity" << ","
         << "quantity" << "\n";

    // Write the number of ICU diagnosis
    file << "icu" << ","
         << "," 
         << _stats->icu_diagnostics << "\n";

    // Write the doctors and triage levels
    for (const auto& [doctor, severity_map] : _stats->doctors_diagnostics) {
        for (const auto& [severity, quantity] : severity_map) {
            file << doctor << ","
                 << severity << ","
                 << quantity << "\n";
        }
    }
}