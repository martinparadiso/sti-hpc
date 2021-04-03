#include "proxy_icu.hpp"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/optional.hpp>
#include <fstream>
#include <repast_hpc/AgentId.h>
#include <sstream>

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct a real ICU keeping track of beds assigned
/// @param communicator The MPI communicator
/// @param mpi_tag The MPI tag for the communication
/// @param hospital_props The hospital properties stored in a JSON object
/// @param real_rank Real rank
sti::proxy_icu::proxy_icu(communicator_ptr           communicator,
                          int                        mpi_tag,
                          int                        real_rank,
                          const boost::json::object& hospital_props)
    : icu { hospital_props }
    , _communicator { communicator }
    , _mpi_base_tag { mpi_tag }
    , _real_rank { real_rank }
{
}

sti::proxy_icu::~proxy_icu() = default;

/// @brief Due to dependencies, beds cannot be created durning construction
/// @param af Agent factory, to construct the beds
void sti::proxy_icu::create_beds(agent_factory& /*unused*/)
{
    // Beds are created in the real ICU
}

////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Request a bed in the ICU
/// @param id The ID of the requesting agent
void sti::proxy_icu::request_bed(const repast::AgentId& id)
{
    _pending_requests.push_back(id);
}

/// @brief Check if the request has been processed
/// @return If the request was processed by the manager, True if there is a bed, false otherwise
boost::optional<bool> sti::proxy_icu::get_response(const repast::AgentId& id)
{
    // Search the vector of pending responses for the messages with that id
    const auto it = std::find_if(_pending_responses.begin(),
                                 _pending_responses.end(),
                                 [&](const auto& r) {
                                     return r.first == id;
                                 });

    if (it == _pending_responses.end()) {
        return boost::none;
    }

    const auto response = *it;
    _pending_responses.erase(it);
    return response.second;
}

/// @brief Sync the requests and responses between the processes
void sti::proxy_icu::sync()
{
    // Send pending requests
    auto mpi_tag = _mpi_base_tag;
    _communicator->send(_real_rank, mpi_tag++, _pending_requests);
    _pending_requests.clear();

    // The real ICU is processing the requests...

    // Receive the responses
    auto buff = decltype(_pending_responses) {};
    _communicator->recv(_real_rank, mpi_tag++, buff);
    _pending_responses.insert(_pending_responses.end(), buff.begin(), buff.end());
}

/// @brief Absorb nearby agents into the ICU void
/// @details The ICU is implemented as an spaceless entity, patients are
/// absorbed into the ICU dimension and dissapear from space. They can
/// contract the desease via environment, but not through other patients.
void sti::proxy_icu::tick()
{
    // Do nothing, the work is in the real_icu
}

/// @brief Save the ICU stats into a file
/// @param filepath The path to the folder where
void sti::proxy_icu::save(const std::string& folderpath) const
{
    // Write the ICU probabilities/randoms generated
    const auto* icu_stats  = icu::stats();
    auto        stats_path = std::ostringstream {};
    stats_path << folderpath
               << "icu_in_process_"
               << _communicator->rank()
               << ".csv";
    auto stats_file = std::ofstream { stats_path.str() };

    stats_file << "sleep_time,assigned\n";

    for (const auto& [sleep_time, assigned] : icu_stats->sleep_times) {
        stats_file << sleep_time.length() << ","
                   << assigned << "\n";
    }
}
