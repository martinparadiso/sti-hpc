/// @file person.hpp
/// @brief Person with no logic or mobility, only transmission
#pragma once

#include "patient.hpp"

#include <cstdint>
#include <string>
#include <utility>

#include "contagious_agent.hpp"
#include "infection_logic/human_infection_cycle.hpp"

namespace sti {

/// @brief An agent representing a person with no logic or mobility only tranmission
class person_agent final : public contagious_agent {

public:
    ////////////////////////////////////////////////////////////////////////////
    // FLYWEIGHT
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Person flyweight/common attributes
    struct flyweight {
        const infection_factory* inf_factory;
    };

    using flyweight_ptr = const flyweight*;
    using person_type   = std::string;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Construct a new person
    /// @param id The id of the agent
    /// @param type The 'type' of person. Normally a rol, i.e. radiologist
    /// @param fw The flyweight containing the shared attributes
    /// @param hic The infection logic
    person_agent(const id_t&                  id,
                 const person_type&           type,
                 const flyweight*             fw,
                 const human_infection_cycle& hic);

    /// @brief Create an empty person
    /// @param fw The agent flyweight
    person_agent(const id_t& id, const flyweight* fw);

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
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the type of this agent
    /// @return The type of the agent
    type get_type() const override;

    /// @brief Perform the actions this agent is supposed to
    void act() final;

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    human_infection_cycle* get_infection_logic() override;

    /// @brief Get the infection logic
    /// @return A pointer to the infection logic
    const human_infection_cycle* get_infection_logic() const override;

    /// @brief Return the agent statistics as a json object
    /// @return A Boost.JSON object containing relevant statistics
    boost::json::object stats() const override;

private:
    friend class boost::serialization::access;

    // Private serialization, for security
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*unused*/)
    {
        ar& _type;
        ar& _infection_logic;
    }

    flyweight_ptr         _flyweight;
    person_type           _type;
    human_infection_cycle _infection_logic;

}; // class person_agent

} // namespace sti

BOOST_CLASS_IMPLEMENTATION(sti::person_agent, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(sti::person_agent, boost::serialization::track_never)
