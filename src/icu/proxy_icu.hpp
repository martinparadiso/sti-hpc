/// @file icu/proxy_icu.hpp
/// @brief 'Real' part of the proxy pattern applied to the icu
#pragma once

#include "../icu.hpp"

#include <memory>
#include <cstdint>
#include <vector>

#include "../coordinates.hpp"

// Fw. declarations
namespace boost {
namespace mpi {
    class communicator;
} // namespace mpi
} // namespace boost

namespace boost {
namespace json {
    class object;
} // class json
} // class boost

namespace sti {
class agent_factory;
class contagious_agent;
class object_agent;
class hospital_plan;
class clock;
class space_wrapper;
} // namespace sti

namespace sti {

/// @brief Real ICU, in charge of the responses
class proxy_icu final : public icu {

public:
    using communicator_ptr = boost::mpi::communicator*;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a real ICU keeping track of beds assigned
    /// @param communicator The MPI communicator
    /// @param mpi_tag The MPI tag for the communication
    /// @param hospital_props The hospital properties stored in a JSON object
    /// @param real_rank Real rank
    proxy_icu(communicator_ptr           communicator,
              int                        mpi_tag,
              int                        real_rank,
              const boost::json::object& hospital_props);

    proxy_icu(const proxy_icu&) = delete;
    proxy_icu& operator=(const proxy_icu&) = delete;

    proxy_icu(proxy_icu&&) = default;
    proxy_icu& operator=(proxy_icu&&) = default;

    ~proxy_icu();

    /// @brief Due to dependencies, beds cannot be created durning construction
    /// @param infection_factory The infection factory, to create the beds
    void create_beds(infection_factory &infection_factory) override;

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Request a bed in the ICU
    /// @param id The ID of the requesting agent
    void request_bed(const repast::AgentId& id) override;

    /// @brief Check if the request has been processed
    /// @return If the request was processed by the manager, True if there is a bed, false otherwise
    boost::optional<bool> get_response(const repast::AgentId& id) override;

    /// @brief Sync the requests and responses between the processes
    void sync() override;

    /// @brief Absorb nearby agents into the ICU void
    /// @details The ICU is implemented as an spaceless entity, patients are
    /// absorbed into the ICU dimension and dissapear from space. They can
    /// contract the desease via environment, but not through other patients.
    void tick() override;

    /// @brief Save the ICU stats into a file
    /// @param filepath The path to the folder where
    void save(const std::string& folderpath) const override;

private:
    communicator_ptr _communicator;
    int              _mpi_base_tag;
    int              _real_rank;

    std::vector<message>         _pending_responses;
    std::vector<repast::AgentId> _pending_requests;
};

} // namespace sti