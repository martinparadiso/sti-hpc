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

class contagious_agent;
using agent            = contagious_agent;
using repast_space     = repast::SharedDiscreteSpace<agent, repast::StrictBorders, repast::SimpleAdder<agent>>;
using repast_space_ptr = const repast_space*;

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

        precission                infect_chance;
        int                       infect_distance;
        clock::date_t::resolution incubation_time;
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
    constexpr static STAGE get_stage(std::uint32_t i)
    {

        if (i == 0) return STAGE::HEALTHY;
        if (i == 1) return STAGE::INCUBATING;
        if (i == 2) return STAGE::SICK;
        throw bad_stage_cast {};
    }

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialization tuple used to serialize and deserialize the object
    // using serialization_pack = std::tuple<std::uint32_t, clock::date_t::resolution>;
    struct serialization_pack {
        std::uint32_t             stage;
        clock::date_t::resolution infection_time;
    };

    human_infection_cycle() = delete;

    /// @brief Construct a cycle starting in a given state
    /// @param fw The flyweight containing the shared attributes
    /// @param id The repast agent id
    /// @param stage The initial stage
    human_infection_cycle(flyweight_ptr fw, const agent_id& id, STAGE stage)
        : _flyweight { fw }
        , _id { id }
        , _stage { stage }
        , _infection_time { stage != STAGE::HEALTHY ? 0UL : _flyweight->clk->now() }
    {
    }

    /// @brief Construct a cycle starting in a given state, specifing the time of infection
    /// @param fw The flyweight containing the shared attributes
    /// @param id The repast agent id
    /// @param stage The initial stage
    /// @param t The time the human became infected
    human_infection_cycle(flyweight_ptr   fw,
                          const agent_id& id,
                          STAGE           stage,
                          clock::date_t   t)
        : _flyweight { fw }
        , _id { id }
        , _stage { stage }
        , _infection_time { t }
    {
    }

    /// @brief Construct a cycle from serialized data
    /// @param fw The flyweight containing the shared attributes
    /// @param id The repast agent id
    /// @param sp The serialized data
    human_infection_cycle(flyweight_ptr             fw,
                          const agent_id&           id,
                          const serialization_pack& sp)
        : _flyweight { fw }
        , _id { id }
        , _stage { get_stage(sp.stage) }
        , _infection_time { sp.infection_time }
    {
    }

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Update the internal state of the infection
    /// @param id The new id of the agent associated with this logic
    /// @param sp Serialization pack, containing the information to recreate
    void update(const agent_id& id, const serialization_pack& sp)
    {
        _id             = id;
        _stage          = get_stage(sp.stage);
        _infection_time = sp.infection_time;
    }

    /// @brief Serialize the internal state of the infection logic
    /// @return A struct containing the serialized data
    serialization_pack serialize() const
    {
        return { to_int(_stage), _infection_time.epoch() };
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

private:
    flyweight_ptr _flyweight;
    agent_id      _id;
    STAGE         _stage;
    clock::date_t _infection_time;
}; // class human_infection_cycle

} // namespace sti