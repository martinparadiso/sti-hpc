#include <algorithm>
#include <vector>

#include "chair_manager.hpp"
#include "plan/plan.hpp"

////////////////////////////////////////////////////////////////////////////////
// PROXY_CHAIR_MANAGER
////////////////////////////////////////////////////////////////////////////////

sti::proxy_chair_manager::proxy_chair_manager(communicator* comm, int real_manager)
    : _world { comm }
    , _real_rank { real_manager }
{
}

/// @brief Request an empty chair
/// @param id The id of the agent requesting a chair
void sti::proxy_chair_manager::request_chair(const repast::AgentId& id)
{
    const auto& msg = chair_request_msg { id };
    _request_buffer.push_back(msg);
}

/// @brief Release a chair
/// @param chair_loc The coordinates of the chair being released
void sti::proxy_chair_manager::release_chair(const coordinates& chair_loc)
{
    const auto& msg = chair_release_msg { chair_loc };
    _release_buffer.push_back(msg);
}

/// @brief Get the response of a chair request
/// @param id The id of the agent requesting the chair
/// @return An optional containing the response, if the manager already processed the request
boost::optional<sti::chair_response_msg> sti::proxy_chair_manager::get_response(const repast::AgentId& id)
{
    // Search the vector of pending responses for the messages with that id
    const auto it = std::remove_if(_pending_responses.begin(),
                                   _pending_responses.end(),
                                   [&](const auto& r) {
                                       return r.agent_id == id;
                                   });

    // The number of messages for that id that are pending
    const auto msg_count = _pending_responses.end() - it;

    if (msg_count == 0) return {};

    if (msg_count == 1) {
        const auto response = *it;
        _pending_responses.pop_back();
        return response;
    }

    // If we are here, the same id requested more than one chair
    throw std::exception {};
}

/// @brief Send the requests and retrieve the pending responses
void sti::proxy_chair_manager::sync()
{
    // Send requests and releases
    _world->send(_real_rank, mpi_base_tag + 0, _request_buffer);
    _world->send(_real_rank, mpi_base_tag + 1, _release_buffer);

    // Receive responses
    auto new_responses = std::vector<chair_response_msg> {};
    _world->recv(_real_rank, mpi_base_tag + 2, new_responses);
    _pending_responses.insert(_pending_responses.end(), new_responses.begin(), new_responses.end());

    // Clear the buffers
    _request_buffer.clear();
    _release_buffer.clear();
}

////////////////////////////////////////////////////////////////////////////////
// UNNAMED NAMESPACE FOR AUX. FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

namespace {

/// @brief Release a chair
/// @param chair_pool The chairs
/// @param location The location of the chair to release
void release(std::vector<sti::real_chair_manager::chair>& chair_pool,
             sti::plan::coordinates                       location)
{
    const auto it = std::find_if(chair_pool.begin(), chair_pool.end(), [&](const auto& chair) {
        return chair.location == location;
    });

    // If the chair is not in the vector, something went wrong
    if (it == chair_pool.end()) throw std::exception {};

    // Otherwise, mark it as unused
    it->in_use = false;
}

/// @brief Get an empty chair
/// @param chair_pool The pool of chairs
sti::chair_response_msg search_chair(std::vector<sti::real_chair_manager::chair>& chair_pool)
{

    auto cp_it = chair_pool.begin();

    while (cp_it != chair_pool.end() && cp_it->in_use) {
        ++cp_it;
    }

    if (cp_it == chair_pool.end()) {
        return sti::chair_response_msg { {}, boost::none };
    }

    // Mark the chair as used and return
    cp_it->in_use = true;
    return sti::chair_response_msg { {}, cp_it->location };
}

} // namespace

////////////////////////////////////////////////////////////////////////////////
// REAL_CHAIR_MANAGER
////////////////////////////////////////////////////////////////////////////////

sti::real_chair_manager::real_chair_manager(communicator* comm, plan& building)
    : _world { comm }
{

    for (const auto& [x, y] : building.get(plan_tile::TILE_ENUM::CHAIR)) {
        _chair_pool.push_back({ { x, y }, false });
    }
}

/// @brief Request an empty chair
/// @param id The id of the agent requesting a chair
void sti::real_chair_manager::request_chair(const repast::AgentId& id)
{
    auto response     = search_chair(_chair_pool);
    response.agent_id = id;
    _pending_responses.push_back(response);
} // void request_chair(...)

/// @brief Release a chair
/// @param chair_loc The coordinates of the chair being released
void sti::real_chair_manager::release_chair(const sti::plan::coordinates& chair_loc)
{
    release(_chair_pool, chair_loc);
} // void release_chair(...)

/// @brief Get the response of a chair request
/// @param id The id of the agent requesting the chair
/// @return An optional containing the response, if the manager already processed the request
boost::optional<sti::chair_response_msg> sti::real_chair_manager::get_response(const repast::AgentId& id)
{
    // Search the vector of pending responses for the messages with that id
    const auto it = std::remove_if(_pending_responses.begin(),
                                   _pending_responses.end(),
                                   [&](const auto& r) {
                                       return r.agent_id == id;
                                   });

    // The number of messages for that id that are pending
    const auto msg_count = _pending_responses.end() - it;

    if (msg_count == 0) return {};

    if (msg_count == 1) {
        const auto response = *it;
        _pending_responses.pop_back();
        return response;
    }

    // If we are here, the same id requested more than one chair
    throw std::exception {};
} // boost::optional<sti::chair_response_msg> get_response()

/// @brief Receive all requests, process, and respond
void sti::real_chair_manager::sync()
{

    auto in_req       = std::vector<chair_request_msg> {};
    auto in_rel       = std::vector<chair_release_msg> {};
    auto out_response = std::map<int, std::vector<chair_response_msg>> {};

    // First receive the request and releases
    for (auto i = 0; i < _world->size(); i++) {
        auto tmp_requests = std::vector<chair_request_msg> {};
        auto tmp_releases = std::vector<chair_release_msg> {};

        _world->recv(boost::mpi::any_source, mpi_base_tag + 0, tmp_requests);
        _world->recv(boost::mpi::any_source, mpi_base_tag + 1, tmp_releases);

        in_req.insert(in_req.end(), tmp_requests.begin(), tmp_requests.end());
        in_rel.insert(in_rel.end(), tmp_releases.begin(), tmp_releases.end());
    }

    // Process releases first
    for (const auto& r : in_rel) {
        release(_chair_pool, r.chair_location);
    }

    // Now process the requests
    for (const auto& req : in_req) {
        const auto from_rank = req.agent_id.currentRank();
        auto       response  = search_chair(_chair_pool);
        response.agent_id    = req.agent_id;
        out_response[from_rank].push_back(response);
    }

    // Send all the messages, the receiver it's in the agent id
    for (const auto& [from_rank, vector] : out_response) {
        _world->send(from_rank, mpi_base_tag + 2, vector);
    }
}
