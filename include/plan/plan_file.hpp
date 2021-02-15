#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <istream>
#include <iterator>
#include <vector>

#include "plan.hpp"

namespace sti {

/// @brief information about the file format
struct file_format {
    using length_unit = std::uint32_t;
    using tile_unit   = std::uint8_t;

    constexpr auto static magic_numbers = std::array { 'P', 'L', 'A' };
    constexpr auto static versions      = std::array { 1U };
    constexpr auto static header_size   = 16U;
};

/// @brief Exception thrown when no file was loaded
struct no_plan_file : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: no plan file";
    }
};

/// @brief Exception thrown when a file with the wrong magic numbers is passed
struct plan_file_too_small : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: plan file too small";
    }
};

/// @brief Exception thrown when a file with the wrong magic numbers is passed
struct plan_file_wrong_magic_number : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Incorrect file magic number in plan file";
    }
};

/// @brief Exception thrown when a file with the wrong magic numbers is passed
struct plan_file_unsupported_version : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Unsupported plan version";
    }
};

/// @brief Exception thrown when a file with the wrong magic numbers is passed
struct plan_file_unknown_code : public std::exception {
    const char* what() const noexcept override
    {
        return "Exception: Unknown tile code in plan file";
    }
};

/// @brief Convert plan file tile code to the corresponding tile type
/// @param value The value to decode
/// @return The corresponding tile enum
plan_tile decode_tile(file_format::tile_unit value)
{
    using E = plan_tile::TILE_ENUM;

    if (value == 0) return plan_tile { E::FLOOR };
    if (value == 1) return plan_tile { E::WALL };

    if (value == 16) return plan_tile { E::CHAIR };

    if (value == 64) return plan_tile { E::ENTRY };
    if (value == 65) return plan_tile { E::EXIT };
    if (value == 66) return plan_tile { E::TRIAGE };
    if (value == 67) return plan_tile { E::ICU };

    if (value == 96) return plan_tile { E::RECEPTIONIST };

    if (128 <= value && value <= 255) return plan_tile { E::DOCTOR, static_cast<file_format::tile_unit>(value - 128) };

    throw plan_file_unknown_code {};
}

/// @brief Load a plan from a file
/// @param file_path The path to the file containing the plan description
/// @throws inconsistent_row If a line has a different length
/// @throws unknown_tile If the file contains an unknown token
/// @return a plan created from the file
plan load_plan(const std::string& file_path)
{

    auto file     = std::ifstream { file_path, std::ios::binary };
    auto raw_data = std::vector<std::uint8_t> {};

    if (!file.is_open()) {
        throw no_plan_file {};
    }

    std::for_each(
        std::istreambuf_iterator<char> { file },
        std::istreambuf_iterator<char> {},
        [&](const auto c) {
            raw_data.push_back(c);
        });

    file.close();

    // Check the header
    if (raw_data.size() < file_format::header_size) throw plan_file_too_small {};

    auto data_it = raw_data.begin();

    if (static_cast<char>(*(data_it++)) != file_format::magic_numbers[0]) throw plan_file_wrong_magic_number {};
    if (static_cast<char>(*(data_it++)) != file_format::magic_numbers[1]) throw plan_file_wrong_magic_number {};
    if (static_cast<char>(*(data_it++)) != file_format::magic_numbers[2]) throw plan_file_wrong_magic_number {};

    const auto        version = static_cast<std::uint8_t>(*(data_it++));
    const auto* const it      = std::find(file_format::versions.begin(), file_format::versions.end(), version);
    if (it == file_format::versions.end()) throw plan_file_unsupported_version {};

    // Get the number of columns
    const auto columns = [&]() -> file_format::length_unit {
        std::uint32_t tmp = 0;
        std::memcpy(&tmp, data_it.base(), 4);
        return tmp;
    }();
    data_it += 4;

    // Get the number of rows
    const auto rows = [&]() -> file_format::length_unit {
        std::uint32_t tmp = 0;
        std::memcpy(&tmp, data_it.base(), 4);
        return tmp;
    }();
    data_it += 4;

    // Reserved bytes
    data_it += 4;

    // The buffer size must be at least header_size(12) + rows * columns
    if (raw_data.size() != file_format::header_size + rows * columns) throw plan_file_too_small {};

    // The rest of the file is the plan itself
    auto return_plan = plan {};

    for (auto i = 0U; i < columns; i++) {

        auto new_column = plan::column_type {};

        for (auto j = 0U; j < rows; j++) {
            const auto tile_code = (*data_it);
            new_column.push_back(decode_tile(tile_code));
            data_it++;
        }

        return_plan.add_column(std::move(new_column));
    }

    return return_plan;
}

} // namespace sti