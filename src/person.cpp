#include "person.hpp"

boost::json::object sti::person_agent::kill_and_collect()
{
    auto output    = boost::json::object {};
    output["type"] = "person";

    const auto& stage_str = [&]() {
        const auto& stage = _infection_logic.get_stage();

        switch (stage) {
        case human_infection_cycle::STAGE::HEALTHY:
            return "healthy";
        case human_infection_cycle::STAGE::INCUBATING:
            return "incubating";
        case human_infection_cycle::STAGE::SICK:
            return "sick";
        }
    }();
    output["final_stage"] = stage_str;

    const auto& infect_time = _infection_logic.get_infection_time();
    if (infect_time) {
        output["infection_time"] = infect_time->str();
    }
    return output;
}