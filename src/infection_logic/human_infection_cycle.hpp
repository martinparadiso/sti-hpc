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
#include "../coordinates.hpp"

// Fw. declarations
namespace boost {
namespace json {
    class value;
} // namespace json
} // namespace boost

namespace sti {
class clock;
class infection_environment;
class object_infection;
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
        timedelta  min_incubation_time;
        timedelta  max_incubation_time;
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
    // "MODE"
    ////////////////////////////////////////////////////////////////////////////

    /// @details The human infection logic has diverse "modes". Depending on the
    /// situation, that behave differently
    enum class MODE { // clang-format off
        NORMAL, // Normal human infection cycle, gets infected and contaminates
        IMMUNE, // Immune mode, cannot get infected
        COMA,   // The patient has no physical location, cannot infect nearby
                // agents. Only interacts with the assigned bed and the
                // environment
    }; // clang-format on

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct an empty human cycle with no internal state
    human_infection_cycle(flyweight_ptr fw, environment_ptr env = nullptr);

    /// @brief Construct a cycle starting in a given state, specifing the time of infection
    /// @param id The id of the agent associated with this patient
    /// @param fw The flyweight containing the shared attributes
    /// @param stage The initial stage
    /// @param mode The "mode"
    /// @param infection_time The time of infection
    /// @param env The infection environment this human resides in
    human_infection_cycle(flyweight_ptr          fw,
                          const repast::AgentId& id,
                          STAGE                  stage,
                          MODE                   mode,
                          datetime               infection_time,
                          environment_ptr        env = nullptr);

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Change the infection mode
    /// @param new_mode The new mode to use in this infection
    void mode(MODE new_mode);

    /// @brief Get an ID/string to identify the object in post-processing
    /// @return A string identifying the object
    std::string get_id() const override;

    /// @brief Set the infection environment this human resides
    /// @param env_ptr A pointer to the environment
    void set_environment(const infection_environment* env_ptr);

    /// @brief Get the probability of contaminating an object
    /// @return A value in the range [0, 1)
    precission get_contamination_probability() const override;

    /// @brief Get the probability of infecting humans
    /// @param position The requesting agent position, to determine the distance
    /// @return A value in the range [0, 1)
    precission get_infect_probability(coordinates<double> position) const override;

    /// @brief Check if the person is sick
    bool is_sick() const;

    /// @brief Make the human interact with another infection logic
    /// @param other The other infection cycle
    void interact_with(const infection_cycle& other) override;

    /// @brief The infection has time-based stages, this method performs the changes
    void tick();

    ////////////////////////////////////////////////////////////////////////////
    // DATA COLLECTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get statistics about the infection
    /// @return A Boost.JSON value containing relevant statistics
    boost::json::value stats() const;

private:

    /// @brief Try to get infected via the environment
    void infect_with_environment();

    /// @brief Try go get infected with nearby agents
    void infect_with_nearby();
    
    friend class boost::serialization::access;

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _id;
        ar& _stage;
        ar& _infection_time;
        ar& _infected_by;
        ar& _infect_location;
        ar& _incubation_end;
        ar& _mode;
    }

    /// @brief Indicate that the patient has been infected
    /// @param infected_by Who infected the agent
    void infected(const std::string& infected_by);

    flyweight_ptr   _flyweight;
    environment_ptr _environment;

    repast::AgentId  _id;
    STAGE            _stage;
    MODE             _mode;
    datetime         _infection_time;
    datetime         _incubation_end;
    std::string      _infected_by;
    coordinates<int> _infect_location;

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

    template <class Archive>
    void serialize(Archive& ar, sti::human_infection_cycle::MODE& m, const unsigned int /*unused*/)
    {
        ar& m;
    } // void serialize(...)

} // namespace serialization
} // namespaces boost

BOOST_CLASS_IMPLEMENTATION(sti::human_infection_cycle, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(sti::human_infection_cycle, boost::serialization::track_never)
