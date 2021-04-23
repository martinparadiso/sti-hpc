/// @file utils.hpp
/// @brief Collection of utils and compile time configurations
#pragma once

#include <functional>
#include <numeric>
#include <string_view>
#include <string>
#include <sstream>
#include <utility>

namespace sti {

using probability_precission         = double;
constexpr auto accumulator_tolerance = std::pair { 0.99, 1.01 };

/// @brief Exception thrown when the distribution is wrong
struct wrong_distribution : public std::exception {

    std::string msg;

    wrong_distribution(std::string_view hint, double value)
    {
        auto os = std::ostringstream {};
        os << "Exception: Wrong distribution, sums" << value
           << "\n Hint: " << hint << '\n';
        msg = os.str();
    };

    const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

/// @brief Validate a given probability, i.e. it falls in the range [0, 1)
/// @details This function does not return a value, in case of failure, it
/// throws an exception
/// @throws wrong_probability If one of the checks failed
/// @param value The probability to validate
/// @param description A hint for the exception
void validate_probability(probability_precission value, std::string_view description);

/// @brief Validate a probability distribution, i.e. it sums 1
/// @details This function does not return a value, in case of failure, it
/// throws an exception
/// @throws wrong_distribution If one of the checks failed
/// @param begin_it An iterator pointing to the beginning of the distribution
/// @param end_it An iterator pointing one past-the-end of the distribution
/// @param sum_function The function used to accumulate the probabilities. The
/// signature should be equivalent to that of std::accumulate
/// @param hint Information to identify the offending distribution
/// @param base_value The start value of the accumulator
template <typename InputIt, typename BinaryOperation>
void validate_distribution(InputIt                begin_it,
                           InputIt                end_it,
                           BinaryOperation        sum_function,
                           std::string_view       hint,
                           probability_precission base_value = 0.0)
{
    auto acc = std::accumulate(begin_it, end_it, base_value, sum_function);
    if (acc < accumulator_tolerance.first || acc > accumulator_tolerance.second) {
        throw wrong_distribution { hint, acc };
    }
}
} // namespace sti