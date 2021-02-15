/// @brief Agent capable of infecting others
#pragma once

#include <cstdint>
#include <vector>

#include <boost/variant.hpp>

#include "clock.hpp"

namespace sti {

/// @brief An virtual class representing an agent capable of infecting others
class contagious_agent {

public:

    contagious_agent() = default;
    contagious_agent(const contagious_agent&) = default;
    contagious_agent(contagious_agent&&) = default;
    contagious_agent& operator=(const contagious_agent&) = default;
    contagious_agent& operator=(contagious_agent&&) = default;
    virtual ~contagious_agent() = default;

    /// @brief The different type of contagious agents
    enum class type {
        CHAIR,
        FIXED_PERSON,
        PATIENT,
    };

    using p_precission = float;
    using variant      = boost::variant<float, double, std::int32_t, std::uint32_t, std::uint64_t, std::int64_t>;
    using serial_data  = std::vector<variant>;

    /// @brief Get the type of this agent
    /// @return The type of the agent
    virtual type get_type() const = 0;

    /// @brief Serialize the agent data into a vector
    /// @returns A vector with the agent data required for reconstruction
    virtual serial_data serialize() const = 0;

    /// @brief Update the agent state
    /// @details Update the agent state with new data
    /// @param data The new data for the agent
    virtual void update(const serial_data& data) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// PATIENT
////////////////////////////////////////////////////////////////////////////////

/// @brief An agent representing a patient
class patient_agent : public contagious_agent {

public:
    /// @brief A patient cannot be default constructed, probabilities are needed
    patient_agent()
        : _infecting_chance {}
        , _entry_time {}
    {
    }

    /// @brief Create a new patient
    /// @param infecting_chance The change of infecting other patients
    /// @param entry_time The instant the patient enter the building
    patient_agent(p_precission infecting_chance, const clock::date_t& entry_time)
        : _infecting_chance { infecting_chance }
        , _entry_time { entry_time }
    {
    }

    /// @brief Create a new patient from serialized data
    /// @param data The serialized data
    patient_agent(const serial_data& data)
    : _infecting_chance{ boost::get<p_precission>(data.at(0)) }
    , _entry_time{ boost::get<clock::resolution>(data.at(1)) }
    {

    }

    /// @brief Get the type of the agent
    /// @returns Patient
    type get_type() const override
    {
        return contagious_agent::type::PATIENT;
    }


    /// @brief Serialize the internal state of the agent
    /// @return The serialized data
    serial_data serialize() const override {
        return { _infecting_chance, _entry_time.epoch() };
    }

    /// @brief Update this agent with new data
    /// @param data The serial data used to update this agent
    void update(const serial_data& data) override {
        *this = patient_agent(data);
    }

    /// @brief Get infect chance
    /// @return The infect chance
    auto infect_chance() const {
        return _infecting_chance;
    }

    /// @brief Get the time the patient was admitted at the hospital
    /// @return The date (with a precission of seconds) the patient was admitted
    auto entry_time() const {
        return _entry_time;
    }

private:
    contagious_agent::p_precission _infecting_chance;
    clock::date_t                  _entry_time;

    // TODO: missing data?
};

////////////////////////////////////////////////////////////////////////////////
// INT <-> ENUM
////////////////////////////////////////////////////////////////////////////////

/// @brief Error trying to convert from int to enum
struct unknown_agent_type : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: The int value does not correspond to any known agent";
    }
};

/// @brief Convert an agent enum to int
/// @return The int corresponding
constexpr int to_int(contagious_agent::type type)
{

    using E = contagious_agent::type;

    switch (type) { // clang-format off
        case E::CHAIR:          return 0;
        case E::FIXED_PERSON:   return 1;
        case E::PATIENT:        return 2;
    } // clang-format on
}

/// @brief Convert an int to a agent enum
constexpr contagious_agent::type to_agent_enum(int i)
{
    using E = contagious_agent::type;

    if (i == 0) return E::CHAIR;
    if (i == 1) return E::FIXED_PERSON;
    if (i == 2) return E::PATIENT;

    throw unknown_agent_type {};
}

} // namespace sti