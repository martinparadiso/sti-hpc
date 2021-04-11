#include "../icu.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/json/object.hpp>
#include <fstream>
#include <repast_hpc/Properties.h>
#include <repast_hpc/Random.h>
#include <repast_hpc/RepastProcess.h>
#include <sstream>

#include "../space_wrapper.hpp"
#include "../hospital_plan.hpp"
#include "../json_serialization.hpp"
#include "../agent_factory.hpp"
#include "real_icu.hpp"
#include "proxy_icu.hpp"

////////////////////////////////////////////////////////////////////////////
// EXCEPTIONS
////////////////////////////////////////////////////////////////////////////

namespace {

/// @brief Error in the probabilities
struct wrong_icu_probabilities : public std::exception {
    const char* what() const noexcept override
    {
        return "ICU: Probabilities don't add up";
    }
};
} // namespace

////////////////////////////////////////////////////////////////////////////
// TYPE DEFINITIONS
////////////////////////////////////////////////////////////////////////////

struct sti::icu::statistics {
    using counter_type = std::uint32_t;

    std::map<timedelta, counter_type> sleep_times;
    counter_type                      deaths;
};

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct an ICU, let the construct figure out which one
/// @param communicator The MPI communicator
/// @param mpi_tag The MPI tag for the communication
/// @param hospital_props The hospital properties stored in a JSON object
/// @param hospital_plan The hospital plan.
/// @param space The space_wrapper
/// @param clock The simulation clock
sti::icu::icu(boost::mpi::communicator*  communicator,
              const repast::Properties&  execution_props,
              const boost::json::object& hospital_props,
              const hospital_plan&       hospital_plan,
              space_wrapper*             space,
              clock*                     clock)
    : _sleep_times { [&]() {
        auto vector = decltype(_sleep_times) {};

        const auto sleep_times = hospital_props.at("parameters").at("icu").at("sleep_time").as_array();

        for (const auto& entry : sleep_times) {
            const auto time        = boost::json::value_to<timedelta>(entry.at("time"));
            const auto probability = entry.at("probability").as_double();

            vector.push_back({ time, probability });
        }

        return vector;
    }() }
    , _death_probability { [&]() {
        return hospital_props.at("parameters").at("icu").at("death_probability").as_double();
    }() }
    , _real_icu { [&]() -> decltype(_real_icu) {
        // If the ICU is in this process create a real ICU. Ownership is set in
        // the next construct.
        const auto real_rank = boost::lexical_cast<int>(execution_props.getProperty("icu.manager.rank"));

        if (real_rank == communicator->rank()) {
            return new real_icu { communicator, mpi_tag, hospital_props, hospital_plan, space, clock };
        }

        return nullptr;
    }() }
    , _icu_admission { [&]() -> decltype(_icu_admission)::pointer {
        // If the real_icu is initialized, use the same pointer to initialize
        // the ownership. Otherwise this is a remote/proxy process and a
        // proxy_icu has to be created
        if (_real_icu != nullptr) {
            return _real_icu;
        }

        const auto real_rank = boost::lexical_cast<int>(execution_props.getProperty("icu.manager.rank"));
        return new proxy_icu { communicator, mpi_tag, real_rank };
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
// ADMISSION MANAGEMENT
////////////////////////////////////////////////////////////////////////////

/// @brief Get the admsission system, to add and remove agents
/// @return A reference to the admission
sti::icu_admission& sti::icu::admission()
{
    return *_icu_admission;
}

/// @brief Get the admsission system, to add and remove agents
/// @return A const reference to the admission
const sti::icu_admission& sti::icu::admission() const
{
    return *_icu_admission;
}

////////////////////////////////////////////////////////////////////////////
// REAL ICU ACCESS
////////////////////////////////////////////////////////////////////////////

/// @brief Get a reference to the real ICU, if it resides in this process
/// @return An optional containing a reference to the real ICU
std::optional<std::reference_wrapper<sti::real_icu>> sti::icu::get_real_icu()
{
    if (_real_icu != nullptr) return { *_real_icu };
    return {};
}

/// @brief Get a reference to the real ICU, if it resides in this process
/// @return An optional containing a const reference to the real ICU
std::optional<std::reference_wrapper<const sti::real_icu>> sti::icu::get_real_icu() const
{
    if (_real_icu != nullptr) return { *_real_icu };
    return {};
}

////////////////////////////////////////////////////////////////////////////
// ICU RANDOMNESS
////////////////////////////////////////////////////////////////////////////

/// @brief Get the time the patient must spend in the ICU
/// @return The time the patient has to sleep
sti::timedelta sti::icu::get_icu_time() const
{
    // The algorithm generates a random number, and checks in which bracket
    // it falls
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

/// @brief Save the ICU stats into a file
/// @param filepath The path to the folder where
void sti::icu::save(const std::string& folderpath) const
{
    // First save the ICU stats, then the real_icu
    auto os = std::ostringstream {};
    os << folderpath
       << "/icu_stats_in_process_"
       << repast::RepastProcess::instance()->rank()
       << ".csv";

    { // Open the file
        auto file = std::ofstream {};

        file << "parameter,value\n";

        for (const auto& [time, value] : _stats->sleep_times) {
            file << "sleep_" << time.str() << ","
                 << value
                 << "\n";
        }

        file << "deaths"
             << ","
             << _stats->deaths << "\n";
    } // Close the file

    // If the real_icu is here, save the stats
    if (_real_icu != nullptr) {
        _real_icu->save(folderpath);
    }
}
