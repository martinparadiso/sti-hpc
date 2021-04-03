#include "../icu.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/json/object.hpp>
#include <repast_hpc/Properties.h>
#include <repast_hpc/Random.h>

#include "../space_wrapper.hpp"
#include "../hospital_plan.hpp"
#include "../json_serialization.hpp"
#include "../agent_factory.hpp"
#include "real_icu.hpp"
#include "proxy_icu.hpp"

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

namespace {

/// @brief Error in the probabilities
struct wrong_icu_probabilities : public std::exception {

    const char* what() const noexcept override
    {
        return "ICU: Probabilities don't add app";
    }
};
}

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct an ICU
/// @param hospital_params The hospital parameters
sti::icu::icu(const boost::json::object& hospital_params)
    : _sleep_times { [&]() {
        auto vector = decltype(_sleep_times) {};

        const auto sleep_times = hospital_params.at("parameters").at("icu").at("sleep_time").as_array();

        for (const auto& entry : sleep_times) {
            const auto time        = boost::json::value_to<timedelta>(entry.at("time"));
            const auto probability = entry.at("probability").as_double();

            vector.push_back({ time, probability });
        }

        return vector;
    }() }
    , _death_probability { [&]() {
        return hospital_params.at("parameters").at("icu").at("death_probability").as_double();
    }() }
    , _stats { std::make_unique<decltype(_stats)::element_type>() }
{

    // Verify the probabilities
    auto acc = 0.0;
    for (const auto& [key, value] : _sleep_times) acc += value;

    if (acc != 1.0) throw wrong_icu_probabilities {};

    if (_death_probability > 1.0) throw wrong_icu_probabilities {};
}

sti::icu::~icu() = default;

////////////////////////////////////////////////////////////////////////////
// ICU RANDOMNESS
////////////////////////////////////////////////////////////////////////////

/// @brief Get the time the patient must spend in the ICU
/// @return The time the patient has to sleep
sti::timedelta sti::icu::get_icu_time() const
{
    // The algorithm generates a random number, and checks in which bracket
    // falls.
    const auto sleep_assigned = [&]() {
        const auto random_sleep = repast::Random::instance()->nextDouble();
        auto       base         = 0.0;
        for (const auto& [sleep_time, probability] : _sleep_times) {

            if (base <= random_sleep && random_sleep <= base + probability) {
                return sleep_time;
            }

            base += probability;
        }

        // If for some reason the program is still here, return the last
        // sleep_time
        const auto last = --_sleep_times.end();
        return last->first;
    }();

    // Update the statistics
    _stats->sleep_times[sleep_assigned]++;

    return sleep_assigned;
}

/// @brief Return the outcome of the ICU, dead or alive
/// @return True if the patient is saved, false if the patient dies
bool sti::icu::survive() const
{

    const auto survive = [&]() {
        const auto random = repast::Random::instance()->nextDouble();
        return random >= _death_probability;
    }();

    if (!survive) {
        _stats->deaths++;
    }

    return survive;
}

////////////////////////////////////////////////////////////////////////////
// STATISTICS COLLECTION
////////////////////////////////////////////////////////////////////////////

/// @brief Get a pointer to the statistics
/// @return A pointer to the stats
sti::icu::statistics* sti::icu::stats() const
{
    return _stats.get();
}

////////////////////////////////////////////////////////////////////////////////
// ICU CREATION
////////////////////////////////////////////////////////////////////////////////

/// @brief Construct a real ICU keeping track of beds assigned
/// @param communicator The MPI communicator
/// @param mpi_tag The MPI tag for the communication
/// @param hospital_props The hospital properties stored in a JSON object
/// @param hospital_plan The hospital plan.
/// @param space The space_wrapper
/// @param clock The simulation clock
sti::icu* sti::make_icu(boost::mpi::communicator*  communicator,
                        const repast::Properties&  exec_props,
                        const boost::json::object& hospital_props,
                        const hospital_plan&       hospital_plan,
                        space_wrapper*             space,
                        clock*                     clock)
{
    const auto this_rank = communicator->rank();
    const auto real_rank = boost::lexical_cast<int>(exec_props.getProperty("icu.manager.rank"));
    const auto mpi_tag = 7835;
    
    if (real_rank == this_rank) {
        return new real_icu{communicator, mpi_tag, hospital_props, hospital_plan, space, clock};
    }

    return new proxy_icu{communicator, mpi_tag, real_rank, hospital_props};
}