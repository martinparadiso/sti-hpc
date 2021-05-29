#include "person.hpp"

#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/json/value.hpp>

#include "infection_logic/human_infection_cycle.hpp"
#include "json_serialization.hpp"

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct a new person
/// @param id The id of the agent
/// @param type The 'type' of person. Normally a rol, i.e. radiologist
/// @param fw The flyweight containing the shared attributes
/// @param hic The infection logic
sti::person_agent::person_agent(const id_t&                  id,
                                const person_type&           type,
                                const flyweight*             fw,
                                const human_infection_cycle& hic)
    : contagious_agent { id }
    , _flyweight { fw }
    , _type { type }
    , _infection_logic { hic }
{
}

/// @brief Create an empty person
/// @param fw The agent flyweight
sti::person_agent::person_agent(const id_t& id, const flyweight* fw)
    : contagious_agent { id }
    , _flyweight { fw }
    , _infection_logic { fw->inf_factory->make_human_cycle() }
{
}

////////////////////////////////////////////////////////////////////////////
// SERIALIZATION
////////////////////////////////////////////////////////////////////////////

/// @brief Serialize the agent state into a string using Boost.Serialization
/// @return A string with the serialized data
void sti::person_agent::serialize(serial_data& data, boost::mpi::communicator* communicator)
{
    // Used to make sure the stream is flushed
    auto pa = boost::mpi::packed_oarchive { *communicator, data };
    pa << (*this);
}

/// @brief Reconstruct the agent state from a string using Boost.Serialization
/// @param data The serialized data
void sti::person_agent::serialize(const id_t&               id,
                                  serial_data&              data,
                                  boost::mpi::communicator* communicator)
{
    contagious_agent::id(id);
    { // Used to make sure the stream is flushed
        auto ia = boost::mpi::packed_iarchive { *communicator, data };
        ia >> (*this);
    }
}

////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Get the type of this agent
/// @return The type of the agent
sti::person_agent::type sti::person_agent::get_type() const
{
    return type::FIXED_PERSON;
}

/// @brief Perform the actions this agent is supposed to
void sti::person_agent::act()
{
    _infection_logic.infect_with_environment();
    _infection_logic.infect_with_nearby();
    _infection_logic.tick();
}

/// @brief Get the infection logic
/// @return A pointer to the infection logic
sti::human_infection_cycle* sti::person_agent::get_infection_logic()
{
    return &_infection_logic;
}

/// @brief Get the infection logic
/// @return A const pointer to the infection logic
const sti::human_infection_cycle* sti::person_agent::get_infection_logic() const
{
    return &_infection_logic;
}

/// @brief Return the agent statistics as a json object
/// @return A Boost.JSON object containing relevant statistics
boost::json::object sti::person_agent::stats() const
{
    return {
        { "repast_id", to_string(getId()) },
        { "type", _type },
        { "infection", _infection_logic.stats() }
    };
}