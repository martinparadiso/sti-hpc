/// @file staff_manager.hpp
/// @brief Hospital staff manager
#pragma once

#include <repast_hpc/SharedContext.h>
#include <boost/json/array.hpp>
#include <string>
#include <vector>

#include "coordinates.hpp"
#include "person.hpp"

// Fw. declarations
namespace boost {
namespace json {
    class object;
} // namespace json
} // namespace boost
namespace repast {
class AgentId;
} // namespace repast
namespace sti {
class hospital_plan;
class agent_factory;
class contagious_agent;
class space_wrapper;
} // namespace sti

namespace sti {

/// @brief Manage the creation, maintenance and destruction of hospital staff
class staff_manager {

public:
    /// @brief Construct a staff_manager
    /// @param context The repast agent context
    /// @param af The agent factory
    /// @param spaces The space wrapper
    /// @param hospital The hospital plan
    /// @param hospital_props Properties of the hospital
    staff_manager(repast::SharedContext<contagious_agent>* context,
                  agent_factory*                           af,
                  space_wrapper*                           spaces,
                  const hospital_plan*                     hospital,
                  const boost::json::object*               hospital_props);

    /// @brief Create all the hospital staff agents
    void create_staff();

    /// @brief Execute periodic staff logic, i.e. replace sick personnal
    void tick();

    /// @brief Save
    /// @param folderpath The folder to save the results to
    /// @param rank The process rank
    void save(const std::string& folderpath, int rank) const;

private:
    /// @brief Create a person of a given type
    /// @param location The location to insert the person into
    /// @param type The person type
    person_agent* create_person(const sti::coordinates<double>&  location,
                                const person_agent::person_type& type);

    repast::SharedContext<contagious_agent>* _context;
    agent_factory*                           _agent_factory;
    space_wrapper*                           _spaces;
    const hospital_plan*                     _hospital_plan;
    const boost::json::object*               _hospital_props;

    boost::json::array         _removed_staff;
    std::vector<person_agent*> _created;
}; // class staff_manager

} // namespace sti