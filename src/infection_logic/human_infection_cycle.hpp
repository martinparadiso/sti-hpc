/// @file infection_logic/human_infection_cycle.hpp
/// @brief The human infection logic
#pragma once

#include <cassert>
#include <cstdint>
#include <exception>
#include <tuple>

#include <repast_hpc/RepastProcess.h>
#include <repast_hpc/SharedDiscreteSpace.h>

#include "../clock.hpp"
#include "infection_cycle.hpp"

namespace sti {

/// @brief Represents the human infection cycle: healthy, incubating, sick
class human_infection_cycle final : public infection_cycle {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Struct containing the shared attributes of all infection in humans
    struct flyweight {
        repast_space_ptr repast_space;
        clock*           clk;

        precission infect_chance;
        int        infect_distance;
        timedelta  incubation_time;
    };

    using flyweight_ptr = const flyweight*;

    ////////////////////////////////////////////////////////////////////////////
    // STAGE
    ////////////////////////////////////////////////////////////////////////////

    struct bad_stage_cast : public std::exception {
        const char* what() const noexcept final
        {
            return "Exception: Can't cast int to human_infection_cycle::STAGE";
        }
    };

    /// @brief The stages/cycle of a disease
    enum STAGE { HEALTHY,
                 INCUBATING,
                 SICK };

    /// @brief Convert an enum to int
    /// @param stage The stage to serialize
    /// @return The integer representing the enum
    constexpr static std::uint32_t to_int(STAGE stage)
    {
        switch (stage) { // clang-format off
            case STAGE::HEALTHY:    return 0;
            case STAGE::INCUBATING: return 1;
            case STAGE::SICK:       return 2;
        } // clang-format on
    }

    /// @brief Deserialize/convert an int to enum
    /// @param i The number to deserialize/convert to enum
    /// @return the corresponding enum
    /// @throws bad_stage_cast If the enum is out of range
    constexpr static STAGE to_enum(std::uint32_t i)
    {
        if (i == 0) return STAGE::HEALTHY;
        if (i == 1) return STAGE::INCUBATING;
        if (i == 2) return STAGE::SICK;
        throw bad_stage_cast {};
    }

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Default construct a cycle
    human_infection_cycle(flyweight_ptr fw)
        : _id {}
        , _flyweight { fw }
        , _stage {}
        , _infection_time {}
    {
    }

    /// @brief Construct a cycle starting in a given state, specifing the time of infection
    /// @param fw The flyweight containing the shared attributes
    /// @param id The repast agent id
    /// @param stage The initial stage
    /// @param t The time the human became infected
    human_infection_cycle(const agent_id& id,
                          flyweight_ptr   fw,
                          STAGE           stage)
        : _id { id }
        , _flyweight { fw }
        , _stage { stage }
        , _infection_time {}
    {
    }

    /// @brief Construct a cycle starting in a given state, specifing the time of infection
    /// @param fw The flyweight containing the shared attributes
    /// @param id The repast agent id
    /// @param stage The initial stage
    /// @param t The time the human became infected
    human_infection_cycle(const agent_id& id,
                          flyweight_ptr   fw,
                          STAGE           stage,
                          datetime        t)
        : _id { id }
        , _flyweight { fw }
        , _stage { stage }
        , _infection_time { t }
    {
        // The infection time is only applicable when the stage is not healthy
        assert(stage != STAGE::HEALTHY);
    }

    /// @brief Construct a cycle from serialized data
    /// @param fw The flyweight containing the shared attributes
    /// @param id The repast agent id
    /// @param stage The initial stage
    /// @param t The time the human became infected
    human_infection_cycle(const agent_id& id,
                          flyweight_ptr   fw,
                          serial_data&    queue)
        : _id { id }
        , _flyweight { fw }
        , _stage {}
        , _infection_time {}
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
        queue.push(_infection_time.epoch());
    }

    /// @brief Deserialize the data and update the agent data
    /// @param queue The queue containing the data
    void deserialize(serial_data& queue) final
    {
        _id = boost::get<repast::AgentId>(queue.front());
        queue.pop();
        _stage = to_enum(boost::get<std::uint32_t>(queue.front()));
        queue.pop();
        _infection_time = datetime { boost::get<datetime::resolution>(queue.front()) };
        queue.pop();
    }

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Check if the current state of the infection is contagious
    /// @return True if the person is incubating or sick
    bool is_contagious() const
    {
        return _stage != STAGE::HEALTHY;
    };

    /// @brief Get the probability of infecting others
    /// @param position The position of the other agent
    /// @return A value between 0 and 1
    precission get_probability(const position_t& position) const final
    {

        // Get the position of this object
        const auto location = [&]() {
            auto location = std::vector<int> {};
            _flyweight->repast_space->getLocation(_id, location);
            return repast::Point<int>(location);
        }();

        const auto distance = _flyweight->repast_space->getDistanceSq(location, position);

        if (_stage == STAGE::HEALTHY) return 0.0;
        if (distance > static_cast<double>(_flyweight->infect_distance)) return 0.0;
        return _flyweight->infect_chance;
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
    datetime      _infection_time;
}; // class human_infection_cycle

} // namespace sti