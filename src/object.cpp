#include "object.hpp"

// TODO: Implement properly
boost::json::object sti::object_agent::kill_and_collect()
{
    auto output = boost::json::object {};
    output["type"] = "object";

    const auto& stage_str = [&]() {
        const auto& stage = _infection_logic.get_stage();

        switch (stage) {
        case object_infection_cycle::STAGE::CLEAN:
            return "clean";
        case object_infection_cycle::STAGE::INFECTED:
            return "infected";
        }
    }();

    output["stage"] = stage_str;

    return output;
}