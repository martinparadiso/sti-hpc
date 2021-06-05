#include "entry.hpp"

#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/detail/value_to.hpp>
#include <cstdint>
#include <fstream>
#include <numeric>
#include <sstream>

#include "agent_factory.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "utils.hpp"

namespace {
struct inconsistent_distribution : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Wrong number of bins for a given day in the patient distribution";
    }
};

struct negative_patients : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Negative number of patients entering the hospital in file";
    }
};

struct inconsistent_bins_in_file : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Wrong number of bins for a given day in the patient distribution file";
    }
};

struct influx_and_infected_probability_differ : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Number of days in the influx distribution and the infected probability differ";
    }
};
}

////////////////////////////////////////////////////////////////////////////////
// PATIENT_DISTRIBUTION
////////////////////////////////////////////////////////////////////////////////

/// @brief Construct a patient influx distribution
/// @param patient_rate The influx of patients, a matrix where the first dimension is the day, and the second a fixed interval in the day
/// @param infected_chance A vector containing the probability per day of a patient being infected
sti::patient_distribution::patient_distribution(entry_rate_type&&           patient_rate,
                                                infected_probability_type&& infected_chance)
    : _data { std::move(patient_rate) }
    , _infected_chance { std::move(infected_chance) }
{
}

/// @brief Get the total number of patients that will enter the hospital
std::uint32_t sti::patient_distribution::total_patients() const
{
#include <iostream>

    // const auto total = _data.
    const auto acc = std::accumulate(_data.begin(), _data.end(), 0U,
                                     [](const auto& acc, const auto& val) {
                                         return acc + std::accumulate(val.begin(), val.end(), 0U, [](const auto& acc_day, const auto& val_day) {
                                                    return acc_day + val_day;
                                                });
                                     });

    return acc;
}

/// @brief Get the number of days the distribution cover
/// @return The number of days for which data is available
std::uint32_t sti::patient_distribution::days() const
{
    return static_cast<std::uint32_t>(_data.size());
}

/// @brief Get the number of bins/intervals in a day
/// @return The number of intervals in a day
std::uint32_t sti::patient_distribution::intervals() const
{
    return static_cast<std::uint32_t>(_data.at(0).size());
}

/// @brief Get the number of patients entering the hospital in a given interval
/// @param day The day
/// @param interval The interval of the day
/// @return The number of patients entering the hospital in the specified interval
std::uint32_t sti::patient_distribution::get(std::uint32_t day, std::uint32_t interval) const
{
    return static_cast<std::uint32_t>(_data.at(day).at(interval));
}

/// @brief Get the probability of a patient being infected in a given day
/// @param day The day for which the probability is required
/// @return A double in the range [0, 1), indicating the chance of a patient being infected
sti::probability_precission sti::patient_distribution::get_infected_probability(std::uint32_t day) const
{
    return _infected_chance.at(day);
}

////////////////////////////////////////////////////////////////////////////////
// HOSPITAL_ENTRY
////////////////////////////////////////////////////////////////////////////////

/// @brief Create a hospital entry
/// @details The hospital entry is in charge of creating the patients
/// @param location The location of the entry
/// @param clock The simulation clock
/// @param patient_admissions The patient admission histogram
/// @param factory The agent factory, for patient creation
sti::hospital_entry::hospital_entry(sti::coordinates<int>  location,
                                    sti::clock*            clock,
                                    patient_distribution&& patient_admissions,
                                    agent_factory*         factory)
    : _location { location }
    , _clock { clock }
    , _patient_distribution(std::move(patient_admissions))
    , _generated_patients (
        _patient_distribution.days(), std::vector<std::uint32_t> (_patient_distribution.intervals(), 0U )
    )
    // The length of the interval is the number of seconds in a day divided
    // by the number of intervals
    , _interval_length { (24 * 60 * 60) / _patient_distribution.intervals() }
    , _agent_factory { factory }
{
}

/// @brief Ask how many patients are waiting at the door, upon call the counter is cleared
/// @details Calculate how many patients must be created according to the
///          provided distribution. The function assumes the caller is going to create
///          those agents, the internal counter of created agents is increased in N.
/// @return The number of patients waiting admission
std::uint64_t sti::hospital_entry::patients_waiting()
{
    const auto now         = _clock->now().human();
    const auto day         = now.days;
    const auto day_seconds = now.hours * 60 * 60 + now.minutes * 60 + now.seconds;
    const auto bin         = day_seconds / _interval_length; // Calculate the corresponding bin for this instant
    const auto bin_offset  = day_seconds % _interval_length; // Seconds since the start of the corresponding interval

    // Get the number of patients expected for this interval and the rate of
    // admission
    const auto interval_admission_target = _patient_distribution.get(day, bin);

    const auto expected = [&]() {
        // Guard to make sure we don't divide by zero
        if (interval_admission_target == 0) {
            return 0U;
        }

        // The number of patients that were already admitted durning this
        // interval should be at least the time passed since the start of the
        // interval divided by the rate.
        // Note: this is equivalent to ceil(_interval_lenght / interval_admission_target).
        // If normal division (floor()) is used, it generates more patients than
        // it should due to rounding
        const auto rate = (_interval_length + interval_admission_target - 1) / interval_admission_target;
        return 1 + bin_offset / rate;
    }();

    const auto agents_waiting = expected - _generated_patients.at(day).at(bin);

    // Increase the number of agents created and return the value
    _generated_patients.at(day).at(bin) += agents_waiting;
    return agents_waiting;
}

////////////////////////////////////////////////////////////////////////////
// SAVE STATISTICS
////////////////////////////////////////////////////////////////////////////

/// @brief Save the stadistics/metrics to a file
/// @param filepath The path to the folder where
/// @param rank The rank of the process
void sti::hospital_entry::save(const std::string& folderpath, int rank) const
{
    auto os = std::ostringstream {};
    os << folderpath
       << "/entry.p"
       << rank
       << ".csv";

    auto file = std::ofstream { os.str() };

    file << "day,period,patients_generated\n";

    for (auto day = 0UL; day < _generated_patients.size(); ++day) {
        for (auto bin = 0UL; bin < _generated_patients.at(day).size(); ++bin) {
            file << day << ','
                 << bin << ','
                 << _generated_patients.at(day).at(bin) << '\n';
        }
    }
}

/// @brief Generate the pending patients
void sti::hospital_entry::generate_patients()
{
    // Create the needed patients
    const auto pending = patients_waiting();
    for (auto i = 0U; i < pending; i++) {

        using STAGES = human_infection_cycle::STAGE;

        const auto random = repast::Random::instance()->nextDouble();
        const auto stage  = _patient_distribution.get_infected_probability(_clock->now().human().days) > random ? STAGES::SICK : STAGES::HEALTHY;
        _agent_factory->insert_new_patient(_location.continuous(), stage);
    }
}

/// @brief Load the patient distribution curve from a file
/// @param json The json object containing the influx
/// @details File format is described in the documentation
sti::patient_distribution sti::load_patient_distribution(const boost::json::object& json)
{
    const auto& days = boost::json::value_to<boost::json::array>(json.at("parameters").at("patient").at("influx"));
    auto        data = std::vector<std::vector<std::uint32_t>> {};

    for (const auto& day : days) {
        const auto day_vec = boost::json::value_to<std::vector<std::uint32_t>>(day);
        data.push_back(day_vec);
    }

    const auto& infected_chance_data = json.at("parameters").at("patient").at("infected_probability").as_array();
    auto        infected_chance      = patient_distribution::infected_probability_type {};
    for (const auto& v : infected_chance_data) {
        validate_probability(v.as_double(), "Patient infected probabilty");
        infected_chance.push_back(v.as_double());
    }

    if (data.size() != infected_chance.size()) {
        throw influx_and_infected_probability_differ {};
    }

    return patient_distribution { std::move(data), std::move(infected_chance) };
}