#include "staff_manager.hpp"

#include <boost/json.hpp>
#include <fstream>
#include <repast_hpc/Random.h>
#include <sstream>

#include "agent_factory.hpp"
#include "hospital_plan.hpp"
#include "infection_logic/human_infection_cycle.hpp"
#include "person.hpp"
#include "utils.hpp"
#include "space_wrapper.hpp"

/// @brief Construct a staff_manager
/// @param context The repast agent context
/// @param af The agent factory
/// @param spaces The space wrapper
/// @param hospital The hospital plan
/// @param hospital_props Properties of the hospital
sti::staff_manager::staff_manager(repast::SharedContext<contagious_agent>* context,
                                  agent_factory*                           af,
                                  space_wrapper*                           spaces,
                                  const hospital_plan*                     hospital,
                                  const boost::json::object*               hospital_props)
    : _context { context }
    , _agent_factory { af }
    , _spaces { spaces }
    , _hospital_plan { hospital }
    , _hospital_props { hospital_props }
{
}

/// @brief Create a person of a given type
/// @param location The location to insert the person into
/// @param type The person type
sti::person_agent* sti::staff_manager::create_person(const sti::coordinates<double>&  location,
                                                     const person_agent::person_type& type)
{

    const auto immunity_chance = _hospital_props->at("parameters").at("personnel").at("immunity").as_double();

    auto is_immune = [&]() -> bool {
        const auto random = repast::Random::instance()->nextDouble();
        return random < immunity_chance;
    };

    return _agent_factory->insert_new_person(location,
                                             type,
                                             human_infection_cycle::STAGE::HEALTHY,
                                             is_immune());
}

/// @brief Create all the hospital staff agents
void sti::staff_manager::create_staff()
{

    for (const auto& doc : _hospital_plan->doctors()) {

        if (_spaces->local_dimensions().contains(doc.location)) {

            auto* agent = create_person(doc.location.continuous(), doc.type);
            _created.push_back(agent);
        }
    }

    for (const auto& rec : _hospital_plan->receptionists()) {
        if (_spaces->local_dimensions().contains(rec.location)) {
            auto* agent = create_person(rec.location.continuous(), "receptionist");
            _created.push_back(agent);
        }
    }
}

/// @brief Execute periodic staff logic, i.e. replace sick personnal
void sti::staff_manager::tick()
{
    // Technically the vector is not being modified, but rather the pointers, so
    // this shouldn't explode
    for (auto& person : _created) {
        if (person->get_infection_logic()->is_sick()) {
            const auto type = person->get_role();
            const auto location = _spaces->get_continuous_location(person->getId());
            _removed_staff.push_back(person->stats());

            _spaces->remove_agent(person);
            _context->removeAgent(person);

            auto* new_person = create_person(location, type);
            person = new_person;
        }
    }
}

/// @brief Save all the staff agents
/// @param folderpath The folder to save the results to
/// @param rank The process rank
void sti::staff_manager::save(const std::string& folderpath, int rank) const
{
    auto array = boost::json::array {_removed_staff};

    for (auto* person : _created) {
        array.push_back(person->stats());
        _spaces->remove_agent(person);
        _context->removeAgent(person);
    }

    auto os = std::ostringstream {};
    os << folderpath
       << "/staff.p"
       << rank
       << ".json";
    auto file = std::ofstream { os.str() };
    file << array;
}
