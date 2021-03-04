/// @file space_wrapper.hpp
/// @details Repast requires 2 different spaces, discrete and continuous, this is a wrapper to avoid problems
#pragma once

#include <boost/mpi/communicator.hpp>
#include <cstdint>
#include <repast_hpc/SharedContext.h>
#include <repast_hpc/SharedContinuousSpace.h>
#include <repast_hpc/SharedDiscreteSpace.h>
#include <vector>

// #include "contagious_agent.hpp"

// Forward declarations of repast classes
namespace repast {

class Properties;

template <typename T>
class Point;

class AgentId;

class GridDimensions;
}

namespace sti {

// Forward declarations
class plan;
class contagious_agent;

/// @brief A space wrapper
class space_wrapper {

public:
    using agent            = contagious_agent;
    using agent_context    = repast::SharedContext<agent>;
    using continuous_space = repast::SharedContinuousSpace<agent, repast::StrictBorders, repast::SimpleAdder<agent>>;
    using discrete_space   = repast::SharedDiscreteSpace<agent, repast::StrictBorders, repast::SimpleAdder<agent>>;
    using properties       = repast::Properties;
    using communicator     = boost::mpi::communicator;

    using space_unit = double;
    /// @brief Pair of values representing a point in space
    using continuous_point = repast::Point<double>;
    using discrete_point   = repast::Point<int>;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Create a space wrapper
    /// @param building_plan The hospital plan
    /// @param props A repast properties object
    /// @param context The repast agent context
    /// @param comm The Boost.MPI communicator
    space_wrapper(sti::plan& building_plan, properties& props, agent_context& context, communicator* comm);

    ////////////////////////////////////////////////////////////////////////////
    // BEHAVIOUR
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the area simulated in this process
    /// @return The area simulated, consisting of an origin point an the extent
    repast::GridDimensions local_dimensions() const;

    /// @brief Get the discrete location of an agent
    /// @param id The id of the agent
    /// @return The discrete (integral) location of the agent
    discrete_point get_discrete_location(const repast::AgentId& id) const;

    /// @brief Get the continuous location of an agent
    /// @param id The id of the agent
    /// @return The continuous point of the agent
    continuous_point get_continuous_location(const repast::AgentId& id) const;

    /// @brief Get the agents around a certain point of the map
    /// @param p The center of the circle
    /// @param r The radius of the circle
    /// @return A vector containing the agents inside the circle
    std::vector<agent*> agents_around(continuous_point p, double r) const;

    /// @brief Move the agent towards a certain cell
    /// @param id The id of the agent
    /// @param cell The discrete point/cell to move to
    /// @param d Distance to move
    /// @return The agent new effective location
    continuous_point move_towards(const repast::AgentId& id, const discrete_point& cell, space_unit d);

    /// @brief Move the agent to a specific point
    /// @param id The id of the agent
    /// @param point The exact point to move the agent to
    /// @return The agent new effective location
    continuous_point move_to(const repast::AgentId& id, const continuous_point& point);

    /// @brief Move the agent to a specific cell
    /// @details Given that the real space is continuous, the agents is inserted
    ///          in the middle of the cell.
    /// @param id The id of the agent
    /// @param cell The cell to move to
    /// @return The agent new effective location
    continuous_point move_to(const repast::AgentId& id, const discrete_point& cell);

    /// @brief Synchronize the agents between the processes
    void balance();

private:
    continuous_space* _continuous_space;
    discrete_space*   _discrete_space;
};

/// @brief Calculate the distance between two continuous points
/// @return The distance between the points
double sq_distance(const space_wrapper::continuous_point& lho, const space_wrapper::continuous_point& rho);

} // namespace sti
