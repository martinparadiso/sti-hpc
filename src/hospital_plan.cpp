/// @file hospital.cpp
/// @brief Hospital definitions
#include "hospital_plan.hpp"

#include "json_serialization.hpp"

#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <boost/json/src.hpp>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
// HOSPITAL TILES
////////////////////////////////////////////////////////////////////////////////

// Helper functions
namespace {

// sti::coordinates<int> get_location(const boost::json::value& json)
// {
//     return {
//         boost::json::value_to<int>(json.at("x")),
//         boost::json::value_to<int>(json.at("y"))
//     };
// }

/// @brief Generic load for simple types that have only location
template <typename T, sti::tiles::ENUMS e>
std::vector<T> load_several(const boost::json::value& json,
                            const std::string&        key,
                            sti::tiles::grid&         grid)
{
    auto vector = std::vector<T> {};
    for (const auto& element : boost::json::value_to<std::vector<boost::json::value>>(json.at("building").at(key))) {
        const auto loc = boost::json::value_to<sti::coordinates<int>>(element);

        grid.at(static_cast<std::size_t>(loc.x)).at(static_cast<std::size_t>(loc.y)) = e;
        vector.push_back(T { loc });
    }
    return vector;
}

/// @brief Generic load for simple types that have only location
template <typename T, sti::tiles::ENUMS e>
T load_one(const boost::json::value& json,
           const std::string&        key,
           sti::tiles::grid&         grid)
{
    const auto loc = boost::json::value_to<sti::coordinates<int>>((json.at("building").at(key)));

    grid.at(static_cast<std::size_t>(loc.x)).at(static_cast<std::size_t>(loc.y)) = e;
    return T { loc };
}

} // namespace

std::vector<sti::tiles::wall> sti::tiles::wall::load(const boost::json::object& hospital, sti::tiles::grid& map)
{
    return load_several<wall, ENUMS::WALL>(hospital, "walls", map);
}

std::vector<sti::tiles::chair> sti::tiles::chair::load(const boost::json::object& hospital, sti::tiles::grid& map)
{
    return load_several<chair, ENUMS::CHAIR>(hospital, "chairs", map);
}

sti::tiles::entry sti::tiles::entry::load(const boost::json::object& hospital, sti::tiles::grid& map)
{
    return load_one<entry, ENUMS::ENTRY>(hospital, "entry", map);
}

sti::tiles::exit sti::tiles::exit::load(const boost::json::object& hospital, sti::tiles::grid& map)
{
    return load_one<exit, ENUMS::EXIT>(hospital, "exit", map);
}

std::vector<sti::tiles::triage> sti::tiles::triage::load(const boost::json::object& hospital, sti::tiles::grid& map)
{
    auto vector = std::vector<triage> {};

    for (const auto& element : boost::json::value_to<std::vector<boost::json::value>>(hospital.at("building").at("triages"))) {
        const auto location = boost::json::value_to<sti::coordinates<int>>(element.at("location"));
        map.at(static_cast<std::size_t>(location.x))
            .at(static_cast<std::size_t>(location.y))
            = ENUMS::TRIAGE;
        vector.push_back(triage { location });
    }
    return vector;
}

sti::tiles::icu sti::tiles::icu::load(const boost::json::object& hospital, sti::tiles::grid& map)
{
    const auto entry = boost::json::value_to<sti::coordinates<int>>((hospital.at("building").at("icu_entry")));
    const auto exit = boost::json::value_to<sti::coordinates<int>>((hospital.at("building").at("icu_entry")));

    map.at(static_cast<std::size_t>(entry.x)).at(static_cast<std::size_t>(entry.y));
    return icu{entry, exit};
}

std::vector<sti::tiles::receptionist> sti::tiles::receptionist::load(const boost::json::object& hospital, sti::tiles::grid& map)
{

    auto vector = std::vector<receptionist> {};

    for (const auto& element : boost::json::value_to<std::vector<boost::json::value>>(hospital.at("building").at("receptionists"))) {
        const auto location      = boost::json::value_to<sti::coordinates<int>>(element.at("location"));
        const auto patient_chair = boost::json::value_to<sti::coordinates<int>>(element.at("patient_chair"));
        map.at(static_cast<std::size_t>(location.x))
            .at(static_cast<std::size_t>(location.y))
            = ENUMS::RECEPTIONIST;
        map.at(static_cast<std::size_t>(location.x))
            .at(static_cast<std::size_t>(location.y))
            = ENUMS::RECEPTION_PATIENT_CHAIR;
        vector.push_back(receptionist { location, patient_chair });
    }
    return vector;
}

std::vector<sti::tiles::doctor> sti::tiles::doctor::load(const boost::json::object& hospital, sti::tiles::grid& map)
{

    auto vector = std::vector<doctor> {};

    for (const auto& element : boost::json::value_to<std::vector<boost::json::value>>(hospital.at("building").at("doctors"))) {
        const auto location      = boost::json::value_to<sti::coordinates<int>>(element);
        const auto patient_chair = boost::json::value_to<sti::coordinates<int>>(element.at("patient_chair"));
        const auto type          = boost::json::value_to<std::string>(element.at("type"));
        map.at(static_cast<std::size_t>(location.x))
            .at(static_cast<std::size_t>(location.y))
            = ENUMS::DOCTOR;
        map.at(static_cast<std::size_t>(location.x))
            .at(static_cast<std::size_t>(location.y))
            = ENUMS::DOCTOR_PATIENT_CHAIR;
        vector.push_back(doctor { location, patient_chair, type });
    }
    return vector;
}

////////////////////////////////////////////////////////////////////////////////
// HOSPITAL // CONSTRUCTION
////////////////////////////////////////////////////////////////////////////////

/// @brief Load a hospital from a JSON object
/// @param json The JSON containing the hospital description
sti::hospital_plan::hospital_plan(const boost::json::object& json)
    : _width { boost::json::value_to<int>(json.at("building").at("width")) }
    , _height { boost::json::value_to<int>(json.at("building").at("height")) }
    , _grid { std::vector { static_cast<std::size_t>(_width),
                            std::vector {
                                static_cast<std::size_t>(_height),
                                tiles::ENUMS::FLOOR } } }
    , _walls { tiles::wall::load(json, _grid) }
    , _chairs { tiles::chair::load(json, _grid) }
    , _entry { tiles::entry::load(json, _grid) }
    , _exit { tiles::exit::load(json, _grid) }
    , _triages { tiles::triage::load(json, _grid) }
    , _icu { tiles::icu::load(json, _grid) }
    , _receptionists { tiles::receptionist::load(json, _grid) }
    , _doctors { tiles::doctor::load(json, _grid) }
{
}

////////////////////////////////////////////////////////////////////////////
// HOSPITAL // PLAN
////////////////////////////////////////////////////////////////////////////

/// @brief Access the location
/// @param l The location
/// @return A reference to the tile
sti::tiles::ENUMS& sti::hospital_plan::operator[](const coordinates<int>& l)
{
    const auto x = static_cast<decltype(_grid)::size_type>(l.x);
    const auto y = static_cast<decltype(_grid)::size_type>(l.y);
    return _grid.at(x).at(y);
}

/// @brief Access the location
/// @param l The location
/// @return A reference to the tile
const sti::tiles::ENUMS& sti::hospital_plan::operator[](const coordinates<int>& l) const
{
    const auto x = static_cast<decltype(_grid)::size_type>(l.x);
    const auto y = static_cast<decltype(_grid)::size_type>(l.y);
    return _grid.at(x).at(y);
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
