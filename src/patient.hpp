/// @file patient.hpp
/// @brief Patient agent
#pragma once

#include <algorithm>
#include <array>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <cstdint>
#include <math.h>
#include <memory>
#include <repast_hpc/Grid.h>
#include <repast_hpc/Point.h>
#include <repast_hpc/SharedContext.h>
#include <sstream>
#include <vector>

#include "chair_manager.hpp"
#include "clock.hpp"
#include "contagious_agent.hpp"
#include "coordinates.hpp"
#include "hospital_plan.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "patient_fsm.hpp"
#include "reception.hpp"
#include "space_wrapper.hpp"
#include "triage.hpp"

namespace sti {

struct patient_flyweight {
    const sti::infection_factory*            inf_factory;
    repast::SharedContext<contagious_agent>* context;
    sti::space_wrapper*                      space;
    sti::clock*                              clk;
    sti::hospital_plan*                      hospital;
    sti::chair_manager*                      chairs;
    sti::reception*                          reception;
    sti::triage*                             triage;
    const double                             walk_speed;
    sti::timedelta                           reception_time;
    sti::timedelta                           triage_duration;
};

/// @brief An agent representing a patient
class patient_agent final : public contagious_agent {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////
    using flyweight     = patient_flyweight;
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
                  const human_infection_cycle& hic);

    /// @brief Create an empty patient
    /// @param id The agent id
    /// @param fw The agent flyweight
    patient_agent(const id_t& id, flyweight_ptr fw);

    // Remove the copy constructors
    patient_agent(const patient_agent&) = delete;
    patient_agent& operator=(const patient_agent&) = delete;

    // Default move
    patient_agent(patient_agent&&) noexcept = default;
    patient_agent& operator=(patient_agent&&) noexcept = default;

    ~patient_agent();

    ////////////////////////////////////////////////////////////////////////////
    // SERIALIZATION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Serialize the agent state into a string using Boost.Serialization
    /// @return A string with the serialized data
    serial_data serialize() override
    {
        auto ss = std::stringstream {};
        { // Used to make sure the stream is flushed
            auto oa = boost::archive::text_oarchive { ss };
            oa << (*this);
        }
        return ss.str();
    }

    /// @brief Reconstruct the agent state from a string using Boost.Serialization
    /// @param data The serialized data
    void serialize(const id_t& id, const serial_data& data) override
    {
        contagious_agent::id(id);
        auto ss = std::stringstream {};
        ss << data;
        { // Used to make sure the stream is flushed
            auto ia = boost::archive::text_iarchive { ss };
            ia >> (*this);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // BAHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of the agent
    /// @returns Patient
    type get_type() const final;

    // TODO: Implement properly
    boost::json::object kill_and_collect() final;

    /// @brief Get the time the patient was admitted at the hospital
    /// @return The date (with a precission of seconds) the patient was admitted
    datetime entry_time() const;

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const infection_cycle* get_infection_logic() const final;

    /// @brief Execute the patient logic, both infection and behaviour
    void act() final;

private:
    friend struct patient_fsm;
    friend class boost::serialization::access;

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _entry_time;
        ar& _infection_logic;
        ar& _fsm;
        ar& _path;
    } // serialize(...)

    flyweight_ptr                    _flyweight;
    datetime                         _entry_time;
    human_infection_cycle            _infection_logic;
    patient_fsm                      _fsm;
    std::vector<coordinates<double>> _path {};
}; // patient_agent

} // namespace sti