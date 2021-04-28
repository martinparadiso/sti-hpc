/// @file hospital.cpp
/// @brief Hospital definitions
#include "hospital_plan.hpp"

#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <boost/json/src.hpp>
#include <string>
#include <vector>

#include "json_serialization.hpp"
#include "pathfinder.hpp"
#include "clock.hpp"

////////////////////////////////////////////////////////////////////////////////
// HOSPITAL TILES
////////////////////////////////////////////////////////////////////////////////

// Helper functions
namespace {

/// @brief Generic load for simple types that have only location
/// @param hospital_params The JSON containing the building information
/// @param key The key with which the object is stored
/// @param obstacles The obstacles map
/// @param is_walkable True if a patient can walk over it, false otherwise
template <typename T, sti::tiles::ENUMS e>
std::vector<T> load_several(const boost::json::value&  hospital_params,
                            const std::string&         key,
                            sti::tiles::obstacles_map& obstacles,
                            bool                       is_walkable)
{
    auto vector = std::vector<T> {};
    for (const auto& element : boost::json::value_to<std::vector<boost::json::value>>(hospital_params.at("building").at(key))) {
        const auto loc = boost::json::value_to<sti::coordinates<int>>(element);

        obstacles.at(static_cast<std::size_t>(loc.x)).at(static_cast<std::size_t>(loc.y)) = is_walkable;
        vector.push_back(T { loc });
    }
    return vector;
}

/// @brief Generic load for simple types that have only location
/// @param hospital_params The JSON containing the building information
/// @param key The key with which the object is stored
/// @param obstacles The obstacles map
/// @param is_walkable True if a patient can walk over it, false otherwise
template <typename T, sti::tiles::ENUMS e>
T load_one(const boost::json::value&  hospital_params,
           const std::string&         key,
           sti::tiles::obstacles_map& obstacles,
           bool                       is_walkable)
{
    const auto loc = boost::json::value_to<sti::coordinates<int>>((hospital_params.at("building").at(key)));

    obstacles.at(static_cast<std::size_t>(loc.x)).at(static_cast<std::size_t>(loc.y)) = is_walkable;
    return T { loc };
}

} // namespace

std::vector<sti::tiles::wall> sti::tiles::wall::load(const boost::json::object& hospital, sti::tiles::obstacles_map& map)
{
    return load_several<wall, ENUMS::WALL>(hospital, "walls", map, false);
}

std::vector<sti::tiles::chair> sti::tiles::chair::load(const boost::json::object& hospital, sti::tiles::obstacles_map& map)
{
    return load_several<chair, ENUMS::CHAIR>(hospital, "chairs", map, false);
}

sti::tiles::entry sti::tiles::entry::load(const boost::json::object& hospital, sti::tiles::obstacles_map& map)
{
    return load_one<entry, ENUMS::ENTRY>(hospital, "entry", map, false);
}

sti::tiles::exit sti::tiles::exit::load(const boost::json::object& hospital, sti::tiles::obstacles_map& map)
{
    // Note: the exit periodically queries the space to retrieve all agents
    // standing above it, so, if an agent walks over it, it will be removed
    return load_one<exit, ENUMS::EXIT>(hospital, "exit", map, false);
}

std::vector<sti::tiles::triage> sti::tiles::triage::load(const boost::json::object& hospital, sti::tiles::obstacles_map& map)
{
    auto vector = std::vector<triage> {};

    for (const auto& element : boost::json::value_to<std::vector<boost::json::value>>(hospital.at("building").at("triages"))) {
        const auto location = boost::json::value_to<sti::coordinates<int>>(element.at("patient_location"));
        map.at(static_cast<std::size_t>(location.x))
            .at(static_cast<std::size_t>(location.y))
            = true;
        vector.push_back(triage { location });
    }
    return vector;
}

sti::tiles::icu sti::tiles::icu::load(const boost::json::object& hospital, sti::tiles::obstacles_map& map)
{
    return load_one<icu, ENUMS::ICU>(hospital, "icu", map, true);
}

std::vector<sti::tiles::receptionist> sti::tiles::receptionist::load(const boost::json::object& hospital, sti::tiles::obstacles_map& map)
{

    auto vector = std::vector<receptionist> {};

    for (const auto& element : boost::json::value_to<std::vector<boost::json::value>>(hospital.at("building").at("receptionists"))) {
        const auto location      = boost::json::value_to<sti::coordinates<int>>(element.at("receptionist_location"));
        const auto patient_chair = boost::json::value_to<sti::coordinates<int>>(element.at("patient_location"));
        map.at(static_cast<std::size_t>(location.x))
            .at(static_cast<std::size_t>(location.y))
            = true;
        map.at(static_cast<std::size_t>(location.x))
            .at(static_cast<std::size_t>(location.y))
            = true;
        vector.push_back(receptionist { location, patient_chair });
    }
    return vector;
}

std::vector<sti::tiles::doctor> sti::tiles::doctor::load(const boost::json::object& hospital, sti::tiles::obstacles_map& map)
{

    auto vector = std::vector<doctor> {};

    for (const auto& element : boost::json::value_to<std::vector<boost::json::value>>(hospital.at("building").at("doctors"))) {
        const auto location      = boost::json::value_to<sti::coordinates<int>>(element.at("doctor_location"));
        const auto patient_chair = boost::json::value_to<sti::coordinates<int>>(element.at("patient_location"));
        const auto type          = boost::json::value_to<std::string>(element.at("specialty"));
        map.at(static_cast<std::size_t>(location.x))
            .at(static_cast<std::size_t>(location.y))
            = true;
        map.at(static_cast<std::size_t>(location.x))
            .at(static_cast<std::size_t>(location.y))
            = true;
        vector.push_back(doctor { location, patient_chair, type });
    }
    return vector;
}

////////////////////////////////////////////////////////////////////////////////
// HOSPITAL // CONSTRUCTION
////////////////////////////////////////////////////////////////////////////////

/// @brief Load a hospital from a JSON object
/// @param json The JSON containing the hospital description
/// @param clock The simulation clock
sti::hospital_plan::hospital_plan(const boost::json::object& json,
                                  const clock*               clock)
    : _width { boost::json::value_to<int>(json.at("building").at("width")) }
    , _height { boost::json::value_to<int>(json.at("building").at("height")) }
    , _obstacles(static_cast<std::size_t>(_width), std::vector(static_cast<std::size_t>(_height), true))
    , _walls { tiles::wall::load(json, _obstacles) }
    , _chairs { tiles::chair::load(json, _obstacles) }
    , _entry { tiles::entry::load(json, _obstacles) }
    , _exit { tiles::exit::load(json, _obstacles) }
    , _triages { tiles::triage::load(json, _obstacles) }
    , _icu { tiles::icu::load(json, _obstacles) }
    , _receptionists { tiles::receptionist::load(json, _obstacles) }
    , _doctors { tiles::doctor::load(json, _obstacles) }
    , _pathfinder { &_obstacles, clock }
{
}

sti::hospital_plan::~hospital_plan() = default;

////////////////////////////////////////////////////////////////////////////
// HOSPITAL // PLAN
////////////////////////////////////////////////////////////////////////////

/// @brief A const reference to the obstacles
/// @return A vector of vectors of bools, True if the coordinate is walkable
const sti::tiles::obstacles_map& sti::hospital_plan::obstacles() const
{
    return _obstacles;
}

/// @brief Get the pathfinder
/// @return A pointer to the pathfinder
sti::pathfinder* sti::hospital_plan::get_pathfinder()
{
    return &_pathfinder;
}

/// @brief Get all the walls
const std::vector<sti::tiles::wall>& sti::hospital_plan::walls() const
{
    return _walls;
}

/// @brief Get all the chairs
const std::vector<sti::tiles::chair>& sti::hospital_plan::chairs() const
{
    return _chairs;
}

/// @brief Get the entry
sti::tiles::entry sti::hospital_plan::entry() const
{
    return _entry;
}

/// @brief Get the exit
sti::tiles::exit sti::hospital_plan::exit() const
{
    return _exit;
}

/// @brief Get triage
const std::vector<sti::tiles::triage>& sti::hospital_plan::triages() const
{
    return _triages;
}

/// @brief Get the icu
sti::tiles::icu sti::hospital_plan::icu() const
{
    return _icu;
}

/// @brief Get all the receptionists
const std::vector<sti::tiles::receptionist>& sti::hospital_plan::receptionists() const
{
    return _receptionists;
}

/// @brief Get all the doctors
const std::vector<sti::tiles::doctor>& sti::hospital_plan::doctors() const
{
    return _doctors;
}
