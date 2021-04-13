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
/// @param real_rank Real rank
sti::proxy_icu::proxy_icu(communicator_ptr           communicator,
                          int                        mpi_tag,
                          int                        real_rank)
    : _communicator { communicator }
    , _mpi_base_tag { mpi_tag }
    , _real_rank { real_rank }
{
}

sti::proxy_icu::~proxy_icu() = default;

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
boost::optional<bool> sti::proxy_icu::peek_response(const repast::AgentId& id) const
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
    return response.second;
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
