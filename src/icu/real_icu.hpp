/// @file icu/real_icu.hpp
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
class real_icu final : public icu {

public:
    using communicator_ptr = boost::mpi::communicator*;
    using bed_counter_type = std::uint32_t;

    /// @brief Collect different statistics
    struct statistics;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a real ICU keeping track of beds assigned
    /// @param communicator The MPI communicator
    /// @param mpi_tag The MPI tag for the communication
    /// @param hospital_props The hospital properties stored in a JSON object
    /// @param hospital_plan The hospital plan.
    /// @param space The space_wrapper
    /// @param clock The simulation clock
    real_icu(communicator_ptr           communicator,
             int                        mpi_tag,
             const boost::json::object& hospital_props,
             const hospital_plan&       hospital_plan,
             space_wrapper*             space,
             clock*                     clock);

    real_icu(const real_icu&) = delete;
    real_icu& operator=(const real_icu&) = delete;

    real_icu(real_icu&&) = default;
    real_icu& operator=(real_icu&&) = default;

    ~real_icu();

    /// @brief Due to dependencies, beds cannot be created durning construction
    /// @param af Agent factory, to construct the beds
    void create_beds(agent_factory& af) override;

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

    space_wrapper* _space;
    clock*         _clock;

    coordinates<int> _icu_entry;
    coordinates<int> _icu_exit;

    std::unique_ptr<statistics> _stats;

    bed_counter_type                                         _reserved_beds;
    std::vector<std::pair<object_agent*, contagious_agent*>> _bed_pool;

    std::vector<message> _pending_responses;

    ////////////////////////////////////////////////////////////////////////////////
    // HELPER FUNCTIONS
    ///////////////////////////////////////////////////////////////////////////////

    /// @brief Insert a patient into the ICU
    /// @throws no_more_beds If the bed_pool is full
    /// @param patient_ptr A pointer to the patient to remove
    void insert(contagious_agent* patient_ptr);

    /// @brief Remove a patient from the ICU
    /// @throws no_patient If the agent to remove is not in the bed pool
    /// @param patient_ptr A pointer to the patient to remove
    void remove(const sti::contagious_agent* patient_ptr);
};

} // namespace sti