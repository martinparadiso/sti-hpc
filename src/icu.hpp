/// @file icu.hpp
/// @brief ICU manager, keeps track of all the beds
#pragma once

#include <repast_hpc/SharedContext.h>
#include <cstdint>
#include <functional>
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
class icu_admission;
class infection_factory;
class contagious_agent;
class object_agent;
class hospital_plan;
class clock;
class space_wrapper;
class real_icu;
} // namespace sti

namespace sti {

/// @brief ICU manager, wraps the proxy ICU and the real ICU.
/// @details Is implemented in two parts, a 'real' icu an P-1 proxys that
/// synchronize with the real queue once per tick.
class icu final {

public:
    constexpr static auto mpi_tag = 7835; //  MPI tag for synchronization

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct an ICU, let the construct figure out which one
    /// @param context A pointer to the repast context
    /// @param communicator The MPI communicator
    /// @param mpi_tag The MPI tag for the communication
    /// @param hospital_props The hospital properties stored in a JSON object
    /// @param hospital_plan The hospital plan.
    /// @param space The space_wrapper
    /// @param clock The simulation clock
    icu(repast::SharedContext<contagious_agent>* context,
        boost::mpi::communicator*                communicator,
        const boost::json::object&               hospital_props,
        const hospital_plan&                     hospital_plan,
        space_wrapper*                           space,
        clock*                                   clock);

    icu(const icu&) = delete;
    icu& operator=(const icu&) = delete;

    icu(icu&&)   = default;
    icu& operator=(icu&&) = default;

    virtual ~icu();

    ////////////////////////////////////////////////////////////////////////////
    // ADMISSION MANAGEMENT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the admsission system, to add and remove agents
    /// @return A reference to the admission
    icu_admission& admission();

    /// @brief Get the admsission system, to add and remove agents
    /// @return A const reference to the admission
    const icu_admission& admission() const;

    ////////////////////////////////////////////////////////////////////////////
    // REAL ICU ACCESS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get a reference to the real ICU, if it resides in this process
    /// @return An optional containing a reference to the real ICU
    std::optional<std::reference_wrapper<real_icu>> get_real_icu();

    /// @brief Get a reference to the real ICU, if it resides in this process
    /// @return An optional containing a const reference to the real ICU
    std::optional<std::reference_wrapper<const real_icu>> get_real_icu() const;

    ////////////////////////////////////////////////////////////////////////////
    // STATISTICS COLLECTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Save the ICU stats into a file
    /// @param filepath The path to the folder where
    void save(const std::string& folderpath) const;

private:
    // The wrapper stores an owning pointer to an abstract icu_admission and a
    // raw pointer to the real icu, in case the real_icu is in this process
    real_icu*                      _real_icu;
    std::unique_ptr<icu_admission> _icu_admission;

}; // class icu

////////////////////////////////////////////////////////////////////////////////
// ICU CREATION
////////////////////////////////////////////////////////////////////////////////

} // namespace sti