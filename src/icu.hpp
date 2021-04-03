/// @file icu.hpp
/// @brief ICU manager, keeps track of all the beds
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clock.hpp"

// Fw. declarations
namespace boost {
template <typename T>
class optional;
namespace json {
    class object;
} // namespace json
namespace mpi {
    class communicator;
} // namespace mpi
} // namespace boost

namespace repast {
class AgentId;
class Properties;
} // namespace repast

namespace sti {
class agent_factory;
class contagious_agent;
class object_agent;
class hospital_plan;
class clock;
class space_wrapper;
} // namespace sti

namespace sti {

/// @brief ICU manager
/// @details Is implemented in two parts, a 'real' icu an P-1 proxys that
/// synchronize with the real queue once per tick.
class icu {

public:
    using precission = double;
    using message    = std::pair<repast::AgentId, bool>;

    struct statistics {
        using counter_type = std::uint32_t;

        std::map<timedelta, counter_type> sleep_times;
        counter_type                      deaths;
    };

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct an ICU
    /// @param hospital_params The hospital parameters
    icu(const boost::json::object& hospital_params);

    icu(const icu&) = delete;
    icu& operator=(const icu&) = delete;

    icu(icu&&)   = default;
    icu& operator=(icu&&) = default;

    virtual ~icu();

    /// @brief Due to dependencies, beds cannot be created durning construction
    /// @param af Agent factory, to construct the beds
    virtual void create_beds(agent_factory& af) = 0;

    ////////////////////////////////////////////////////////////////////////////
    // ADMISSION MANAGEMENT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Request a bed in the ICU
    /// @param id The ID of the requesting agent
    virtual void request_bed(const repast::AgentId& id) = 0;

    /// @brief Check if the request has been processed
    /// @return If the request was processed by the manager, True if there is a bed, false otherwise
    virtual boost::optional<bool> get_response(const repast::AgentId& id) = 0;

    /// @brief Sync the requests and responses between the processes
    virtual void sync() = 0;

    /// @brief Absorb nearby agents into the ICU void
    /// @details The ICU is implemented as an spaceless entity, patients are
    /// absorbed into the ICU dimension and dissapear from space. They can
    /// contract the desease via environment, but not through other patients.
    virtual void tick() = 0;

    ////////////////////////////////////////////////////////////////////////////
    // ICU RANDOMNESS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the time the patient must spend in the ICU
    /// @return The time the patient has to sleep
    timedelta get_icu_time() const;

    /// @brief Return the outcome of the ICU, dead or alive
    /// @return True if the patient is saved, false if the patient dies
    bool survive() const;

    ////////////////////////////////////////////////////////////////////////////
    // STATISTICS COLLECTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get a pointer to the statistics
    /// @return A pointer to the stats
    statistics* stats() const;

    /// @brief Save the ICU stats into a file
    /// @param filepath The path to the folder where
    virtual void save(const std::string& folderpath) const = 0;

private:
    std::vector<std::pair<timedelta, precission>> _sleep_times;
    precission                                    _death_probability;

    std::unique_ptr<statistics> _stats;
}; // class icu

////////////////////////////////////////////////////////////////////////////////
// ICU CREATION
////////////////////////////////////////////////////////////////////////////////

/// @brief Construct a real ICU keeping track of beds assigned
/// @param communicator The MPI communicator
/// @param mpi_tag The MPI tag for the communication
/// @param hospital_props The hospital properties stored in a JSON object
/// @param hospital_plan The hospital plan.
/// @param space The space_wrapper
/// @param clock The simulation clock
icu* make_icu(boost::mpi::communicator*  communicator,
              const repast::Properties&  exec_props,
              const boost::json::object& hospital_props,
              const hospital_plan&       hospital_plan,
              space_wrapper*             space,
              clock*                     clock);

} // namespace sti