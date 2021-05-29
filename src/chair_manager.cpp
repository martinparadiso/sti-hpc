#include <algorithm>
#include <boost/json.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/vector.hpp>
#include <cstdint>
#include <fstream>
#include <memory>
#include <repast_hpc/Random.h>
#include <sstream>
#include <utility>
#include <vector>

#include "chair_manager.hpp"
#include "clock.hpp"
#include "infection_logic/object_infection.hpp"
#include "infection_logic/infection_factory.hpp"
#include "contagious_agent.hpp"
#include "space_wrapper.hpp"

////////////////////////////////////////////////////////////////////////////
// CHAIR MANAGER
////////////////////////////////////////////////////////////////////////////

/// @brief Construct a chair manager
/// @param space A pointer to the space wrapper
sti::chair_manager::chair_manager(const space_wrapper* space)
    : _space { space }
{
}

/// @brief Create all the chairs located in this process
/// @param hospital_plan The hospital plan
/// @param inf_fac The infection factory
void sti::chair_manager::create_chairs(const hospital_plan& hospital_plan, infection_factory& inff)
{
    for (const auto& chair : hospital_plan.chairs()) {
        if (_space->local_dimensions().contains(chair.location)) {
            _chair_pool.push_back({ chair.location,
                                    inff.make_object_infection("chair", object_infection::STAGE::CLEAN) });
        }
    }
}

/// @brief Execute the periodic logic
void sti::chair_manager::tick()
{
    for (auto& [chair_location, chair_infection] : _chair_pool) {
        for (auto& agent : _space->agents_in_cell(chair_location)) {
            chair_infection.interact_with(*agent->get_infection_logic());
            agent->get_infection_logic()->interact_with(chair_infection);
        }
        chair_infection.tick();
    }
}

////////////////////////////////////////////////////////////////////////////
// STATISTICS
////////////////////////////////////////////////////////////////////////////

/// @brief Save stats
/// @param folderpath The folder to save the results to
/// @param rank The rank of the process
void sti::chair_manager::save(const std::string& folderpath, int rank) const
{
    auto output_array = boost::json::array {};

    for (const auto& [location, infection] : _chair_pool) {
        output_array.push_back(infection.stats());
    }

    auto os = std::ostringstream {};
    os << folderpath
       << "/chairs.p"
       << rank
       << ".json";
    auto chairs_file = std::ofstream { os.str() };
    chairs_file << output_array;
}

////////////////////////////////////////////////////////////////////////////////
// PROXY_CHAIR_MANAGER
////////////////////////////////////////////////////////////////////////////////

/// @brief Construct a proxy chair manager
/// @param comm The MPI communicator
/// @param real_manager The rank of the real chair manager
/// @param space A pointer to the space
sti::proxy_chair_manager::proxy_chair_manager(communicator*        comm,
                                              int                  real_manager,
                                              const space_wrapper* space)
    : chair_manager { space }
    , _world { comm }
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

/// @brief Check if there is a response without removing from the queue
/// @param id The id of the agent requesting the chair
/// @return An optional containing the response, if the manager already processed the request
boost::optional<sti::chair_response_msg> sti::proxy_chair_manager::peek_response(const repast::AgentId& id)
{
    // Search the vector of pending responses for the messages with that id
    const auto it = std::find_if(_pending_responses.begin(),
                                 _pending_responses.end(),
                                 [&](const auto& r) {
                                     return r.agent_id == id;
                                 });

    // The number of messages for that id that are pending
    if (it == _pending_responses.end())
        return {};

    return *it;
}

/// @brief Get the response of a chair request
/// @param id The id of the agent requesting the chair
/// @return An optional containing the response, if the manager already processed the request
boost::optional<sti::chair_response_msg> sti::proxy_chair_manager::get_response(const repast::AgentId& id)
{
    // Search the vector of pending responses for the messages with that id
    const auto it = std::find_if(_pending_responses.begin(),
                                 _pending_responses.end(),
                                 [&](const auto& r) {
                                     return r.agent_id == id;
                                 });

    if (it == _pending_responses.end()) {
        return boost::none;
    }

    const auto response = *it;
    _pending_responses.erase(it);
    return response;
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

/// @brief Save stats
/// @param folderpath The folder to save the results to
/// @param rank The rank of the process
void sti::proxy_chair_manager::save(const std::string& folderpath, int rank) const
{
    chair_manager::save(folderpath, rank);
}

////////////////////////////////////////////////////////////////////////////////
// UNNAMED NAMESPACE FOR AUX. FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

namespace {

/// @brief Release a chair
/// @param chair_pool The chairs
/// @param location The location of the chair to release
void release(std::vector<sti::real_chair_manager::chair>& chair_pool,
             sti::coordinates<double>                     location)
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
    // Chairs assigned need to be random otherwise the first chair will be
    // constantly in use, and infection rate goes to hell.
    // To pick a random chair, generate a random number in the range
    // [0, number_of_chairs), and start looking for an empty chair at that point
    // If after number_of_chairs iterations none is found, there are no chairs
    // available.
    // FIXME: it will be easier to keep a counter with the number of empty
    // chairs, to determine if there are chairs available in O(1) instead of 
    // this horrible O(N)

    auto c          = static_cast<std::uint32_t>(repast::Random::instance()->nextDouble() * static_cast<double>(chair_pool.size()));
    auto iterations = 0U;

    while (iterations < chair_pool.size()) {

        if (!chair_pool.at(c).in_use) {
            chair_pool.at(c).in_use = true;
            return sti::chair_response_msg { {}, chair_pool.at(c).location };
        }

        ++c;
        c = c >= chair_pool.size() ? 0 : c;
        ++iterations;
    }
    
    return sti::chair_response_msg { {}, boost::none };
}

} // namespace

////////////////////////////////////////////////////////////////////////////////
// REAL_CHAIR_MANAGER STATS
////////////////////////////////////////////////////////////////////////////////

/// @brief Collect chair stats
class sti::real_chair_manager::statistics {

public:
    using counter_type = decltype(sti::real_chair_manager::_chair_pool)::difference_type;

    /// @brief Add a new entry to the number of available chairs
    void push_free_chairs(counter_type c)
    {
        free_chairs.push_back(c);
    }

    /// @brief Save the stats to a given folder
    void save(const std::string& folderpath, int rank)
    {
        auto os = std::ostringstream {};
        os << folderpath
           << "/chair_availability.p"
           << rank
           << ".csv";
        auto avail_file = std::ofstream { os.str() };

        avail_file << "tick" << ','
                   << "free_chairs" << '\n';

        auto tick = 0;
        for (const auto& entry : free_chairs) {
            avail_file << tick++ << ','
                       << entry << '\n';
        }
    }

private:
    std::vector<counter_type> free_chairs; // TODO: Preallocate this vector
};

////////////////////////////////////////////////////////////////////////////////
// REAL_CHAIR_MANAGER
////////////////////////////////////////////////////////////////////////////////

/// @brief Construct a proxy chair manager
/// @param comm The MPI communicator
/// @param building The hospital plan
/// @param space A pointer to the space
sti::real_chair_manager::real_chair_manager(communicator*        comm,
                                            const hospital_plan& building,
                                            const space_wrapper* space)
    : chair_manager { space }
    , _world { comm }
    , _stats { /*std::make_unique<statistics>()*/ }
{
    for (const auto& chair : building.chairs()) {
        _chair_pool.push_back({ chair.location.continuous(), false });
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
void sti::real_chair_manager::release_chair(const sti::coordinates<double>& chair_loc)
{
    release(_chair_pool, chair_loc);
} // void release_chair(...)

/// @brief Check if there is a response without removing from the queue
/// @param id The id of the agent requesting the chair
/// @return An optional containing the response, if the manager already processed the request
boost::optional<sti::chair_response_msg> sti::real_chair_manager::peek_response(const repast::AgentId& id)
{
    // Search the vector of pending responses for the messages with that id
    const auto it = std::find_if(_pending_responses.begin(),
                                 _pending_responses.end(),
                                 [&](const auto& r) {
                                     return r.agent_id == id;
                                 });

    // The number of messages for that id that are pending
    if (it == _pending_responses.end()) {
        return boost::none;
    }

    return *it;
}

/// @brief Get the response of a chair request
/// @param id The id of the agent requesting the chair
/// @return An optional containing the response, if the manager already processed the request
boost::optional<sti::chair_response_msg> sti::real_chair_manager::get_response(const repast::AgentId& id)
{
    // Search the vector of pending responses for the messages with that id
    const auto it = std::find_if(_pending_responses.begin(),
                                 _pending_responses.end(),
                                 [&](const auto& r) {
                                     return r.agent_id == id;
                                 });

    if (it == _pending_responses.end()) {
        return boost::none;
    }

    const auto response = *it;
    _pending_responses.erase(it);
    return response;

} // boost::optional<sti::chair_response_msg> get_response()

/// @brief Receive all requests, process, and respond
void sti::real_chair_manager::sync()
{

    auto in_req       = std::vector<chair_request_msg> {};
    auto in_rel       = std::vector<chair_release_msg> {};
    auto out_response = std::map<int, std::vector<chair_response_msg>> {};

    // First receive the request and releases
    for (auto i = 0; i < _world->size() - 1; i++) {
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

    // Count the chairs
    if (_stats) {
        const auto free_chairs = std::count_if(_chair_pool.begin(), _chair_pool.end(),
                                               [](const auto& chair) {
                                                   return !chair.in_use;
                                               });
        _stats->push_free_chairs(free_chairs);
    }

    // Generate the missing output buffers,
    for (auto i = 0; i < _world->size(); i++) {
        if (_world->rank() != i) {
            out_response[i];
        }
    }

    // Send all the messages, the receiver it's in the agent id
    for (const auto& [from_rank, vector] : out_response) {
        _world->send(from_rank, mpi_base_tag + 2, vector);
    }
}

/// @brief Save stats
/// @param folderpath The folder to save the results to
/// @param rank The rank of the process
void sti::real_chair_manager::save(const std::string& folderpath, int rank) const
{
    chair_manager::save(folderpath, rank);
    if (_stats) {
        _stats->save(folderpath, _world->rank());
    }
}

/// @brief Construct a chair manager
/// @param comm The MPI communicator
/// @param building The hospital plan
/// @param space A pointer to the space
std::unique_ptr<sti::chair_manager> sti::make_chair_manager(repast::Properties&       execution_props,
                                                            boost::mpi::communicator* comm,
                                                            const hospital_plan&      building,
                                                            const space_wrapper*      space)
{
    const auto real_rank = boost::lexical_cast<int>(execution_props.getProperty("chair.manager.rank"));

    if (comm->rank() == real_rank) {
        return std::make_unique<real_chair_manager>(comm, building, space);
    }
    return std::make_unique<proxy_chair_manager>(comm, real_rank, space);
}