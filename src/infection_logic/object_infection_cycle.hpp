/// @file infection_logic/object_infection_cycle.hpp
/// @brief The object infection logic
#pragma once

#include <repast_hpc/AgentId.h>
#include <utility>
#include <vector>

#include "../clock.hpp"
#include "../space_wrapper.hpp"
#include "infection_cycle.hpp"

// Fw. declarations
namespace boost {
namespace json {
    class value;
} // namespace json
} // namespace boost

namespace sti {
class human_infection_cycle;
}; // namespace sti

namespace sti {

/// @brief Represents an object infection cycle: clean or infected
class object_infection_cycle final : public infection_cycle {

public:
    using object_type = std::string;

    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////
    /// @brief Struct containing the shared attributes of all infection in humans
    struct flyweight {
        const sti::space_wrapper* space {};
        const sti::clock*         clock {};

        precission infect_chance {};
        precission object_radius {};
        timedelta  cleaning_interval;
    };

    using flyweights_ptr = const std::map<object_type, flyweight>*;

    ////////////////////////////////////////////////////////////////////////////
    // STAGE
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Stages/states of the object infection
    enum class STAGE { CLEAN,
                       CONTAMINATED };

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct an empty object, flyweight is still needed
    /// @param fw The flyweight containing shared data
    object_infection_cycle(flyweights_ptr fw);

    /// @brief Construct an object infection logic
    /// @param fw The object flyweight
    /// @param id The id of the agent associated with this cycle
    /// @param type The object type, i.e. chair, bed
    /// @param is The initial stage of the object
    object_infection_cycle(flyweights_ptr     fw,
                           const agent_id&    id,
                           const object_type& type,
                           STAGE              is);

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get an ID/string to identify the object in post-processing
    /// @return A string identifying the object
    std::string get_id() const override;

    /// @brief Clean the object, removing contamination and resetting the state
    void clean();

    /// @brief Get the probability of contaminating an object
    /// @return A value in the range [0, 1)
    precission get_contamination_probability() const override;

    /// @brief Get the probability of infecting humans
    /// @param position The requesting agent position, to determine the probability
    /// @return A value in the range [0, 1)
    precission get_infect_probability(coordinates<double> position) const override;

    /// @brief Make the object interact with a person, this can contaminate it
    /// @param human A reference to the human infection cycle interacting with this object
    void interact_with(const human_infection_cycle* human);

    /// @brief Try to get infected with nearby/overlaping patients
    void contaminate_with_nearby();

    /// @brief Perform the periodic logic, i.e. clean the object
    void tick();

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
        ar& _object_type;
        ar& _stage;
        // ar& _next_clean;
        ar& _infected_by;
    }

    flyweights_ptr                                _flyweights;
    repast::AgentId                               _id;
    object_type                                   _object_type;
    STAGE                                         _stage;
    datetime                                      _next_clean;
    std::vector<std::pair<std::string, datetime>> _infected_by;
}; // class object_infection_cycle

} // namespace sti

// Enum serialization
namespace boost {
namespace serialization {

    template <class Archive>
    void serialize(Archive& ar, sti::object_infection_cycle::STAGE& s, const unsigned int /*unused*/)
    {
        ar& s;
    } // void serialize(...)

} // namespace serialization
} // namespaces boost
