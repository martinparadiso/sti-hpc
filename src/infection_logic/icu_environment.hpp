/// @file infection_logic/icu_environment.hpp
/// @brief Implements the ICU infection environment
/// @details Implements the ICU infection environment, where the probability of
/// infecting an agent is based in the current number of persons in the ICU
#pragma once

#include "environment.hpp"
#include "infection_cycle.hpp"

#include <cstdint>
#include <string>

// Fw. declarations
namespace boost {
namespace json {
    class object;
} // namespace json
} // namespace boost

namespace sti {

/// @brief The infection env. of an ICU, the innate infection hazard of an ICU
/// @details Represents the infection environemnt of an ICU, the probability of
/// infecting an agent residing in this environment is linearly proportional to
/// the number of agents residing in the environment.
class icu_environment final : public infection_environment {

public:
    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct an ICU infection environment
    /// @param hospital_params JSON parameters of the hospital
    /// @param name The name of the environment, by default is 'icu'
    icu_environment(const boost::json::object& hospital_params, const std::string& name = "icu");

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get a reference to the current number of patients
    /// @return A reference to the current number of patients in the ICU
    std::uint32_t& patients();

    /// @brief Get the probability of infecting
    /// @return A value in the range [0, 1), depending of the number of patients
    infection_cycle::precission get_probability() const override;

    /// @brief Get the name of the environemnt, for statistic reasons
    /// @details Every environment must have an unique name, to be able to
    /// determine how an agent got infected
    std::string name() const override;

private:
    std::string                 _name;
    std::uint32_t               _current_patients;
    infection_cycle::precission _icu_infection_chance;

}; // class icu_environemnt

} // namespace sti
