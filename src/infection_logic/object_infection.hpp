/// @file infection_logic/ghost_object_cycle.hpp
/// @brief Monoprocess objects with no physical representation.
#pragma once

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "infection_cycle.hpp"
#include "../clock.hpp"

// Fw. declarations
namespace boost {
namespace json {
    class value;
} // namespace json
} // namespace boost
namespace sti {
class space_wrapper;
class human_infection_cycle;
} // namespace sti

namespace sti {

class object_infection final : public infection_cycle {
public:
    using object_type = std::string;
    using id_type     = std::pair<std::int32_t, uint32_t>;

    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Struct containing the shared attributes of all object infections
    struct flyweight {
        const sti::space_wrapper* space {};
        const sti::clock*         clock {};

        precission infect_chance {};
        timedelta  cleaning_interval;
    };

    using flyweights_ptr = const std::map<object_type, flyweight>*;

    ////////////////////////////////////////////////////////////////////////////
    // INFECTION STATS
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Struct used to store the infection statistics
    struct infection_stat {
        std::string infected_by;
        datetime    time;
    }; // struct infection_state

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
    object_infection(flyweights_ptr fw);

    /// @brief Construct an object infection logic
    /// @param fw The object flyweight
    /// @param uint_id Unsigned int giving the agent an id
    /// @param type The object type, i.e. chair, bed
    /// @param is The initial stage of the object
    object_infection(flyweights_ptr     fw,
                     id_type            uint_id,
                     const object_type& type,
                     STAGE              is);

    object_infection(const object_infection&) = default;
    object_infection& operator=(const object_infection&) = default;

    object_infection(object_infection&&) = default;
    object_infection& operator=(object_infection&&) = default;

    ~object_infection() = default;

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

    /// @brief Make the object interact with another cycle
    /// @param other A reference to the human infection interacting with this object
    void interact_with(const infection_cycle& other) override;

    /// @brief Perform the periodic logic, i.e. clean the object
    void tick();

    /// @brief Get statistics about the infection
    /// @return A Boost.JSON value containing relevant statistics
    boost::json::value stats() const;

private:
    flyweights_ptr                                _flyweights;
    id_type                                       _id;
    object_type                                   _object_type;
    STAGE                                         _stage;
    datetime                                      _next_clean;
    std::vector<infection_stat> _infected_by;

}; // class object_cycle


inline void tag_invoke(boost::json::value_from_tag /*unused*/, boost::json::value& jv, const object_infection::infection_stat& td)
{
    jv = {
        { "infected_by", td.infected_by },
        { "time", td.time.seconds_since_epoch() }
    };
}

} // namespace sti