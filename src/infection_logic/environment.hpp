/// @file infection_logic/environment.hpp
/// @brief Represents the "air" in the infection cycle
#pragma once

#include "infection_cycle.hpp"
#include <string>

namespace sti {

/// @brief Represents the 'air' in the infection cycle
/// @details Different infection models use different infection sources. This
/// class represents the most basic source, where the environment/universe the
/// agent resides has an innate probability of infecting him with the disease.
/// The class is pure-virtual to support different models/equations/algorithms
/// to determine the infection probability.
class infection_environment {

public:
    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    infection_environment() = default;

    infection_environment(const infection_environment&) = default;
    infection_environment& operator=(const infection_environment&) = default;

    infection_environment(infection_environment&&) = default;
    infection_environment& operator=(infection_environment&&) = default;

    virtual ~infection_environment() = default;

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the chance of infecting an agent
    /// @return The chance of infecting an agent
    [[nodiscard]] virtual infection_cycle::precission get_probability() const = 0;

    /// @brief Get the name of the environemnt, for statistic reasons
    /// @details Every environment must have an unique name, to be able to
    /// determine how an agent got infected
    [[nodiscard]] virtual std::string name() const = 0;

}; // class infection_environment

} // namespace sti