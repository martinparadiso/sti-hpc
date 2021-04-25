/// @file staff_manager.hpp
/// @brief Hospital staff manager
#pragma once

#include <repast_hpc/SharedContext.h>
#include <string>
#include <vector>

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
    staff_manager(repast::SharedContext<contagious_agent>* context);

    /// @brief Create all the hospital staff agents
    /// @param af The agent factory
    /// @param spaces The space wrapper
    /// @param hospital The hospital plan
    /// @param hospital_props Properties of the hospital
    void create_staff(agent_factory&             af,
                      const space_wrapper&       spaces,
                      const hospital_plan&       hospital,
                      const boost::json::object& hospital_props);

    /// @brief Save
    /// @param folderpath The folder to save the results to
    /// @param rank The process rank
    void save(const std::string& folderpath, int rank) const;

private:
    repast::SharedContext<contagious_agent>* _context;
    std::vector<repast::AgentId>             _created_ids;
}; // class staff_manager

} // namespace sti