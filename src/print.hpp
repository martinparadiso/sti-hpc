#pragma once

#include <iostream>
#include <string>

#include <repast_hpc/RepastProcess.h>

namespace sti {

/// @brief Print the given string to the terminal. Warning: no sync between process
inline void print(const std::string& str)
{
    // TODO: Disable for now
    // const auto rank = repast::RepastProcess::instance()->rank();
    // const auto tick = static_cast<long>(repast::RepastProcess::instance()->getScheduleRunner().currentTick());
    // std::cout << "[" << rank << "] "
    //           << "[" << tick << "] "
    //           << str << std::endl;
}

} // namespace sti