/// @file chair_manager.hpp
/// @brief Class in charge of managing chairs
#pragma once

#include <boost/mpi/communicator.hpp>
#include <map>
#include <memory>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/Properties.h>
#include <utility>
#include <vector>

#include "coordinates.hpp"
#include "hospital_plan.hpp"
#include "infection_logic/object_infection.hpp"

// Fw. declarations
namespace repast {
class Properties;
}

namespace boost {
template <typename T>
class optional;
}
namespace sti {
class infection_factory;
class space_wrapper;
}

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
    sti::coordinates<double> chair_location;

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
    repast::AgentId                           agent_id;
    boost::optional<sti::coordinates<double>> chair_location;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& agent_id;
        ar& chair_location;
    }
};

////////////////////////////////////////////////////////////////////////////
// CHAIR MANAGER
////////////////////////////////////////////////////////////////////////////

/// @brief Contains an interface for managing chairs, and the infection logic
class chair_manager {

public:
    using coordinates  = sti::coordinates<double>;
    using communicator = boost::mpi::communicator;
    template <typename T>
    using optional = boost::optional<T>;

    static constexpr auto mpi_base_tag = 716823;

    /// @brief Construct a chair manager
    /// @param space A pointer to the space wrapper
    chair_manager(const space_wrapper* space);

    chair_manager(const chair_manager&) = default;
    chair_manager& operator=(const chair_manager&) = default;

    chair_manager(chair_manager&&) = default;
    chair_manager& operator=(chair_manager&&) = default;

    virtual ~chair_manager() = default;

    ////////////////////////////////////////////////////////////////////////////
    // ASSIGNMENT BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Request an empty chair
    /// @param id The id of the agent requesting a chair
    virtual void request_chair(const repast::AgentId& id) = 0;

    /// @brief Release a chair
    /// @param chair_loc The coordinates of the chair being released
    virtual void release_chair(const coordinates& chair_loc) = 0;

    /// @brief Check if there is a response without removing from the queue
    /// @param id The id of the agent requesting the chair
    /// @return An optional containing the response, if the manager already processed the request
    virtual optional<chair_response_msg> peek_response(const repast::AgentId& id) = 0;

    /// @brief Get the response of a chair request
    /// @param id The id of the agent requesting the chair
    /// @return An optional containing the response, if the manager already processed the request
    virtual optional<chair_response_msg> get_response(const repast::AgentId& id) = 0;

    /// @brief Synchronize with the other processes
    virtual void sync() = 0;

    ////////////////////////////////////////////////////////////////////////////
    // CHAIR INFECTIONS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create all the chairs located in this process
    /// @param hospital_plan The hospital plan
    /// @param inf_fac The infection factory
    void create_chairs(const hospital_plan& hospital_plan, infection_factory& inff);

    /// @brief Execute the periodic logic
    void tick();

    ////////////////////////////////////////////////////////////////////////////
    // STATISTICS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Save stats
    /// @param folderpath The folder to save the results to
    /// @param rank The rank of the process
    virtual void save(const std::string& folderpath, int rank) const;

private:
    const space_wrapper*                                            _space;
    std::vector<std::pair<sti::coordinates<int>, object_infection>> _chair_pool;
};

/// @brief A proxy chair manager, that comunicates with the real one through MPI
class proxy_chair_manager final : public chair_manager {

public:
    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a proxy chair manager
    /// @param comm The MPI communicator
    /// @param real_manager The rank of the real chair manager
    /// @param space A pointer to the space
    proxy_chair_manager(communicator*        comm,
                        int                  real_manager,
                        const space_wrapper* space);

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Request an empty chair
    /// @param id The id of the agent requesting a chair
    void request_chair(const repast::AgentId& id) override;

    /// @brief Release a chair
    /// @param chair_loc The coordinates of the chair being released
    void release_chair(const coordinates& chair_loc) override;

    /// @brief Check if there is a response without removing from the queue
    /// @param id The id of the agent requesting the chair
    /// @return An optional containing the response, if the manager already processed the request
    optional<chair_response_msg> peek_response(const repast::AgentId& id) override;

    /// @brief Get the response of a chair request
    /// @param id The id of the agent requesting the chair
    /// @return An optional containing the response, if the manager already processed the request
    optional<chair_response_msg> get_response(const repast::AgentId& id) override;

    /// @brief Send the requests and retrieve the pending responses
    void sync() override;

    /// @brief Save stats

    /// @param rank The rank of the process
    void save(const std::string& folderpath, int rank) const override;

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
        sti::coordinates<double> location;
        bool                     in_use;
    };
    template <typename T>
    using pool_t = std::vector<T>;

    /// @brief Collect chair pool stats
    class statistics;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a proxy chair manager
    /// @param comm The MPI communicator
    /// @param building The hospital plan
    /// @param space A pointer to the space
    real_chair_manager(communicator*        comm,
                       const hospital_plan& building,
                       const space_wrapper* space);

    /// @brief Request an empty chair
    /// @param id The id of the agent requesting a chair
    void request_chair(const repast::AgentId& id) override;

    /// @brief Release a chair
    /// @param chair_loc The coordinates of the chair being released
    void release_chair(const coordinates& chair_loc) override;

    /// @brief Check if there is a response without removing from the queue
    /// @param id The id of the agent requesting the chair
    /// @return An optional containing the response, if the manager already processed the request
    optional<chair_response_msg> peek_response(const repast::AgentId& id) override;

    /// @brief Get the response of a chair request
    /// @param id The id of the agent requesting the chair
    /// @return An optional containing the response, if the manager already processed the request
    optional<chair_response_msg> get_response(const repast::AgentId& id) override;

    /// @brief Receive all requests, process, and respond
    void sync() override;

    /// @brief Save stats
    /// @param folderpath The folder to save the results to
    /// @param rank The rank of the process
    void save(const std::string& folderpath, int rank) const override;

private:
    communicator*                   _world;
    pool_t<chair>                   _chair_pool;
    std::vector<chair_response_msg> _pending_responses;
    std::unique_ptr<statistics>     _stats;
};

/// @brief Construct a chair manager
/// @param comm The MPI communicator
/// @param building The hospital plan
/// @param space A pointer to the space
std::unique_ptr<chair_manager> make_chair_manager(repast::Properties&       execution_props,
                                                  boost::mpi::communicator* comm,
                                                  const hospital_plan&      building,
                                                  const space_wrapper*      space);

} // namespace sti

BOOST_CLASS_IMPLEMENTATION(sti::chair_request_msg, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(sti::chair_request_msg, boost::serialization::track_never)

BOOST_CLASS_IMPLEMENTATION(sti::chair_release_msg, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(sti::chair_release_msg, boost::serialization::track_never)

BOOST_CLASS_IMPLEMENTATION(sti::chair_response_msg, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(sti::chair_response_msg, boost::serialization::track_never)
