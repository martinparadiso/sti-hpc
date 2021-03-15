/// @file infection_logic/human_infection_cycle.hpp
/// @brief The human infection logic
#pragma once

#include <cassert>
#include <cstdint>
#include <exception>
#include <optional>
#include <tuple>

#include <repast_hpc/RepastProcess.h>

#include "../clock.hpp"
#include "../space_wrapper.hpp"
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
        sti::space_wrapper* space;
        sti::clock*         clk;

        precission infect_chance;
        double     infect_distance;
        timedelta  incubation_time;
    };

    using flyweight_ptr = const flyweight*;

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

    /// @brief Construct a human cycle with no internal state
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
            return _flyweight->space->get_continuous_location(_id);
        }();

        const auto distance = sti::sq_distance(location, position);

        if (_stage == STAGE::HEALTHY) return 0.0;
        if (distance > static_cast<double>(_flyweight->infect_distance)) return 0.0;
        return _flyweight->infect_chance;
    }

    /// @brief Get the stage of the cycle
    /// @return An enum of type HEALTHY, INCUBATING or SICK
    STAGE get_stage() const
    {
        return _stage;
    }

    /// @brief Get the time of infection
    /// @return If the person was infected, the time of infection
    std::optional<sti::datetime> get_infection_time() const
    {
        if (_infection_time.epoch() != 0) {
            return _infection_time;
        }
        return {};
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
        ar& _infection_time;
    }

    agent_id      _id;
    flyweight_ptr _flyweight;
    STAGE         _stage;
    datetime      _infection_time;
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
