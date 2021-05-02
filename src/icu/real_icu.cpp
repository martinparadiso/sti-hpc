#include "real_icu.hpp"

#include <algorithm>
#include <boost/json/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/json/object.hpp>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <repast_hpc/AgentId.h>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../json_serialization.hpp"
#include "../agent_factory.hpp"
#include "../clock.hpp"
#include "../patient.hpp"
#include "../hospital_plan.hpp"
#include "../space_wrapper.hpp"

namespace {

////////////////////////////////////////////////////////////////////////////////
// HELPER EXCEPTIONS
////////////////////////////////////////////////////////////////////////////////

/// @brief Error trying to serialize or deserialize an agent
struct no_more_beds : public std::exception {

    const char* what() const noexcept override
    {
        return "ICU: Something went wrong, a patient try to get into the ICU but there are no more beds";
    }
};

/// @brief Error trying to serialize or deserialize an agent
struct no_patient_with_that_id : public std::exception {

    const char* what() const noexcept override
    {
        return "ICU: Someone tried to remove a patient that is not here";
    }
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// STATISTICS
////////////////////////////////////////////////////////////////////////////////

/// @brief Collect different statistics
struct sti::real_icu::statistics {
    using counter_type = std::uint32_t;

    /// @brief Collect per-tick statistics
    struct status {
        sti::datetime time;
        counter_type  beds_reserved {};
        counter_type  beds_in_use {};
    };

    /// @brief Keeps track of the number of patients currently in the ICU
    std::vector<status>                               tick_status;
    std::vector<std::pair<repast::AgentId, datetime>> agent_admission;
    std::vector<std::pair<repast::AgentId, datetime>> agent_release;
    std::vector<std::pair<repast::AgentId, datetime>> rejections;
};

////////////////////////////////////////////////////////////////////////////////
// MORGUE
////////////////////////////////////////////////////////////////////////////////

/// @brief Pointer to implementation struct
struct sti::real_icu::morgue {
    boost::json::array agent_output_data {};
};

////////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////////

/// @brief Construct a real ICU keeping track of beds assigned
/// @param context A pointer to the repast context
/// @param communicator The MPI communicator
/// @param mpi_tag The MPI tag for the communication
/// @param space The space_wrapper
/// @param hospital_props The hospital properties stored in a JSON object
/// @param hospital_plan The hospital plan.
/// @param clock The simulation clock
/// @param af Agent factory, to construct the beds
sti::real_icu::real_icu(repast::SharedContext<contagious_agent>* context,
                        communicator_ptr                         communicator,
                        int                                      mpi_tag,
                        space_wrapper*                           space,
                        const boost::json::object&               hospital_props,
                        const hospital_plan&                     hospital_plan,
                        clock*                                   clock)
    : _context { context }
    , _communicator { communicator }
    , _mpi_base_tag { mpi_tag }
    , _space { space }
    , _clock { clock }
    , _icu_location { hospital_plan.icu().location }
    , _reserved_beds { 0 }
    , _capacity { static_cast<decltype(_capacity)>(hospital_props.at("parameters").at("icu").at("beds").as_int64()) }
    , _environment(hospital_props)
    , _morgue { std::make_unique<morgue>() }
    , _stats { std::make_unique<statistics>() }
{
}

sti::real_icu::~real_icu() = default;

/// @brief Due to dependencies, beds cannot be created durning construction
/// @param if Infection factory, to construct the beds
void sti::real_icu::create_beds(infection_factory& infection_factory)
{
    for (auto i = 0U; i < _capacity; ++i) {
        _bed_pool.push_back({ infection_factory.make_object_infection("bed", object_infection::STAGE::CLEAN), nullptr });
    }
}

////////////////////////////////////////////////////////////////////////////////
// ADMISSION MANAGEMENT
////////////////////////////////////////////////////////////////////////////////

/// @brief Check if the request has been processed
/// @return If the request was processed by the manager, True if there is a bed, false otherwise
boost::optional<bool> sti::real_icu::peek_response(const repast::AgentId& id) const
{
    // Search the vector of pending responses for the messages with that id
    const auto it = std::find_if(_pending_responses.begin(),
                                 _pending_responses.end(),
                                 [&](const auto& r) {
                                     return r.first == id;
                                 });

    if (it == _pending_responses.end()) {
        return boost::none;
    }

    const auto response = *it;
    return response.second;
}

/// @brief Check if the request has been processed
/// @return If the request was processed by the manager, True if there is a bed, false otherwise
boost::optional<bool> sti::real_icu::get_response(const repast::AgentId& id)
{
    // Search the vector of pending responses for the messages with that id
    const auto it = std::find_if(_pending_responses.begin(),
                                 _pending_responses.end(),
                                 [&](const auto& r) {
                                     return r.first == id;
                                 });

    if (it == _pending_responses.end()) {
        return boost::none;
    }

    const auto response = *it;
    _pending_responses.erase(it);
    return response.second;
}

/// @brief Sync the requests and responses between the processes
void sti::real_icu::sync()
{
    const auto world_size = _communicator->size();
    const auto this_rank  = _communicator->rank();
    auto       mpi_tag    = _mpi_base_tag;

    // Thr incoming requests are actually world_size - 1, but using a map is
    // more expensive
    auto incoming_requests = std::vector(static_cast<std::uint64_t>(world_size), std::vector<request_message> {});

    // Receive the new requests
    for (auto p = 0; p < world_size; ++p) {
        if (p != this_rank) {
            _communicator->recv(p, mpi_tag, incoming_requests.at(static_cast<std::uint64_t>(p)));
        }
    }
    mpi_tag++;

    // Process the requests
    auto outgoing_responses = std::vector(static_cast<std::uint64_t>(world_size), std::vector<response_message> {});

    for (auto p = 0; p < world_size; ++p) {
        if (p != this_rank) {
            for (const auto& id : incoming_requests.at(static_cast<std::uint64_t>(p))) {
                // If the number of reserved beds is less than the total number of beds,
                // increment the reserved counter and queue the response as true
                if (_reserved_beds < _bed_pool.size()) {
                    _reserved_beds += 1;
                    outgoing_responses.at(static_cast<std::uint64_t>(p)).push_back({ id, true });
                } else {
                    outgoing_responses.at(static_cast<std::uint64_t>(p)).push_back({ id, false });
                }
            }
        }
    }

    // Send the responses
    for (auto p = 0; p < world_size; ++p) {
        if (p != this_rank) {
            _communicator->send(p, mpi_tag, outgoing_responses.at(static_cast<std::uint64_t>(p)));
        }
    }
}

/// @brief Execute periodic actions
void sti::real_icu::tick()
{
    // Kill the patients
    for (auto& [bed, patient] : _bed_pool) {
        if (patient != nullptr && patient->current_state() == patient_fsm::STATE::AWAITING_DELETION) {
            kill(patient);
        }
    }

    // Collect stats, count the number of beds that have a patient assigned
    const auto beds_in_use = std::count_if(_bed_pool.begin(), _bed_pool.end(),
                                           [&](const auto& pair) {
                                               return pair.second != nullptr;
                                           });

    // Update the number of patients in the infection environment
    _environment.patients(static_cast<std::uint32_t>(beds_in_use));

    // Run the infection logic
    for (auto& [bed, patient] : _bed_pool) {
        if (patient != nullptr) {
            bed.interact_with(*patient->get_infection_logic());
            patient->get_infection_logic()->interact_with(bed);
        }
        bed.tick();
    }

    _stats->tick_status.push_back({ _clock->now(),
                                    _reserved_beds,
                                    static_cast<statistics::counter_type>(beds_in_use) });
}

/// @brief Save the ICU stats into a file
/// @param filepath The path to the folder where
void sti::real_icu::save(const std::string& folderpath) const
{
    // Write per-tick stats
    auto tick_status = std::ostringstream {};
    tick_status << folderpath
                << "/icu_status.p"
                << _communicator->rank()
                << ".csv";

    auto file = std::ofstream { tick_status.str() };

    file << "time,beds_reserved,beds_in_use\n";

    for (const auto& status : _stats->tick_status) {
        file << status.time.seconds_since_epoch() << ","
             << status.beds_reserved << ","
             << status.beds_in_use << "\n";
    }

    // Write the admissions, releases and rejections
    auto inout_path = std::ostringstream {};
    inout_path << folderpath
               << "/icu_admissions_and_releases.p"
               << _communicator->rank()
               << ".csv";
    auto inout_file = std::ofstream { inout_path.str() };

    inout_file << "time,agent,type\n";

    for (const auto& admission : _stats->agent_admission) {
        inout_file << admission.second.seconds_since_epoch() << ","
                   << to_string(admission.first) << ","
                   << "admission"
                   << "\n";
    }

    for (const auto& release : _stats->agent_release) {
        inout_file << release.second.seconds_since_epoch() << ","
                   << to_string(release.first) << ","
                   << "release"
                   << "\n";
    }

    for (const auto& rejection : _stats->rejections) {
        inout_file << to_string(rejection.first) << ","
                   << rejection.second.seconds_since_epoch() << ","
                   << "rejection"
                   << "\n";
    }

    // Save the beds metrics
    auto beds = std::ostringstream {};
    beds << folderpath
         << "/icu_beds.p"
         << _communicator->rank()
         << ".json";
    auto beds_file = std::ofstream { beds.str() };

    auto data = boost::json::array {};
    for (const auto& [bed, patient] : _bed_pool) {
        data.push_back(bed.stats());
    }

    beds_file << data;

    auto morgue = std::ostringstream {};
    morgue << folderpath
           << "/morgue.p"
           << _communicator->rank()
           << ".json";
    auto morgue_file = std::ofstream { morgue.str() };

    morgue_file << _morgue->agent_output_data;
}

////////////////////////////////////////////////////////////////////////////////
// PATIENT INSERTION AND REMOVAL
////////////////////////////////////////////////////////////////////////////////

/// @brief Insert a patient into the ICU
/// @throws no_more_beds If the bed_pool is full
/// @param patient_ptr A pointer to the patient to remove
void sti::real_icu::insert(sti::patient_agent* patient)
{
    // Find an empty bed
    auto it = std::find_if(_bed_pool.begin(),
                           _bed_pool.end(),
                           [](const auto& pair) {
                               return pair.second == nullptr;
                           });

    if (it == _bed_pool.end()) throw no_more_beds {};

    // And assign the patient to that bed
    it->second = patient;

    // Set the icu environment, which infects
    patient->get_infection_logic()->set_environment(&_environment);

    // And collect statistics
    _stats->agent_admission.push_back({ patient->getId(), _clock->now() });
}

/// @brief Remove a patient from the ICU
/// @throws no_patient If the agent to remove is not in the bed pool
/// @param patient_ptr A pointer to the patient to remove
void sti::real_icu::remove(sti::patient_agent* patient_ptr)
{
    auto it = std::find_if(_bed_pool.begin(), _bed_pool.end(),
                           [&](const auto& pair) {
                               return pair.second == patient_ptr;
                           });

    if (it == _bed_pool.end()) throw no_patient_with_that_id {};

    // Patient found, 'remove' it by nulling the pointer
    it->second = nullptr;

    // Decrease the number of beds in use
    _reserved_beds -= 1;

    // Remove the icu environment from the patient
    patient_ptr->get_infection_logic()->set_environment(nullptr);

    // Update stats
    _stats->agent_release.push_back({ patient_ptr->getId(), _clock->now() });
}

////////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////////

/// @brief Kill a patient, remove it from the simulation
/// @param patient_ptr A pointer to the patient being removed
void sti::real_icu::kill(sti::patient_agent* patient_ptr)
{
    auto it = std::find_if(_bed_pool.begin(), _bed_pool.end(),
                           [&](const auto& pair) {
                               return pair.second == patient_ptr;
                           });

    if (it == _bed_pool.end()) throw no_patient_with_that_id {};

    // Decrease the number of beds in use
    _reserved_beds -= 1;

    // Remove the icu environment from the patient
    patient_ptr->get_infection_logic()->set_environment(nullptr);

    // Update stats
    _stats->agent_release.push_back({ patient_ptr->getId(), _clock->now() });

    _morgue->agent_output_data.push_back(patient_ptr->stats());
    _space->remove_agent(patient_ptr);
    _context->removeAgent(patient_ptr);

    // 'remove' it by nulling the pointer
    it->second = nullptr;
}

/// @brief Request a bed in the ICU
/// @param id The ID of the requesting agent
void sti::real_icu::request_bed(const repast::AgentId& id)
{
    // If the number of reserved beds is less than the total number of beds,
    // increment the reserved counter and queue the response as true
    if (_reserved_beds < _bed_pool.size()) {
        _reserved_beds += 1;
        _pending_responses.push_back({ id, true });
    }

    // Otherwise the ICU is full, store the rejection and queue the response as
    // false
    else {
        _stats->rejections.push_back({ id, _clock->now() });
        _pending_responses.push_back({ id, false });
    }
}
