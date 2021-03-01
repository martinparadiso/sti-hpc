/// @file patient.hpp
/// @brief Patient agent
#pragma once

#include <algorithm>
#include <cstdint>
#include <math.h>
#include <repast_hpc/Grid.h>
#include <sstream>

#include "chair_manager.hpp"
#include "clock.hpp"
#include "contagious_agent.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "plan/plan.hpp"
#include "print.hpp"

namespace sti {

/// @brief An agent representing a patient
class patient_agent final : public contagious_agent {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Patient common properties
    struct flyweight {
        const infection_factory* inf_factory;
        chair_manager*           chairs;
        plan*                    hospital;
        repast_space_ptr         repast_space;
    };

    using flyweight_ptr = flyweight*;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief A patient cannot be default constructed, unusable
    patient_agent() = delete;

    /// @brief Create a new patient
    /// @param id The agent id
    /// @param fw The flyweight containing the shared properties
    /// @param entry_time The instant the patient enter the building
    patient_agent(const id_t&                  id,
                  flyweight_ptr                fw,
                  const datetime&              entry_time,
                  const human_infection_cycle& hic)
        : contagious_agent { id }
        , _flyweight { fw }
        , _entry_time { entry_time }
        , _infection_logic { hic }
    {
    }

    /// @brief Create a new patient from serialized data
    /// @param id The agent id
    /// @param fw The agent flyweight
    /// @param data The serialized data
    patient_agent(const id_t&   id,
                  flyweight_ptr fw,
                  serial_data&  queue)
        : contagious_agent { id }
        , _flyweight { fw }
        , _entry_time {}
        , _infection_logic { fw->inf_factory->make_human_cycle() }
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
        queue.push(_entry_time.epoch());
        _infection_logic.serialize(queue);
    }

    /// @brief Deserialize the data and update the agent data
    /// @param queue The queue containing the data
    void deserialize(serial_data& queue) final
    {
        _entry_time = datetime { boost::get<datetime::resolution>(queue.front()) };
        queue.pop();
        _infection_logic.deserialize(queue);
    }

    ////////////////////////////////////////////////////////////////////////////
    // BAHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of the agent
    /// @returns Patient
    type get_type() const override
    {
        return contagious_agent::type::PATIENT;
    }

    /// @brief Get the time the patient was admitted at the hospital
    /// @return The date (with a precission of seconds) the patient was admitted
    auto entry_time() const
    {
        return _entry_time;
    }

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const infection_cycle* get_infection_logic() const final
    {
        return &_infection_logic;
    }

    void act() override
    {
        _infection_logic.tick();

        // The patient 0 must request a chair, and walk to it
        if (getId().id() != 0) {

            const auto printas = [&](const auto& str) {
                auto os = std::ostringstream {};
                os << getId() << ": "
                   << str;
                print(os.str());
            };

            const auto convert_coord = [](const std::vector<int>& vec) {
                return plan::coordinates {
                    static_cast<plan::length_type>(vec.at(0)),
                    static_cast<plan::length_type>(vec.at(1))
                };
            };

            const auto get_my_loc = [&]() {
                auto buf = std::vector<int> {};
                _flyweight->repast_space->getLocation(getId(), buf);
                return plan::coordinates {
                    static_cast<plan::length_type>(buf.at(0)),
                    static_cast<plan::length_type>(buf.at(1))
                };
            };

            const auto get_next_location = [&](plan::coordinates dest) -> plan::coordinates {
                const auto my_loc = get_my_loc();
                const auto diff   = plan::coordinates {
                    dest.x - my_loc.x,
                    dest.y - my_loc.y
                };
                // Clamp the movement
                return {
                    my_loc.x + std::clamp(diff.x, -1, 1),
                    my_loc.y + std::clamp(diff.y, -1, 1)
                };
            };

            if (_stage == STAGES::START) {
                // Request a chair
                _flyweight->chairs->request_chair(getId());
                printas("Requested chair");
                _stage = STAGES::WAITING_CHAIR;
                return;
            }

            if (_stage == STAGES::WAITING_CHAIR) {

                // Check if the request has been satisfied
                const auto response = _flyweight->chairs->get_response(getId());
                if (response) {
                    if (response->chair_location) {

                        _chair_assigned = response->chair_location.get();
                        auto os         = std::ostringstream {};
                        os << "Got chair:"
                           << _chair_assigned;
                        printas(os.str());
                        _stage = STAGES::WALKING_TO_CHAIR;
                    } else {
                        auto os = std::ostringstream {};
                        printas("No chair available, leaving");
                        _stage = STAGES::WALKING_TO_EXIT;
                    }
                } else {
                    printas("No response from chair manager");
                }
                return;
            }

            if (_stage == STAGES::WALKING_TO_CHAIR) {

                if (get_my_loc() == _chair_assigned) {
                    printas("Arrived at chair");
                    _stage = STAGES::SIT_DOWN;
                } else {
                    // const auto angles = get_angles_to(_chair_assigned);
                    const auto next_loc = get_next_location(_chair_assigned);
                    auto       os       = std::ostringstream {};
                    os << "Moving to chair :: "
                       << get_my_loc() << " -> "
                       << next_loc;
                    printas(os.str());
                    const auto next_loc_v = std::vector { next_loc.x, next_loc.y };
                    const auto ret        = _flyweight->repast_space->moveTo(this, next_loc_v);
                    assert(ret);
                }

                return;
            }

            if (_stage == STAGES::SIT_DOWN) {
                // Wait a couple of ticks
                if (_ticks_in_chair == 0) {
                    _flyweight->chairs->release_chair(_chair_assigned);
                    _chair_assigned = {};
                    auto os         = std::ostringstream {};
                    os << "Chair released, going to exit at "
                       << _flyweight->hospital->get(plan_tile::TILE_ENUM::EXIT).at(0);
                    printas("Chair released");
                    _stage = STAGES::WALKING_TO_EXIT;
                } else {
                    auto os = std::ostringstream {};
                    os << "Time remaining at chair: "
                       << _ticks_in_chair
                       << " ticks";
                    printas(os.str());
                    --_ticks_in_chair;
                }
                return;
            }

            if (_stage == STAGES::WALKING_TO_EXIT) {
                const auto& exit_loc = _flyweight->hospital->get(plan_tile::TILE_ENUM::EXIT).at(0);

                if (get_my_loc() == exit_loc) {
                    printas("Arrived at exit");
                    _stage = STAGES::DULL;
                } else {
                    // const auto angles = get_angles_to(_chair_assigned);
                    const auto next_loc = get_next_location(exit_loc);
                    auto       os       = std::ostringstream {};
                    os << "Moving to exit :: "
                       << get_my_loc() << " -> "
                       << next_loc;
                    printas(os.str());
                    const auto next_loc_v = std::vector { next_loc.x, next_loc.y };
                    const auto ret        = _flyweight->repast_space->moveTo(this, next_loc_v);
                    assert(ret);
                }
                return;
            };
        }
    }

private:
    flyweight_ptr         _flyweight;
    datetime              _entry_time;
    human_infection_cycle _infection_logic;

    // TODO: Remove this, temp
    enum class STAGES {
        START,
        WAITING_CHAIR,
        WALKING_TO_CHAIR,
        SIT_DOWN,
        WALKING_TO_EXIT,
        DULL,
    };
    STAGES            _stage          = STAGES::START;
    plan::coordinates _chair_assigned = {};
    std::uint32_t     _ticks_in_chair = 5;
}; // patient_agent

} // namespace sti
