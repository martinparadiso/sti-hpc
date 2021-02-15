/// @brief Classes representing the infection evolution over time
/// @details The infection evolution over time is differnet for different types
///          of agents. For instance chairs don't have an incubation period.
#pragma once

#include <array>
#include <type_traits>

namespace sti {

class human_infection_cycle;
class object_infection_cycle;

/// @brief Collection of common parameters used by the different infection cycles
template<typename T>
struct infection_parameters {
    static_assert(! std::is_same_v<T,T>, "Unsupported infection");
};

/// @brief Parameters used by the human infection cycle, i.e. stages durations
template<>
struct infection_parameters<human_infection_cycle> {

    // TODO: Maybe this is a bad idea, and we need to put more thing here, like
    // distance, to concentrate all parameters in a single class
};

/// @brief Represents the human infection cycle: healthy, incubating, sick
class human_infection_cycle {

public:
    enum STATE { HEALTHY,
                 INCUBATING,
                 SICK };

    human_infection_cycle() = delete;

    /// @brief Construct a cycle starting in a given state
    human_infection_cycle(STATE state)
        : _state { state }
    {
    }

    /// @brief Check if the current state of the infection is contagious
    /// @return True if the person is incubating or sick
    bool is_contagious() const
    {
        return _state != STATE::HEALTHY;
    };

private:
    STATE _state;
    // const std::smart_ptr<> TODO: ???
};
 
/// @brief Represents an object infection cycle: clean or infected
class object_infection_cycle {

public:
    enum state { CLEAN,
                 INFECTED };

private:
    state _state;
};

} // namespace sti
