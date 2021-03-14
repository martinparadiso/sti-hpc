#include "patient.hpp"

////////////////////////////////////////////////////////////////////////////
// PIMPL
////////////////////////////////////////////////////////////////////////////

class sti::patient_agent::pimpl {
};

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Create a new patient
/// @param id The agent id
/// @param fw The flyweight containing the shared properties
/// @param entry_time The instant the patient enter the building
sti::patient_agent::patient_agent(const id_t&                  id,
                                  flyweight_ptr                fw,
                                  const datetime&              entry_time,
                                  const human_infection_cycle& hic)
    : contagious_agent { id }
    , _flyweight { fw }
    , _entry_time { entry_time }
    , _infection_logic { hic }
{
}

/// @brief Create a new patient from serialized data
/// @param id The agent id
/// @param fw The agent flyweight
/// @param data The serialized data
sti::patient_agent::patient_agent(const id_t&   id,
                                  flyweight_ptr fw,
                                  serial_data&  queue)
    : contagious_agent { id }
    , _flyweight { fw }
    , _entry_time {}
    , _infection_logic { fw->inf_factory->make_human_cycle() }
{
    deserialize(queue);
}

sti::patient_agent::~patient_agent() = default;

////////////////////////////////////////////////////////////////////////////
// SERIALIZATION
////////////////////////////////////////////////////////////////////////////

/// @brief Serialize the internal state of the infection
/// @param queue The queue to store the data
void sti::patient_agent::serialize(serial_data& queue) const
{
    queue.push(_entry_time.epoch());

    { // tmp
        switch (_stage) {
        case STAGES::START:
            queue.push(0);
            break;
        case STAGES::WAITING_CHAIR:
            queue.push(1);
            break;
        case STAGES::WALKING_TO_CHAIR:
            queue.push(2);
            break;
        case STAGES::SIT_DOWN:
            queue.push(3);
            break;
        case STAGES::WALKING_TO_EXIT:
            queue.push(4);
            break;
        case STAGES::DULL:
            queue.push(5);
            break;
        }
        queue.push(_chair_assigned.x);
        queue.push(_chair_assigned.y);
        queue.push(_chair_release_time.epoch());
    } // tmp
    _infection_logic.serialize(queue);
}

/// @brief Deserialize the data and update the agent data
/// @param queue The queue containing the data
void sti::patient_agent::deserialize(serial_data& queue)
{
    _entry_time = datetime { boost::get<datetime::resolution>(queue.front()) };
    queue.pop();
    { // tmp
        const auto aux = std::array {
            STAGES::START,
            STAGES::WAITING_CHAIR,
            STAGES::WALKING_TO_CHAIR,
            STAGES::SIT_DOWN,
            STAGES::WALKING_TO_EXIT,
            STAGES::DULL
        };
        const auto stage_i = boost::get<int>(queue.front());
        queue.pop();
        _stage = aux.at(static_cast<std::uint64_t>(stage_i));

        const auto x = boost::get<std::int32_t>(queue.front());
        queue.pop();
        const auto y = boost::get<std::int32_t>(queue.front());
        queue.pop();
        _chair_assigned = coordinates { x, y };

        _chair_release_time = datetime { boost::get<datetime::resolution>(queue.front()) };
        queue.pop();
    } // tmp
    _infection_logic.deserialize(queue);
}

////////////////////////////////////////////////////////////////////////////
// BAHAVIOUR
////////////////////////////////////////////////////////////////////////////

/// @brief Get the type of the agent
/// @returns Patient
sti::patient_agent::type sti::patient_agent::get_type() const
{
    return contagious_agent::type::PATIENT;
}

// TODO: Implement properly
boost::json::object sti::patient_agent::kill_and_collect()
{
    auto output = boost::json::object {};
    output["type"] = "patient";
    output["entry_time"] = _entry_time.str();

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

    if (_chair_assigned == coordinates { -1, -1 }) {
        output["chair_assigned"] = "no";
    } else {
        auto os = std::ostringstream {};
        os << _chair_assigned;
        output["chair_assigned"]     = os.str();
        output["chair_release_time"] = _chair_release_time.str();
    }

    const auto& infect_time = _infection_logic.get_infection_time();
    if (infect_time) {
        output["infection_time"] = infect_time->str();
    }

    // Remove from simulation
    _flyweight->space->remove_agent(this);
    _flyweight->context->removeAgent(this);

    return output;
}

/// @brief Get the time the patient was admitted at the hospital
/// @return The date (with a precission of seconds) the patient was admitted
sti::datetime sti::patient_agent::entry_time() const
{
    return _entry_time;
}

/// @brief Get the infection logic
/// @return A pointer to the infection logic
const sti::infection_cycle* sti::patient_agent::get_infection_logic() const
{
    return &_infection_logic;
}

void sti::patient_agent::act()
{
    _infection_logic.tick();

    // The patient 0 must request a chair, and walk to it
    if (getId().id() != 0) {

        const auto printas = [&](const auto& str) {
            auto os = std::ostringstream {};
            os << getId() << ": "
               << str;
            print(os.str());
        };

        const auto get_my_loc = [&]() {
            const auto repast_coord = _flyweight->space->get_discrete_location(getId());
            return coordinates {
                repast_coord.getX(),
                repast_coord.getY()
            };
        };

        if (_stage == STAGES::START) {
            // Request a chair
            _flyweight->chairs->request_chair(getId());
            printas("Requested chair");
            _stage = STAGES::WAITING_CHAIR;
            return;
        }

        if (_stage == STAGES::WAITING_CHAIR) {

            // Check if the request has been satisfied
            const auto response = _flyweight->chairs->get_response(getId());
            if (response) {
                if (response->chair_location) {

                    _chair_assigned = response->chair_location.get();
                    auto os         = std::ostringstream {};
                    os << "Got chair:"
                       << _chair_assigned;
                    printas(os.str());
                    _stage = STAGES::WALKING_TO_CHAIR;
                } else {
                    auto os = std::ostringstream {};
                    printas("No chair available, leaving");
                    _stage = STAGES::WALKING_TO_EXIT;
                }
            } else {
                printas("No response from chair manager");
            }
            return;
        }

        if (_stage == STAGES::WALKING_TO_CHAIR) {

            if (get_my_loc() == _chair_assigned) {
                printas("Arrived at chair");
                // Wait 15 minutes
                _chair_release_time = _flyweight->clk->now() + sti::timedelta { 0, 0, 15, 0 };
                _stage              = STAGES::SIT_DOWN;
            } else {
                const auto destination = repast::Point<int> {
                    _chair_assigned.x,
                    _chair_assigned.y
                };

                const auto my_pos = _flyweight->space->get_continuous_location(getId());

                const auto final_pos = _flyweight->space->move_towards(getId(),
                                                                       destination,
                                                                       _flyweight->walk_speed);
                // assert(ret);

                auto os = std::ostringstream {};
                os << "Walking to chair :: "
                   << my_pos << " -> "
                   << final_pos;
                printas(os.str());
            }

            return;
        }

        if (_stage == STAGES::SIT_DOWN) {
            // Wait a couple of ticks
            if (_chair_release_time < _flyweight->clk->now()) {
                _flyweight->chairs->release_chair(_chair_assigned);
                auto os = std::ostringstream {};
                os << "Chair released, going to exit at "
                   << _flyweight->hospital->get_all(hospital_plan::tile_t::EXIT).at(0);
                printas("Chair released");
                _stage = STAGES::WALKING_TO_EXIT;
            } else {
                printas("Still waiting for chair release");
            }
            return;
        }

        if (_stage == STAGES::WALKING_TO_EXIT) {
            const auto& exit_loc = _flyweight->hospital->get_all(hospital_plan::tile_t::EXIT).at(0);

            const auto my_pos = _flyweight->space->get_continuous_location(getId());

            const auto destination = repast::Point<int> {
                exit_loc.x,
                exit_loc.y
            };

            const auto final_pos = _flyweight->space->move_towards(getId(),
                                                                   destination,
                                                                   _flyweight->walk_speed);

            auto os = std::ostringstream {};
            os << "Walking to exit "
               << exit_loc
               << "  :: "
               << my_pos << "(" << _flyweight->space->get_discrete_location(getId()) << ")"
               << " -> "
               << final_pos;
            printas(os.str());
            return;
        };
    }
}