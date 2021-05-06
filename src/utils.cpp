#include "utils.hpp"

namespace {

/// @brief Exception thrown when a probability is wrong
struct wrong_probability : public std::exception {

    std::string msg;

    wrong_probability(std::string_view hint, double value)
    {
        auto os = std::ostringstream {};
        os << "Exception: Wrong probability, outside range [0, 1]." << value
           << "\n Hint: " << hint << '\n';
        msg = os.str();
    };

    const char* what() const noexcept override
    {
        return msg.c_str();
    }
};
}

/// @brief Validate a given probability, i.e. it falls in the range [0, 1)
/// @details This function does not return a value, in case of failure, it
/// throws an exception
/// @throws wrong_probability If one of the checks failed
/// @param value The probability to validate
/// @param description A hint for the exception
void sti::validate_probability(probability_precission value, std::string_view description)
{
    if (! (0.0 <= value && value <= 1.0)) {
        throw wrong_probability{description, value};
    }
}