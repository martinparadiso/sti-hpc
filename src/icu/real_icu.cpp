#include "real_icu.hpp"

#include <algorithm>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/json/object.hpp>
#include <cstdint>
#include <fstream>
#include <repast_hpc/AgentId.h>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../json_serialization.hpp"
#include "../agent_factory.hpp"
#include "../clock.hpp"
#include "../contagious_agent.hpp"
#include "../hospital_plan.hpp"
#include "../object.hpp"

namespace {

////////////////////////////////////////////////////////////////////////////////
// HELPER EXCEPTIONS
///////////////////////////////////////////////////////////////////////////////

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
}

////////////////////////////////////////////////////////////////////////////
// STATISTICS
////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct a real ICU keeping track of beds assigned
/// @param communicator The MPI communicator
/// @param mpi_tag The MPI tag for the communication
/// @param hospital_props The hospital properties stored in a JSON object
/// @param hospital_plan The hospital plan.
/// @param space The space_wrapper
/// @param clock The simulation clock
/// @param af Agent factory, to construct the beds
sti::real_icu::real_icu(communicator_ptr           communicator,
                        int                        mpi_tag,
                        const boost::json::object& hospital_props,
                        const hospital_plan&       hospital_plan,
                        space_wrapper*             space,
                        clock*                     clock)
    : icu { hospital_props }
    , _communicator { communicator }
    , _mpi_base_tag { mpi_tag }
    , _space { space }
    , _clock { clock }
    , _icu_entry { hospital_plan.icu().entry_location }
    , _icu_exit { hospital_plan.icu().exit_location }
    , _stats { std::make_unique<statistics>() }
    , _reserved_beds { 0 }
    , _bed_pool(hospital_props.at("parameters").at("icu").at("beds").as_uint64(), { nullptr, nullptr })
{
}

sti::real_icu::~real_icu() = default;

/// @brief Due to dependencies, beds cannot be created durning construction
/// @param af Agent factory, to construct the beds
void sti::real_icu::create_beds(agent_factory& af)
{
    for (auto& bed : _bed_pool) {
        bed.first = af.insert_new_object("bed", object_infection_cycle::STAGE::CLEAN);
    }
}

////////////////////////////////////////////////////////////////////////////
// BEHAVIOUR
////////////////////////////////////////////////////////////////////////////

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
    auto incoming_requests = std::vector(static_cast<std::uint64_t>(world_size), std::vector<repast::AgentId> {});

    // Receive the new requests
    for (auto p = 0; p < world_size; ++p) {
        if (p != this_rank) {
            _communicator->recv(p, mpi_tag, incoming_requests.at(static_cast<std::uint64_t>(p)));
        }
    }
    mpi_tag++;

    // Process the requests
    auto outgoing_responses = std::vector(static_cast<std::uint64_t>(world_size), std::vector<message> {});

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

/// @brief Absorb nearby agents into the ICU void
/// @details The ICU is implemented as an spaceless entity, patients are
/// absorbed into the ICU dimension and dissapear from space. They can
/// contract the desease via environment, but not through other patients.
void sti::real_icu::tick()
{

    // Absorb all patients standing in the entry of the ICU
    const auto agents_at_entry = _space->agents_in_cell(_icu_entry);

    for (const auto& agent : agents_at_entry) {
        if (agent->get_type() == contagious_agent::type::PATIENT) {
            insert(agent);
        }
    }

    // Collect stats, count the number of beds that are actually empty
    const auto beds_in_use = std::count_if(_bed_pool.begin(), _bed_pool.end(),
                                           [&](const auto& pair) {
                                               return pair.second == nullptr;
                                           });

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
                << "icu_status_in_process_"
                << _communicator->rank()
                << ".csv";

    auto file = std::ofstream { tick_status.str() };

    file << "time,beds_reserved,beds_in_use\n";

    for (const auto& status : _stats->tick_status) {
        file << status.time.epoch() << ","
             << status.beds_reserved << ","
             << status.beds_in_use << "\n";
    }

    // Write the admissions, releases and rejections
    auto inout_path = std::ostringstream {};
    inout_path << folderpath
               << "icu_admissions_and_releases_in_process_"
               << _communicator->rank()
               << ".csv";
    auto inout_file = std::ofstream { inout_path.str() };

    inout_file << "time,agent,type\n";

    for (const auto& admission : _stats->agent_admission) {
        inout_file << to_string(admission.first) << ","
                   << admission.second.epoch() << ","
                   << "admission"
                   << "\n";
    }

    for (const auto& release : _stats->agent_release) {
        inout_file << to_string(release.first) << ","
                   << release.second.epoch() << ","
                   << "release"
                   << "\n";
    }

    for (const auto& rejection : _stats->rejections) {
        inout_file << to_string(rejection.first) << ","
                   << rejection.second.epoch() << ","
                   << "release"
                   << "\n";
    }

    // Write the ICU probabilities/randoms generated
    const auto* icu_stats  = icu::stats();
    auto        stats_path = std::ostringstream {};
    stats_path << folderpath
               << "icu_in_process_"
               << _communicator->rank()
               << ".csv";
    auto stats_file = std::ofstream { stats_path.str() };

    stats_file << "sleep_time,assigned\n";

    for (const auto& [sleep_time, assigned] : icu_stats->sleep_times) {
        stats_file << sleep_time.length() << ","
                   << assigned << "\n";
    }
}

////////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/// @brief Insert a patient into the ICU
/// @throws no_more_beds If the bed_pool is full
/// @param patient_ptr A pointer to the patient to remove
void sti::real_icu::insert(sti::contagious_agent* patient)
{
    // Find an empty bed
    auto it = std::find_if(_bed_pool.begin(),
                           _bed_pool.end(),
                           [](const auto& pair) {
                               return pair.second == nullptr;
                           });

    if (it == _bed_pool.end()) throw no_more_beds {};

    it->second = patient;

    // And collect statistics
    _stats->agent_admission.push_back({ patient->getId(), _clock->now() });
}

/// @brief Remove a patient from the ICU
/// @throws no_patient If the agent to remove is not in the bed pool
/// @param patient_ptr A pointer to the patient to remove
void sti::real_icu::remove(const sti::contagious_agent* patient_ptr)
{
    auto it = std::find_if(_bed_pool.begin(), _bed_pool.end(),
                           [&](const auto& pair) {
                               return pair.second == patient_ptr;
                           });

    if (it == _bed_pool.end()) throw no_patient_with_that_id {};

    // Patient found, 'remove' it by nulling the pointer. Then insert the
    // patient into the ICU exit.
    it->second = nullptr;

    // Insert the patient into the space
    _space->move_to(patient_ptr->getId(), _icu_exit);

    // Decrease the number of beds in use
    _reserved_beds -= 1;

    // Update stats
    _stats->agent_release.push_back({ patient_ptr->getId(), _clock->now() });
}