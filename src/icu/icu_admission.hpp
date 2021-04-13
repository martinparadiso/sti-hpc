/// @file icu/icu_admission.hpp
/// @brief Abstract class representing the interface for requesting and releasing an ICU bed
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../clock.hpp"

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
class infection_factory;
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
class icu_admission {

public:
    using precission = double;

    using request_message  = repast::AgentId;
    using response_message = std::pair<repast::AgentId, bool>;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    icu_admission() = default;

    icu_admission(const icu_admission&) = default;
    icu_admission& operator=(const icu_admission&) = default;

    icu_admission(icu_admission&&) = default;
    icu_admission& operator=(icu_admission&&) = default;

    virtual ~icu_admission() = default;

    ////////////////////////////////////////////////////////////////////////////
    // ADMISSION MANAGEMENT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Request a bed in the ICU
    /// @param id The ID of the requesting agent
    virtual void request_bed(const repast::AgentId& id) = 0;

    /// @brief Check if the request has been processed, if so, return the answer
    /// @return An optional, containing True if there is a bed, false otherwise
    virtual boost::optional<bool> peek_response(const repast::AgentId& id) const = 0;

    /// @brief Check if the request has been processed, if so, return the answer
    /// @return An optional, containing True if there is a bed, false otherwise
    virtual boost::optional<bool> get_response(const repast::AgentId& id) = 0;

    /// @brief Sync the requests and responses between the processes
    virtual void sync() = 0;

}; // class icu

} // namespace sti