/// @file hospital.hpp
/// @brief Hospital abstraction, for accessing all hospital related stuff
#pragma once

#include <map>
#include <memory>
#include <vector>

#include "coordinates.hpp"
#include "reception.hpp"

// Fw. declarations
namespace repast {
class Properties;
}

namespace boost {
namespace mpi {
    class communicator;
}
namespace json {
    class value;
    class object;
}
}

namespace sti {

// Fw. declarations
class chair_manager;
class hospital_entry;
class hospital_exit;
class hospital_plan;

namespace tiles {

    enum class ENUMS {
        FLOOR,
        WALL,
        CHAIR,
        ENTRY,
        EXIT,
        TRIAGE,
        ICU,
        RECEPTIONIST,
        RECEPTION_PATIENT_CHAIR,
        DOCTOR,
        DOCTOR_PATIENT_CHAIR
    };

    using grid = std::vector<std::vector<ENUMS>>;

    /// @brief Wall tile
    struct wall {
        coordinates<int> location;

        /// @brief Load the walls from a hospital, store the enums in the map and return the list
        /// @param hospital The JSON containing the hospital information
        /// @param map The map to store the enum
        /// @return The list of walls
        static std::vector<wall> load(const boost::json::object& hospital, grid& map);
    };

    /// @brief Chair tile, storing the chair location
    struct chair {
        coordinates<int> location;

        /// @brief Load the chairs from a hospital, store the enums in the map and return the list
        /// @param hospital The JSON containing the hospital information
        /// @param map The map to store the enum
        /// @return The list of chairs
        static std::vector<chair> load(const boost::json::object& hospital, grid& map);
    };

    /// @brief Entry tile, storing the entry location
    struct entry {
        coordinates<int> location;

        /// @brief Load the entry from a hospital, store the enums in the map and return the list
        /// @param hospital The JSON containing the hospital information
        /// @param map The map to store the enum
        /// @return The entry
        static entry load(const boost::json::object& hospital, grid& map);
    };

    /// @brief Exit tile, storing the exit location
    struct exit {
        coordinates<int> location;

        /// @brief Load the exit from a hospital, store the enums in the map and return the list
        /// @param hospital The JSON containing the hospital information
        /// @param map The map to store the enum
        /// @return exit
        static exit load(const boost::json::object& hospital, grid& map);
    };

    /// @brief Triage tile, storing the triage location
    struct triage {
        coordinates<int> location;

        /// @brief Load the triage from a hospital, store the enums in the map and return the list
        /// @param hospital The JSON containing the hospital information
        /// @param map The map to store the enum
        /// @return The list of triages
        static std::vector<triage> load(const boost::json::object& hospital, grid& map);
    };

    /// @brief ICU tile, storing the icu location
    struct icu {
        coordinates<int> location;

        /// @brief Load the ICU from a hospital, store the enums in the map and return the list
        /// @param hospital The JSON containing the hospital information
        /// @param map The map to store the enum
        /// @return The ICU location
        static icu load(const boost::json::object& hospital, grid& map);
    };

    /// @brief Receptionist tile, storing the receptionist and patient location
    struct receptionist {
        coordinates<int> location;
        coordinates<int> patient_chair;

        /// @brief Load the receptionists from a hospital, store the enums in the map and return the list
        /// @param hospital The JSON containing the hospital information
        /// @param map The map to store the enum
        /// @return The list of receptionists
        static std::vector<receptionist> load(const boost::json::object& hospital, grid& map);
    };

    /// @brief Doctor tile, storing the location, type and patient location
    struct doctor {
        coordinates<int> location;
        coordinates<int> patient_chair;
        std::string      type;

        /// @brief Load the doctors from a hospital, store the enums in the map and return the list
        /// @param hospital The JSON containing the hospital information
        /// @param map The map to store the enum
        /// @return The list of doctors
        static std::vector<doctor> load(const boost::json::object& hospital, grid& map);
    };

} // namespace tiles

/// @brief Hospital abstaction, provides access to all hospital-related functions
class hospital_plan {
public:
    using length_t = sti::coordinates<int>::length_t;

    ////////////////////////////////////////////////////////////////////////////
    // CONSTRUCTION
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Load a hospital from a JSON object
    /// @param json The JSON containing the hospital description
    hospital_plan(const boost::json::object& json);

    ////////////////////////////////////////////////////////////////////////////
    // HOSPITAL PLAN
    ////////////////////////////////////////////////////////////////////////////

    /// @brief Get the width of the hospital
    auto width() const
    {
        return _width;
    }

    /// @brief Get the height of the hospital
    auto height() const
    {
        return _height;
    }

    /// @brief Access the location
    /// @param l The location
    /// @return A reference to the tile
    tiles::ENUMS& operator[](const coordinates<int>& l);

    /// @brief Access the location
    /// @param l The location
    /// @return A reference to the tile
    const tiles::ENUMS& operator[](const coordinates<int>& l) const;

    /// @brief Get all the walls
    const std::vector<tiles::wall>& walls() const;

    /// @brief Get all the chairs
    const std::vector<tiles::chair>& chairs() const;

    /// @brief Get the entry
    tiles::entry entry() const;

    /// @brief Get the exit
    tiles::exit exit() const;

    /// @brief Get triage
    const std::vector<tiles::triage>& triages() const;

    /// @brief Get the icu
    tiles::icu icu() const;

    /// @brief Get all the receptionists
    const std::vector<tiles::receptionist>& receptionists() const;

    /// @brief Get all the doctors
    const std::vector<tiles::doctor>& doctors() const;

private:
    length_t _width;
    length_t _height;

    tiles::grid _grid;

    std::vector<tiles::wall>         _walls;
    std::vector<tiles::chair>        _chairs;
    tiles::entry                     _entry;
    tiles::exit                      _exit;
    std::vector<tiles::triage>       _triages;
    tiles::icu                       _icu;
    std::vector<tiles::receptionist> _receptionists;
    std::vector<tiles::doctor>       _doctors;
};

} // namespace sti