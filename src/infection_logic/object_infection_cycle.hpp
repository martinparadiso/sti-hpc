/// @file infection_logic/object_infection_cycle.hpp
/// @brief The object infection logic
#pragma once

#include <exception>

#include <repast_hpc/RepastProcess.h>
#include <repast_hpc/SharedDiscreteSpace.h>

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
        repast_space_ptr repast_space;
        precission       infect_chance;

        int infect_distance;
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
    constexpr static STAGE to_enum(std::uint32_t i)
    {
        if (i == 0) return STAGE::CLEAN;
        if (i == 1) return STAGE::INFECTED;
        throw bad_stage_cast {};
    }

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Default construct an empty object, flyweight is still needed
    /// @param fw The flyweight containing shared data
    object_infection_cycle(flyweight_ptr fw)
        : _id {}
        , _flyweight {fw}
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

    /// @brief Construct an object infection logic from serialized data
    /// @param id The id of the agent associated with this cycle
    /// @param fw The object flyweight
    /// @param queue The serialized data
    object_infection_cycle(const agent_id& id, flyweight_ptr fw, serial_data& queue)
        : _id { id }
        , _flyweight { fw }
        , _stage {}
    {
        deserialize(queue);
    }

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the internal state of the infection
    /// @param queue The queue to store the data
    void serialize(serial_data& queue) const final
    {
        queue.push(_id);
        queue.push(to_int(_stage));
    }

    /// @brief Deserialize the data and update the agent data
    /// @param queue The queue containing the data
    void deserialize(serial_data& queue) final
    {
        _id = boost::get<repast::AgentId>(queue.front());
        queue.pop();
        _stage = to_enum(boost::get<std::uint32_t>(queue.front()));
        queue.pop();
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
            auto location = std::vector<int> {};
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

    /// @brief Run the infection algorithm, polling nearby agents trying to get infected
    void tick() final;

    /// @brief Set the id
    /// @param id The new id
    void set_id(const agent_id& id)
    {
        _id = id;
    }

private:
    agent_id      _id;
    flyweight_ptr _flyweight;
    STAGE         _stage;
}; // class object_infection_cycle

} // namespace sti
