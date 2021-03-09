/// @file json_loader.hpp
/// @brief Small JSON file load
#pragma once

#include <boost/json.hpp>
#include <fstream>
#include <string>

namespace sti {
inline boost::json::value load_json(const std::string& path)
{

    auto       file    = std::ifstream { path };
    const auto content = std::string {
        std::istreambuf_iterator<char> { file },
        std::istreambuf_iterator<char> {},
    };

    return boost::json::parse(content);
}
}
