#include "triage.hpp"

#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <numeric>
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
#include "utils.hpp"

////////////////////////////////////////////////////////////////////////////////
// STATISTICS
////////////////////////////////////////////////////////////////////////////////

/// @brief Triage statistics: patients assigned and such
struct sti::triage::statistic {

    using counter_type = std::uint32_t;

    std::map<timedelta, counter_type>                                icu_diagnostics;
    counter_type                                                     icu_deaths {};
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
    , _this_rank { comm->rank() }
    , _clock { clock }
    , _stats { std::make_unique<statistic>() }
    , _icu_probability { [&]() {
        return hospital_props.at("parameters").at("triage").at("icu").at("probability").as_double();
    }() }
    , _doctors_probabilities { [&]() {
        auto       probs = decltype(_doctors_probabilities) {};
        const auto data  = hospital_props.at("parameters").at("triage").at("doctors_probabilities").as_array();
        for (const auto& value : data) {
            const auto specialty   = boost::json::value_to<std::string>(value.at("specialty"));
            const auto probability = value.at("probability").as_double();
            probs.push_back({ specialty, probability });
        }
        return probs;
    }() }
    , _levels_probabilities { [&]() {
        auto       probs = decltype(_levels_probabilities) {};
        const auto data  = hospital_props.at("parameters").at("triage").at("levels").as_array();
        for (const auto& element : data) {
            const auto level       = boost::json::value_to<int>(element.at("level"));
            const auto probability = element.at("probability").as_double();
            probs.push_back({ level, probability });
        }
        return probs;
    }() }
    , _levels_time_limit { [&]() {
        auto       times = decltype(_levels_time_limit) {};
        const auto data  = hospital_props.at("parameters").at("triage").at("levels").as_array();
        for (const auto& value : data) {
            const auto& level     = boost::json::value_to<int>(value.at("level"));
            const auto& wait_time = boost::json::value_to<timedelta>(value.at("wait_time"));
            times[level]          = wait_time;
        }
        return times;
    }() }
    , _icu_sleep_times { [&]() {
        auto       times = decltype(_icu_sleep_times) {};
        const auto data  = hospital_props.at("parameters").at("icu").at("sleep_times").as_array();

        for (const auto& entry : data) {

            const auto time        = boost::json::value_to<timedelta>(entry.at("time"));
            const auto probability = static_cast<probability_precission>(entry.at("probability").as_double());

            times.push_back({ time, probability });
        }

        return times;
    }() }
    , _icu_death_probability { [&]() {
        return hospital_props.at("parameters").at("triage").at("icu").at("death_probability").as_double();
    }() }
{
    // Make sure all probabilities sum 1
    sti::validate_distribution(
        _doctors_probabilities.begin(), _doctors_probabilities.end(),
        [](auto acc, auto val) { return acc + val.second; },
        "Triage diagnosis distribution",
        _icu_probability);

    sti::validate_distribution(
        _levels_probabilities.begin(), _levels_probabilities.end(),
        [](auto acc, const auto& val) { return acc + val.second; },
        "Triage level distribution");
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

namespace {

/// @brief Find the bucket/bin in which a value falls
/// @details Given a container with bin "tags" and the width of the bins, and a
/// value, try to find the bin in which the value falls. Note that a "sub"
/// histogram can be used, where probabilities sum < 1.0. If this is the case,
/// an offset must be provided satisfying the contition sum(p) + offset = 1.0;
/// in this case the parameter number *must* be greater than the offset, offset
/// is considered the missing bin [0, offset).
/// @tparam Value The "tag" type for a given bracket
/// @tparam Chance The type used for the probability
/// @param map The map containing the brackets (consisting of tag and probability)
/// @param number The number trying to fit in the probability
/// @param offset If the probabilities don't sum 1, an offset fullfiling P + offset = 1 must be provided
template <typename Iterable, typename Chance>
[[nodiscard]] auto find_bracket(const Iterable& container, Chance number, Chance offset = Chance(0.0))
{
    for (const auto& [value, chance] : container) {
        if (offset <= number && number <= offset + chance) {
            return value;
        }
        offset += chance;
    }
    const auto last = --container.end();
    return last->first;
}

} // namespace

/// @brief Diagnose a patient, randomly select a doctor or ICU
/// @return The diagnostic
sti::triage::triage_diagnosis sti::triage::diagnose()
{
    // Roll dice
    const auto random_dispatch = repast::Random::instance()->nextDouble();

    // If the number falls in the ICU bracket (the lower one), return an ICU
    // diagnostic
    if (random_dispatch <= _icu_probability) {
        const auto random_sleep_time = repast::Random::instance()->nextDouble();
        const auto sleep_time        = find_bracket(_icu_sleep_times, random_sleep_time);
        _stats->icu_diagnostics[sleep_time] += 1;

        const auto random_survives_chance = repast::Random::instance()->nextDouble();
        const auto survives               = random_survives_chance >= _icu_death_probability;
        if (!survives) _stats->icu_deaths += 1;

        return icu_diagnosis { sleep_time, survives };
    }

    // Otherwise check in which doctor it falls
    const auto doctor_assigned = find_bracket(_doctors_probabilities, random_dispatch, _icu_probability);

    const auto random_level       = repast::Random::instance()->nextDouble();
    const auto severity_diagnosed = find_bracket(_levels_probabilities, random_level);
    const auto attention_limit    = _clock->now() + _levels_time_limit.at(severity_diagnosed);

    // Increment the statistics
    _stats->doctors_diagnostics[doctor_assigned][severity_diagnosed] += 1;

    // Now we have all the information, return the doctor and the wait time
    return doctor_diagnosis { doctor_assigned, severity_diagnosed, attention_limit };
}

/// @brief Save the stadistics/metrics to a file
/// @param filepath The path to the folder where
void sti::triage::save(const std::string& folderpath) const
{
    // Generate the JSON
    auto jv = boost::json::value {
        { "icu", { { "deaths", _stats->icu_deaths }, { "times", _stats->icu_diagnostics } } },
        { "doctors", _stats->doctors_diagnostics }
    };

    // Construct the file path and open the file
    auto os = std::ostringstream {};
    os << folderpath
       << "/triage.p"
       << _this_rank
       << ".json";
    auto file = std::ofstream { os.str() };
    file << jv;
}
