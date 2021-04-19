#include "object.hpp"

#include <boost/json/object.hpp>
#include <boost/mpi.hpp>
#include <boost/mpi/detail/antiques.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <vector>

#include "infection_logic/object_infection_cycle.hpp"
#include "json_serialization.hpp"

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Create a new object
/// @param id The repast id associated with this agent
/// @param type The object type, i.e.
/// @param fw Flyweight containing shared information
/// @param oic The infection logic
sti::object_agent::object_agent(const id_t&                   id,
                                const object_type&            type,
                                flyweight_ptr                 fw,
                                const object_infection_cycle& oic)
    : contagious_agent { id }
    , _flyweight { fw }
    , _object_type { type }
    , _infection_logic { oic }
{
}

/// @brief Create an object with no internal data
/// @param id The agent id
/// @param type The object type, i.e.
/// @param fw Object flyweight
sti::object_agent::object_agent(const id_t& id, flyweight_ptr fw)
    : contagious_agent { id }
    , _flyweight { fw }
    , _object_type {}
    , _infection_logic { fw->inf_factory->make_object_cycle() }
{
}

////////////////////////////////////////////////////////////////////////////
// SERIALIZATION
////////////////////////////////////////////////////////////////////////////

/// @brief Serialize the agent state into a string using Boost.Serialization
/// @return A string with the serialized data
sti::serial_data sti::object_agent::serialize(boost::mpi::communicator* communicator)
{
    auto buffer = serial_data {};
    { // Used to make sure the stream is flushed
        auto pa = boost::mpi::packed_oarchive { *communicator, buffer };
        pa << (*this);
    }
    return buffer;
}

/// @brief Reconstruct the agent state from a string using Boost.Serialization
/// @param data The serialized data
void sti::object_agent::serialize(const id_t&               id,
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
sti::object_agent::type sti::object_agent::get_type() const
{
    return type::OBJECT;
}

/// @brief Perform the actions this agents is suppossed to
void sti::object_agent::act()
{
    _infection_logic.contaminate_with_nearby();
}

/// @brief Get the infection logic
/// @return A const pointer to the infection logic
sti::object_infection_cycle* sti::object_agent::get_infection_logic()
{
    return &_infection_logic;
}

/// @brief Get the infection logic
/// @return A const pointer to the infection logic
const sti::object_infection_cycle* sti::object_agent::get_infection_logic() const
{
    return &_infection_logic;
}

/// @brief Get the infection logic
/// @return A pointer to the infection logic
sti::object_infection_cycle* sti::object_agent::get_object_infection_logic()
{
    return &_infection_logic;
}

/// @brief Return the agent statistics as a json object
/// @return A Boost.JSON object containing relevant statistics
boost::json::object sti::object_agent::stats() const
{
    return {
        { "repast_id", to_string(getId()) },
        { "type", _object_type },
        { "infection", _infection_logic.stats() }
    };
}