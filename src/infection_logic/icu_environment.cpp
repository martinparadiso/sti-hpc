#include "icu_environment.hpp"

#include <boost/json/object.hpp>

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct an ICU infection environment
/// @param hospital_params JSON parameters of the hospital
/// @param name The name of the environment, by default is 'icu'
sti::icu_environment::icu_environment(const boost::json::object& hospital_params, const std::string& name )
    : _name { name }
    , _current_patients { 0 }
    , _icu_infection_chance { [&](){
        return hospital_params.at("paremeters").at("icu").at("environment").at("infection_chance").as_double();
    }() }
{
}

////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Get a reference to the current number of patients
/// @return A reference to the current number of patients in the ICU
std::uint32_t& sti::icu_environment::patients()
{
    return _current_patients;
}

/// @brief Get the probability of infecting
/// @return A value in the range [0, 1), depending of the number of patients
[[nodiscard]] sti::infection_cycle::precission sti::icu_environment::get_probability() const
{
    return static_cast<decltype(_icu_infection_chance)>(_current_patients) * _icu_infection_chance;
}

/// @brief Get the name of the environemnt, for statistic reasons
/// @details Every environment must have an unique name, to be able to
/// determine how an agent got infected
std::string sti::icu_environment::name() const
{
    return _name;
}
