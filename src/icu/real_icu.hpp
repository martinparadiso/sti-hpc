/// @file icu/real_icu.hpp
/// @brief 'Real' part of the proxy pattern applied to the icu
#pragma once

#include "icu_admission.hpp"

#include <memory>
#include <cstdint>
#include <vector>

#include "../coordinates.hpp"
#include "../infection_logic/icu_environment.hpp"

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
class patient_agent;
class hospital_plan;
class clock;
class space_wrapper;
class object_infection;
} // namespace sti

namespace sti {

/// @brief Real ICU, in charge of the responses
class real_icu final : public icu_admission {

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
             clock*                     clock);

    real_icu(const real_icu&) = delete;
    real_icu& operator=(const real_icu&) = delete;

    real_icu(real_icu&&) = default;
    real_icu& operator=(real_icu&&) = default;

    ~real_icu();

    /// @brief Due to dependencies, beds cannot be created durning construction
    /// @param infection_factory Infection factory, to construct the beds
    void create_beds(infection_factory& infection_factory);

    ////////////////////////////////////////////////////////////////////////////
    // ADMISSION MANAGEMENT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Request a bed in the ICU
    /// @param id The ID of the requesting agent
    void request_bed(const repast::AgentId& id) override;

    /// @brief Check if the request has been processed, if so, return the answer
    /// @return An optional, containing True if there is a bed, false otherwise
    boost::optional<bool> peek_response(const repast::AgentId& id) const override;

    /// @brief Check if the request has been processed, if so, return the answer
    /// @return An optional, containing True if there is a bed, false otherwise
    boost::optional<bool> get_response(const repast::AgentId& id) override;

    /// @brief Sync the requests and responses between the processes
    void sync() override;

    ////////////////////////////////////////////////////////////////////////////
    // PATIENT INSERTION AND REMOVAL
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Insert a patient into the ICU
    /// @throws no_more_beds If the bed_pool is full
    /// @param patient_ptr A pointer to the patient to remove
    void insert(patient_agent* patient_ptr);

    /// @brief Remove a patient from the ICU
    /// @throws no_patient If the agent to remove is not in the bed pool
    /// @param patient_ptr A pointer to the patient to remove
    void remove(sti::patient_agent* patient_ptr);

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Absorb nearby agents into the ICU void
    /// @details The ICU is implemented as an spaceless entity, patients are
    /// absorbed into the ICU dimension and dissapear from space. They can
    /// contract the desease via environment, but not through other patients.
    void tick();

    ////////////////////////////////////////////////////////////////////////////
    // STATS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Save the ICU stats into a file
    /// @param filepath The path to the folder where
    void save(const std::string& folderpath) const;

private:
    communicator_ptr _communicator;
    int              _mpi_base_tag;

    clock*         _clock;

    coordinates<int> _icu_location;

    bed_counter_type                                         _reserved_beds;
    bed_counter_type                                         _capacity;
    std::vector<std::pair<object_infection, patient_agent*>> _bed_pool;
    icu_environment                                          _environment;

    std::vector<response_message> _pending_responses;

    std::unique_ptr<statistics> _stats;
};

} // namespace sti