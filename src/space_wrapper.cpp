#include "space_wrapper.hpp"

#include <cmath>
#include <repast_hpc/AgentId.h>
#include <repast_hpc/Moore2DGridQuery.h>
#include <repast_hpc/Point.h>
#include <repast_hpc/Properties.h>

#include "hospital_plan.hpp"
#include "contagious_agent.hpp"

/// @brief Create a space wrapper
/// @param building_plan The hospital plan
/// @param props A repast properties object
/// @param context The repast agent context
/// @param comm The Boost.MPI communicator
sti::space_wrapper::space_wrapper(sti::hospital_plan& building_plan, properties& props, agent_context& context, communicator* comm)
{

    const auto origin             = repast::Point<double> { 0, 0 };
    const auto extent             = repast::Point<double> { static_cast<double>(building_plan.width()), static_cast<double>(building_plan.height()) };
    const auto grid_dimensions    = repast::GridDimensions { origin, extent };
    const auto process_dimensions = std::vector<int> { repast::strToInt(props.getProperty("x.process")),
                                                       repast::strToInt(props.getProperty("y.process")) };

    _discrete_space = new discrete_space {
        "ParallelAgentDiscreteSpace",
        grid_dimensions,
        process_dimensions,
        2,
        comm
    };
    _continuous_space = new continuous_space {
        "ParallelAgentContinuousSpace",
        grid_dimensions,
        process_dimensions,
        0,
        comm
    };

    context.addProjection(_discrete_space);
    context.addProjection(_continuous_space);
}

/// @brief Get the area simulated in this process
/// @return The area simulated, consisting of an origin point an the extent
repast::GridDimensions sti::space_wrapper::local_dimensions() const
{
    return _continuous_space->dimensions();
}

/// @brief Get the discrete location of an agent
/// @param id The id of the agent
/// @return The discrete (integral) location of the agent
sti::space_wrapper::discrete_point sti::space_wrapper::get_discrete_location(const repast::AgentId& id) const
{
    auto buff = std::vector<int> {};
    _discrete_space->getLocation(id, buff);
    return {
        buff.at(0),
        buff.at(1)
    };
}

/// @brief Get the continuous location of an agent
/// @param id The id of the agent
/// @return The continuous point of the agent
sti::space_wrapper::continuous_point sti::space_wrapper::get_continuous_location(const repast::AgentId& id) const
{
    auto buff = std::vector<double> {};
    _continuous_space->getLocation(id, buff);
    return {
        buff.at(0),
        buff.at(1)
    };
}

/// @brief Get the agents around a certain point of the map
/// @param p The center of the circle
/// @param r The radius of the circle
/// @return A vector containing the agents inside the circle
std::vector<sti::space_wrapper::agent*> sti::space_wrapper::agents_around(continuous_point p,
                                                                          double           r) const
{
    const auto cell = repast::Point<int> {
        static_cast<int>(p.getX()),
        static_cast<int>(p.getY())
    };
    const auto range = static_cast<int>(std::ceil(r));

    // Coarse search
    auto buf   = std::vector<agent*> {};
    auto query = repast::Moore2DGridQuery<agent> { _discrete_space };
    query.query(cell, range, true, buf);

    // Fine search
    buf.erase(std::remove_if(buf.begin(),
                             buf.end(),
                             [&](const auto& agent) {
                                 const auto loc = get_continuous_location(agent->getId());
                                 const auto d   = _continuous_space->getDistanceSq(p, loc);
                                 return d > r;
                             }),
              buf.end());

    return buf;
}

/// @brief Move the agent towards a certain cell
/// @param id The id of the agent
/// @param cell The discrete point/cell to move to
/// @param d Distance to move
/// @return The agent new effective location
sti::space_wrapper::continuous_point sti::space_wrapper::move_towards(const repast::AgentId& id, const discrete_point& cell, space_unit d)
{
    const auto agent_pt = [&]() {
        auto buf = std::vector<double> {};
        _continuous_space->getLocation(id, buf);
        return repast::Point<double> {
            buf.at(0),
            buf.at(1)
        };
    }();

    auto target_pt = repast::Point<double> {
        static_cast<double>(cell.getX() + 0.5),
        static_cast<double>(cell.getY() + 0.5)
    };

    // The movement should be in the direction of the point, with the length of d
    auto vector = repast::Point<double> {
        target_pt.getX() - agent_pt.getX(),
        target_pt.getY() - target_pt.getY()
    };

    // Calculate the distance in each axis
    auto x = target_pt.getX() - agent_pt.getX();
    auto y = target_pt.getY() - agent_pt.getY();

    // Calculate the length of the vector
    const auto length = std::sqrt(x * x + y * y);

    // Clamp the speed/length, maximum should be the distance to the final point
    d = std::min(d, length);

    // If the length is 0 (the agent is already at that location), don't move
    if (d == 0.0) {
        return target_pt;
    }

    x *= d / length;
    y *= d / length;

    const auto new_point = repast::Point<double> {
        agent_pt.getX() + x,
        agent_pt.getY() + y
    };

    return move_to(id, new_point);
}

/// @brief Move the agent to a specific point
/// @param id The id of the agent
/// @param point The exact point to move the agent to
/// @return The agent new effective location
sti::space_wrapper::continuous_point sti::space_wrapper::move_to(const repast::AgentId& id, const continuous_point& point)
{

    const auto cell = repast::Point<int> {
        static_cast<int>(point.getX()),
        static_cast<int>(point.getY())
    };

    _discrete_space->moveTo(id, cell);
    _continuous_space->moveTo(id, point);

    return point;
}

/// @brief Move the agent to a specific cell
/// @details Given that the real space is continuous, the agents is inserted
///          in the middle of the cell.
/// @param id The id of the agent
/// @param point The cell to move to
/// @return The agent new effective location
sti::space_wrapper::continuous_point sti::space_wrapper::move_to(const repast::AgentId& id, const discrete_point& cell)
{
    auto point = repast::Point<double> {
        static_cast<double>(cell.getX()) + 0.5,
        static_cast<double>(cell.getY()) + 0.5
    };

    _discrete_space->moveTo(id, cell);
    _continuous_space->moveTo(id, point);

    return point;
}

/// @brief Synchronize the agents between the processes
void sti::space_wrapper::balance()
{
    _discrete_space->balance();
}

/// @brief Calculate the distance between two continuous points
/// @return The distance between the points
double sti::sq_distance(const space_wrapper::continuous_point& lho, const space_wrapper::continuous_point& rho)
{
    const auto diff = repast::Point<double> {
        lho.getX() - rho.getX(),
        lho.getY() - rho.getY()
    };
    return sqrt(diff.getX() * diff.getX() + diff.getY() * diff.getY());
}
