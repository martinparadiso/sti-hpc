/// @file infection_logic/object_infection_cycle.hpp
/// @brief The object infection logic
#pragma once

#include <exception>

#include <repast_hpc/RepastProcess.h>

#include "../space_wrapper.hpp"
#include "infection_cycle.hpp"

namespace sti {

/// @brief Represents an object infection cycle: clean or infected
class object_infection_cycle final : public infection_cycle {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////
    /// @brief Struct containing the shared attributes of all infection in humans
    struct flyweight {
        sti::space_wrapper* space;
        precission          infect_chance;

        double infect_distance;
    };

    using flyweight_ptr = const flyweight*;

    ////////////////////////////////////////////////////////////////////////////
    // STAGE
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Stages/states of the object infection
    enum STAGE { CLEAN,
                 INFECTED };

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct an empty object, flyweight is still needed
    /// @param fw The flyweight containing shared data
    object_infection_cycle(flyweight_ptr fw)
        : _id {}
        , _flyweight { fw }
        , _stage {}
    {
    }

    /// @brief Construct an object infection logic
    /// @param id The id of the agent associated with this cycle
    /// @param fw The object flyweight
    /// @param is The initial stage of the object
    object_infection_cycle(const agent_id& id, flyweight_ptr fw, STAGE is)
        : _id { id }
        , _flyweight { fw }
        , _stage { is }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the probability of infecting others
    /// @param position The position of the other agent
    /// @return A value between 0 and 1
    precission get_probability(const position_t& position) const override
    {
        // Get the position of this object
        const auto location = [&]() {
            return _flyweight->space->get_continuous_location(_id);
        }();

        const auto distance = sti::sq_distance(location, position);

        if (_stage == STAGE::CLEAN) return 0.0;
        // Note: equality comparison of floating point values is unreliable, use
        // inequality comparison with near-zero values.
        // A person can only be infected by an object if it's in contact, in
        // this case "in contact" means less than 0.2 meters.
        if (distance >= 0.2 || distance < 0.0) return 0.0;
        return _flyweight->infect_chance;
    }

    /// @brief Get the stage of the object infection
    /// @return An enum, with value CLEAN or INFECTED
    STAGE get_stage() const
    {
        return _stage;
    }

    /// @brief Clean the object, removing contamination
    void clean()
    {
        _stage = STAGE::CLEAN;
    }

    /// @brief Run the infection algorithm, polling nearby agents trying to get infected
    void tick() final;

    /// @brief Set the id
    /// @param id The new id
    void set_id(const agent_id& id)
    {
        _id = id;
    }

private:
    friend class boost::serialization::access;

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _id;
        ar& _stage;
    }

    agent_id      _id;
    flyweight_ptr _flyweight;
    STAGE         _stage;
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
