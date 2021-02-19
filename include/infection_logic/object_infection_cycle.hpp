/// @file infection_logic/object_infection_cycle.hpp
/// @brief The object infection logic
#pragma once

#include <exception>

#include <repast_hpc/RepastProcess.h>
#include <repast_hpc/SharedDiscreteSpace.h>

#include "infection_cycle.hpp"

namespace sti {

class parallel_agent;
using repast_space =repast::SharedDiscreteSpace<parallel_agent, repast::StrictBorders, repast::SimpleAdder<parallel_agent>>;
using repast_space_ptr = const repast_space*;

/// @brief Represents an object infection cycle: clean or infected
class object_infection_cycle final : infection_cycle {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////
    /// @brief Struct containing the shared attributes of all infection in humans
    struct flyweight {
        repast_space_ptr repast_space;
        precission infect_chance;
    };

    using flyweight_ptr = const flyweight*;

    ////////////////////////////////////////////////////////////////////////////
    // STAGE
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Stages/states of the object infection
    enum STAGE { CLEAN,
                 INFECTED };

    /// @brief Error trying to convert an int to an ENUM
    struct bad_stage_cast : public std::exception {
        const char* what() const noexcept override
        {
            return "Exception: Can't cast int to object_infection_cycle::STAGE";
        }
    };

    /// @brief Convert an enum to int
    /// @param stage The stage to serialize
    /// @return The integer representing the enum
    constexpr static std::uint32_t to_int(STAGE stage)
    {
        switch (stage) { // clang-format off
            case STAGE::CLEAN:    return 0;
            case STAGE::INFECTED: return 1;
        } // clang-format on
    }

    /// @brief Deserialize/convert an int to enum
    /// @param i The number to deserialize/convert to enum
    /// @return the corresponding enum
    /// @throws bad_stage_cast If the enum is out of range
    constexpr static STAGE get_stage(std::uint32_t i)
    {
        if (i == 0) return STAGE::CLEAN;
        if (i == 1) return STAGE::INFECTED;
        throw bad_stage_cast {};
    }

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialization tuple used to serialize and deserialize the object
    struct serialization_pack {
        std::uint32_t stage;
    };

    /// @brief Construct an object infection logic
    /// @param fw The object flyweight
    /// @param id The id of the agent associated with this cycle
    /// @param is The initial stage of the object
    object_infection_cycle(flyweight_ptr fw, const agent_id& id, STAGE is)
        : _flyweight { fw }
        , _id { id }
        , _stage { is }
    {
    }

    /// @brief Construct an object infection logic
    /// @param fw The object flyweight
    /// @param id The id of the agent associated with this cycle
    /// @param sp The serialized data
    object_infection_cycle(flyweight_ptr fw, const agent_id& id, const serialization_pack& sp)
        : _flyweight { fw }
        , _id { id }
        , _stage { get_stage(sp.stage) }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Update the internal state of the infection
    /// @param id The new id of the agent associated with this logic
    /// @param sp Serialization pack, containing the information to update
    void update(const agent_id& id, const serialization_pack& sp)
    {
        _id    = id;
        _stage = get_stage(sp.stage);
    }

    /// @brief Serialize the internal state of the infection logic
    /// @return A struct containing the serialized data
    serialization_pack serialize() const
    {
        return { to_int(_stage) };
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
            auto location = std::vector<int>{};
            _flyweight->repast_space->getLocation(_id, location);
            return repast::Point<int>(location);
        }();

        const auto distance = _flyweight->repast_space->getDistanceSq(location, position);

        if (_stage == STAGE::CLEAN) return 0.0;
        // Note: equality comparison of floating point values is unreliable, use
        // inequality comparison with near-zero values.
        // A person can only be infected by an object if it's in contact, in
        // this case "in contact" means less than 0.2 meters.
        if (distance >= 0.2 || distance < 0.0) return 0.0;
        return _flyweight->infect_chance;
    }

    /// @brief Clean the object, removing contamination
    void clean()
    {
        _stage = STAGE::CLEAN;
    }

private:
    flyweight_ptr _flyweight;
    agent_id      _id;
    STAGE         _stage;
}; // class object_infection_cycle

} // namespace sti
