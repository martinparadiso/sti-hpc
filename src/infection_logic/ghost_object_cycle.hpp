/// @file infection_logic/ghost_object_cycle.hpp
/// @brief Monoprocess objects with no physical representation.
#pragma once

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "infection_cycle.hpp"
#include "object_infection_cycle.hpp"
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

class ghost_object_cycle final : public infection_cycle {
public:
    using object_type = std::string;
    using id_type     = std::pair<std::int32_t, uint32_t>;

    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    using flyweights_ptr = const std::map<object_type, object_infection_cycle::flyweight>*;

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
    ghost_object_cycle(flyweights_ptr fw);

    /// @brief Construct an object infection logic
    /// @param fw The object flyweight
    /// @param uint_id Unsigned int giving the agent an id
    /// @param type The object type, i.e. chair, bed
    /// @param is The initial stage of the object
    ghost_object_cycle(flyweights_ptr     fw,
                       id_type            uint_id,
                       const object_type& type,
                       STAGE              is);

    ghost_object_cycle(const ghost_object_cycle&) = default;
    ghost_object_cycle& operator=(const ghost_object_cycle&) = default;

    ghost_object_cycle(ghost_object_cycle&&) = default;
    ghost_object_cycle& operator=(ghost_object_cycle&&) = default;

    ~ghost_object_cycle() = default;

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

    /// @brief Make the object interact with a human
    /// @param human A reference to the human infection interacting with this object
    void interact_with(const infection_cycle* other);

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
    std::vector<std::pair<std::string, datetime>> _infected_by;

}; // class ghost_object_cycle

} // namespace sti