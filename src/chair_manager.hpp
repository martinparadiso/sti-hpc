/// @file chair_manager.hpp
/// @brief Class in charge of managing chairs
#pragma once

#include <boost/mpi/communicator.hpp>
#include <boost/optional/optional.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/vector.hpp>
#include <map>
#include <repast_hpc/AgentId.h>
#include <utility>
#include <vector>

// #include "plan/plan.hpp"
#include "hospital_plan.hpp"

namespace sti {

/// @brief A chair request, a petition for an empty chair
struct chair_request_msg {
    repast::AgentId agent_id;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& agent_id;
    }
};

/// @brief A indication that a chair has been released
struct chair_release_msg {
    sti::coordinates<int> chair_location;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& chair_location;
    }
};

/// @brief A request response, only for empty chair requests
struct chair_response_msg {
    // If a chair was available, the response has the coordinates, if there was no
    // chair available, no coordinates are sent
    repast::AgentId                        agent_id;
    boost::optional<sti::coordinates<int>> chair_location;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& agent_id;
        ar& chair_location;
    }
};

/// @brief An abstract chair manager
class chair_manager {

public:
    using coordinates  = sti::coordinates<int>;
    using communicator = boost::mpi::communicator;
    template <typename T>
    using optional = boost::optional<T>;

    static constexpr auto mpi_base_tag = 716823;

    chair_manager() = default;

    chair_manager(const chair_manager&) = default;
    chair_manager& operator=(const chair_manager&) = default;

    chair_manager(chair_manager&&) = default;
    chair_manager& operator=(chair_manager&&) = default;

    virtual ~chair_manager() = default;

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Request an empty chair
    /// @param id The id of the agent requesting a chair
    virtual void request_chair(const repast::AgentId& id) = 0;

    /// @brief Release a chair
    /// @param chair_loc The coordinates of the chair being released
    virtual void release_chair(const coordinates& chair_loc) = 0;

    /// @brief Get the response of a chair request
    /// @param id The id of the agent requesting the chair
    /// @return An optional containing the response, if the manager already processed the request
    virtual optional<chair_response_msg> get_response(const repast::AgentId& id) = 0;

    /// @brief Synchronize with the other processes
    virtual void sync() = 0;
};

/// @brief A proxy chair manager, that comunicates with the real one through MPI
class proxy_chair_manager final : public chair_manager {

public:
    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    proxy_chair_manager(communicator* comm, int real_manager);

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Request an empty chair
    /// @param id The id of the agent requesting a chair
    void request_chair(const repast::AgentId& id) final;

    /// @brief Release a chair
    /// @param chair_loc The coordinates of the chair being released
    void release_chair(const coordinates& chair_loc) final;

    /// @brief Get the response of a chair request
    /// @param id The id of the agent requesting the chair
    /// @return An optional containing the response, if the manager already processed the request
    optional<chair_response_msg> get_response(const repast::AgentId& id) final;

    /// @brief Send the requests and retrieve the pending responses
    void sync() final;

private:
    communicator* _world;
    int           _real_rank;

    std::vector<chair_request_msg>  _request_buffer;
    std::vector<chair_release_msg>  _release_buffer;
    std::vector<chair_response_msg> _pending_responses;
};

/// @brief The real chair manager containing the chair pool
class real_chair_manager final : public chair_manager {

public:
    struct chair {
        sti::coordinates<int> location;
        bool                  in_use;
    };

    /// @brief Construct a chair manager
    /// @param comm Boost.MPI communicator
    /// @param building Building plan
    real_chair_manager(communicator* comm, const hospital_plan& building);

    /// @brief Request an empty chair
    /// @param id The id of the agent requesting a chair
    void request_chair(const repast::AgentId& id) final;

    /// @brief Release a chair
    /// @param chair_loc The coordinates of the chair being released
    void release_chair(const coordinates& chair_loc) final;

    /// @brief Get the response of a chair request
    /// @param id The id of the agent requesting the chair
    /// @return An optional containing the response, if the manager already processed the request
    optional<chair_response_msg> get_response(const repast::AgentId& id) final;

    /// @brief Receive all requests, process, and respond
    void sync() final;

private:
    communicator* _world;

    template <typename T>
    using pool_t = std::vector<T>;

    pool_t<chair> _chair_pool;

    std::vector<chair_response_msg> _pending_responses;
};

} // namespace sti