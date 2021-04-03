/// @file infection_logic/human_infection_cycle.hpp
/// @brief The human infection logic
#pragma once

#include <cassert>
#include <cstdint>
#include <repast_hpc/AgentId.h>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>

#include "../clock.hpp"
#include "infection_cycle.hpp"

// Fw. declarations
namespace boost {
namespace json {
    class value;
} // namespace json
} // namespace boost

namespace sti {
class clock;
class infection_environment;
class space_wrapper;
} // namespace sti

namespace sti {

/// @brief Represents the human infection cycle: healthy, incubating, sick
class human_infection_cycle final : public infection_cycle {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Struct containing the shared attributes of all infection in humans
    struct flyweight {
        const sti::space_wrapper* space {};
        const sti::clock*         clk {};

        precission infect_probability {};
        precission infect_distance {};
        precission contamination_probability {};
        timedelta  incubation_time;
    };

    using flyweight_ptr   = const flyweight*;
    using environment_ptr = const infection_environment*;

    ////////////////////////////////////////////////////////////////////////////
    // STAGE
    ////////////////////////////////////////////////////////////////////////////

    /// @brief The stages/cycle of a disease
    enum STAGE { HEALTHY,
                 INCUBATING,
                 SICK };

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct an empty human cycle with no internal state
    human_infection_cycle(flyweight_ptr fw, environment_ptr env = nullptr);

    /// @brief Construct a cycle starting in a given state, specifing the time of infection
    /// @param id The id of the agent associated with this patient
    /// @param fw The flyweight containing the shared attributes
    /// @param stage The initial stage
    /// @param infection_time The time of infection
    /// @param env The infection environment this human resides in
    human_infection_cycle(flyweight_ptr          fw,
                          const repast::AgentId& id,
                          STAGE                  stage,
                          datetime               infection_time,
                          environment_ptr        env = nullptr);

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the AgentId associated with this cycle
    /// @return A reference to the agent id
    repast::AgentId& id() override;

    /// @brief Get the AgentId associated with this cycle
    /// @return A reference to the agent id
    const repast::AgentId& id() const override;

    /// @brief Set the infection environment this human resides
    /// @param env_ptr A pointer to the environment
    void set_environment(const infection_environment* env_ptr);

    /// @brief Get the probability of contaminating an object
    /// @return A value in the range [0, 1)
    precission get_contamination_probability() const;

    /// @brief Get the probability of infecting humans
    /// @param position The requesting agent position, to determine the distance
    /// @return A value in the range [0, 1)
    precission get_infect_probability(coordinates<double> position) const override;

    /// @brief Try to get infected via the environment
    void infect_with_environment();

    /// @brief Try go get infected with nearby agents
    void infect_with_nearby();

    /// @brief The infection has time-based stages, this method performs the changes
    void tick();

    ////////////////////////////////////////////////////////////////////////////
    // DATA COLLECTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get statistics about the infection
    /// @return A Boost.JSON value containing relevant statistics
    boost::json::value stats() const;

private:
    friend class boost::serialization::access;

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _id;
        ar& _stage;
        ar& _infection_time;
        ar& _infected_by;
    }

    flyweight_ptr   _flyweight;
    environment_ptr _environment;

    repast::AgentId _id;
    STAGE           _stage;
    datetime        _infection_time;
    std::string     _infected_by;
}; // class human_infection_cycle

} // namespace sti

// Enum serialization
namespace boost {
namespace serialization {

    template <class Archive>
    void serialize(Archive& ar, sti::human_infection_cycle::STAGE& s, const unsigned int /*unused*/)
    {
        ar& s;
    } // void serialize(...)

} // namespace serialization
} // namespaces boost
