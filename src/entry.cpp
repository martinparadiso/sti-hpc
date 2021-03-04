#include "entry.hpp"

#include <boost/lexical_cast.hpp>
#include <sstream>

#include "agent_factory.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "print.hpp"

/// @brief Create a hospital entry
/// @details The hospital entry is in charge of creating the patients
/// @param location The location of the entry
/// @param clock The simulation clock
/// @param patient_admissions The patient admission histogram
/// @param factory The agent factory, for patient creation
sti::hospital_entry::hospital_entry(plan::coordinates                     location,
                                    sti::clock*                           clock,
                                    std::unique_ptr<patient_distribution> patient_admissions,
                                    agent_factory*                        factory,
                                    repast::Properties&                   props)
    : _location { location }
    , _clock { clock }
    , _patient_distribution(std::move(patient_admissions))
    , _generated_patients { boost::histogram::make_histogram(
          patient_distribution::axis_t(0, _patient_distribution->days(), "day"),
          patient_distribution::axis_t(0, _patient_distribution->intervals(), "interval")) }
    // The length of the interval is the number of seconds in a day divided
    // by the number of intervals
    , _interval_length { (24 * 60 * 60) / _patient_distribution->intervals() }
    , _agent_factory { factory }
    , _infected_chance {
        boost::lexical_cast<double>(props.getProperty("patient.infected.chance"))
    }
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
    const auto interval_admission_target = _patient_distribution->get(day, bin);
    const auto rate                      = _interval_length / interval_admission_target;

    // The number of patients that were already admitted durning this
    // interval should be at least the time passed since the start of the
    // interval divided by the rate
    const auto expected = 1 + bin_offset / rate;

    const auto agents_waiting = expected - static_cast<std::uint64_t>(_generated_patients.at(day, bin));

    // Increase the number of agents created and return the value
    _generated_patients(day, bin, boost::histogram::weight(agents_waiting));
    return agents_waiting;
}

/// @brief Generate the pending patients
void sti::hospital_entry::generate_patients()
{
    // Create the needed patients
    const auto pending = patients_waiting();
    for (auto i = 0U; i < pending; i++) {

        using STAGES = human_infection_cycle::STAGE;

        const auto random = repast::Random::instance()->nextDouble();
        const auto stage  = _infected_chance > random ? STAGES::SICK : STAGES::HEALTHY;
        const auto * agent_ptr = _agent_factory->insert_new_patient(_location, stage);

        auto ss = std::stringstream{};
        ss << "Creating patient :: "
           << agent_ptr->getId() << " :: "
           << (stage == STAGES::SICK ? "SICK" : "HEALTHY");
        print(ss.str());
    }
}
