/// @file patient.hpp
/// @brief Patient agent
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <repast_hpc/Grid.h>
#include <repast_hpc/Point.h>
#include <repast_hpc/SharedContext.h>
#include <sstream>
#include <vector>

#include "contagious_agent.hpp"
#include "coordinates.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "infection_logic/infection_factory.hpp"
#include "patient_fsm.hpp"

// Fw. declarations
namespace sti {
class space_wrapper;
class reception;
class chair_manager;
class triage;
class doctors;
class hospital_plan;
class infection_factory;
class clock;
class icu;
} // namespace sti

namespace sti {

struct patient_flyweight {
    const sti::infection_factory*            inf_factory {};
    repast::SharedContext<contagious_agent>* context {};
    sti::space_wrapper*                      space {};
    sti::clock*                              clk {};
    sti::hospital_plan*                      hospital {};
    sti::chair_manager*                      chairs {};
    sti::reception*                          reception {};
    sti::triage*                             triage {};
    sti::doctors*                            doctors {};
    sti::icu*                                icu {};
    const double                             walk_speed {};
    sti::timedelta                           reception_time;
    sti::timedelta                           triage_duration;
    patient_fsm::flyweight                   fsm {};
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
    /// @param communicator The MPI communicator over which this archive will be sent
    /// @return A string with the serialized data
    void serialize(serial_data& data, boost::mpi::communicator* communicator) override;

    /// @brief Reconstruct the agent state from a string using Boost.Serialization
    /// @param id The new AgentId
    /// @param data The serialized data
    /// @param communicator The MPI communicator over which the archive was sent
    void serialize(const id_t&               id,
                   serial_data&              data,
                   boost::mpi::communicator* communicator) override;

    ////////////////////////////////////////////////////////////////////////////
    // BAHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of the agent
    /// @returns Patient
    type get_type() const final;

    /// @brief Return the agent statistics as a json object
    /// @return A Boost.JSON object containing relevant statistics
    boost::json::object stats() const override;

    /// @brief Get the time the patient was admitted at the hospital
    /// @return The date (with a precission of seconds) the patient was admitted
    datetime entry_time() const;

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    human_infection_cycle* get_infection_logic() override;

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const human_infection_cycle* get_infection_logic() const override;

    /// @brief Get status
    /// @return The current state of the patient FSM
    patient_fsm::STATE current_state() const;

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
    } // serialize(...)

    flyweight_ptr         _flyweight;
    datetime              _entry_time;
    human_infection_cycle _infection_logic;
    patient_fsm           _fsm;
}; // patient_agent

} // namespace sti

BOOST_CLASS_IMPLEMENTATION(sti::patient_agent, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(sti::patient_agent, boost::serialization::track_never)
