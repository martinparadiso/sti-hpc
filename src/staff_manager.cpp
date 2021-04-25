#include "staff_manager.hpp"

#include <boost/json.hpp>
#include <fstream>
#include <repast_hpc/Random.h>
#include <sstream>

#include "agent_factory.hpp"
#include "hospital_plan.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "utils.hpp"

/// @brief Construct a staff_manager
/// @param context The repast agent context
sti::staff_manager::staff_manager(repast::SharedContext<contagious_agent>* context)
    : _context { context }
{
}

/// @brief Create all the hospital staff agents
/// @param af The agent factory
/// @param hospital The hospital plan
/// @param hospital_props Properties of the hospital
void sti::staff_manager::create_staff(agent_factory&             af,
                                      const hospital_plan&       hospital,
                                      const boost::json::object& hospital_props)
{

    const auto immunity_chance = hospital_props.at("parameters").at("personnel").at("immunity").as_double();

    auto is_immune = [&]() -> bool {
        const auto random = repast::Random::instance()->nextDouble();
        return random < immunity_chance;
    };

    for (const auto& doc : hospital.doctors()) {

        const auto* agent = af.insert_new_person(doc.location.continuous(),
                                                 doc.type,
                                                 human_infection_cycle::STAGE::HEALTHY,
                                                 is_immune());
        _created_ids.push_back(agent->getId());
    }

    for (const auto& rec : hospital.receptionists()) {
        const auto* agent = af.insert_new_person(rec.location.continuous(),
                                                 "receptionist",
                                                 human_infection_cycle::STAGE::HEALTHY,
                                                 is_immune());
        _created_ids.push_back(agent->getId());
    }
}

/// @brief Save all the staff agents
/// @param folderpath The folder to save the results to
/// @param rank The process rank
void sti::staff_manager::save(const std::string& folderpath, int rank) const
{
    auto array = boost::json::array {};

    for (const auto& id : _created_ids) {
        const auto& agent = _context->getAgent(id);
        array.push_back(agent->stats());
        _context->removeAgent(agent);
    }

    auto os = std::ostringstream {};
    os << folderpath
       << "/staff.p"
       << rank
       << ".json";
    auto file = std::ofstream { os.str() };
    file << array;
}