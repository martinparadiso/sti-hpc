/// @brief Hospital entry point
#pragma once

#include <algorithm>
#include <boost/histogram.hpp>
#include <boost/histogram/axis/integer.hpp>
#include <boost/histogram/histogram.hpp>
#include <boost/histogram/make_histogram.hpp>
#include <boost/histogram/weight.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <repast_hpc/Properties.h>
#include <sstream>
#include <string_view>
#include <string>
#include <tuple>
#include <vector>

#include "clock.hpp"
#include "hospital_plan.hpp"
#include "metric.hpp"

// Fw. declarations
namespace boost {
namespace json {
    class array;
    class object;
}
}

namespace sti {

// Fw. declarations
class agent_factory;

struct inconsistent_distribution : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Wrong number of bins for a given day in the patient distribution";
    }
};

struct negative_patients : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Negative number of patients entering the hospital in file";
    }
};

struct inconsistent_bins_in_file : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Wrong number of bins for a given day in the patient distribution file";
    }
};

/// @brief Distribution of patients entering the hospital
/// @details Distribution/rate of patient entering the hospital in a given
///          length of time. The distribution is discrete divided in days, which
///          are also divided in N bins. The number of bins is specified by the
///          user but has to be equal for all days
class patient_distribution {

public:
    using axis_t    = boost::histogram::axis::integer<std::uint32_t, boost::histogram::use_default, boost::histogram::axis::option::none_t>;
    using axes_t    = std::tuple<axis_t, axis_t>;
    using hist_t    = boost::histogram::histogram<axes_t>;
    using data_type = std::vector<std::vector<std::uint32_t>>;

    patient_distribution(data_type&& data)
        : _data { boost::histogram::make_histogram(
            axis_t(0, static_cast<std::uint32_t>(data.size()), "day"),
            axis_t(0, static_cast<std::uint32_t>(data.begin()->size()), "interval")) }
    {
        // Fill with the data
        for (auto i = 0U; i < data.size(); i++) {
            for (auto j = 0U; j < data[i].size(); j++) {
                _data(i, j, boost::histogram::weight(data[i][j]));
            }
        }
    }

    /// @brief Get the number of days the distribution cover
    /// @return The number of days for which data is available
    auto days() const
    {
        return static_cast<std::uint32_t>(_data.axis(0).size());
    }

    /// @brief Get the number of bins/intervals in a day
    /// @return The number of intervals in a day
    auto intervals() const
    {
        return static_cast<std::uint32_t>(_data.axis(1).size());
    }

    /// @brief Get the number of patients entering the hospital in a given interval
    /// @param day The day
    /// @param interval The interval of the day
    /// @return The number of patients entering the hospital in the specified interval
    auto get(std::uint32_t day, std::uint32_t interval) const
    {
        return static_cast<std::uint64_t>(_data.at(static_cast<int>(day), interval));
    }

    /// @brief Increment in 1 the number of patitents
    template <typename... Args>
    void increment(Args... args)
    {
        _data(args...);
    }

    hist_t _data;

private:
};

/// @brief Hospital entry point, periodically generates
class hospital_entry {

public:
    /// @brief Create a hospital entry
    /// @details The hospital entry is in charge of creating the patients
    /// @param location The location of the entry
    /// @param clock The simulation clock
    /// @param patient_admissions The patient admission histogram
    /// @param factory The agent factory, for patient creation
    /// @param props Repast properties
    hospital_entry(coordinates                           location,
                   sti::clock*                           clock,
                   std::unique_ptr<patient_distribution> patient_admissions,
                   agent_factory*                        factory,
                   const boost::json::object&            props);

    /// @brief Generate the pending patients
    void generate_patients();

    /// @brief Generate a JSON object containing the entry statistics
    /// @return The Boost.JSON object with the statistics
    boost::json::array statistics() const;

private:
    coordinates                               _location;
    const sti::clock*                         _clock;
    std::unique_ptr<patient_distribution>     _patient_distribution;
    sti::metric<patient_distribution::hist_t> _generated_patients;
    const std::uint32_t                       _interval_length;

    sti::agent_factory* _agent_factory;
    double              _infected_chance;

    /// @brief Ask how many patients are waiting at the door, upon call the counter is cleared
    /// @details Calculate how many patients must be created according to the
    ///          provided distribution. The function assumes the caller is going to create
    ///          those agents, the internal counter of created agents is increased in N.
    /// @return The number of patients waiting admission
    std::uint64_t patients_waiting();
};

/// @brief Load the patient distribution curve from a file
/// @param json The json object containing the influx
/// @details File format is described in the documentation
std::unique_ptr<patient_distribution> load_patient_distribution(const boost::json::object& json);

} // namespace sti