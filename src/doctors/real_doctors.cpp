#include "real_doctors.hpp"

#include "proxy_doctors.hpp"
#include <algorithm>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/mpi/communicator.hpp>
#include <sstream>
#include <vector>
#include <iostream>

#include "../debug_flags.hpp"
#include "../hospital_plan.hpp"

/// @brief Construct real queue, specifing the rank of the real queue
/// @param communicator The MPI Communicator
/// @param mpi_tag The MPI tag used for comunication
/// @param hospital The hospital plan, to extract the doctors
sti::real_doctors::real_doctors(communicator_ptr communicator, int mpi_tag, const hospital_plan& hospital)
    : _communicator { communicator }
    , _my_rank { _communicator->rank() }
    , _base_tag { mpi_tag }
    // , _doctors {
    //     [&]() {
    //         auto queue = decltype(_doctors) {};
    //         for (const auto& doctor : hospital.doctors()) {
    //             queue[doctor.type].push_back(doctor.patient_chair.continuous());
    //         }
    //         return queue;
    //     }()
    // }
    , _front { [&]() {
        auto front = decltype(_front) {};
        for (const auto& doctor : hospital.doctors()) {
            front[doctor.type][doctor.patient_chair.continuous()] = boost::none;
        }
        return front;
    }() }
    , _patients_queue { [&]() {
        auto pq = decltype(_patients_queue) {};
        for (const auto& [specialty, boxes] : _front) {
            pq[specialty] = {};
        }
        return pq;
    }() }
{
}

////////////////////////////////////////////////////////////////////////////////
// REAL_DOCTORS METHODS
////////////////////////////////////////////////////////////////////////////////

/// @brief Enqueue in a doctor type
/// @param type The doctor specialization to enqueue in
/// @param id The agent id
/// @param timeout Instant of time that the patient will leave if doesn't receive attention
void sti::real_doctors::enqueue(const specialty_type& type, const repast::AgentId& id, const datetime& timeout)
{
    insert_in_order(type, { id, timeout });
}

/// @brief Remove an agent from the queues
/// @param type The doctor type/specialty to dequeue from
/// @param id The agent id to dequeue
void sti::real_doctors::dequeue(const specialty_type& type, const agent_id& id)
{
    remove_patient(type, id);
}

/// @brief Check if the agent has a turn assigned. Returns the location of the doctor to go
/// @param type The doctor type/specialty to check
/// @param id The agent ID to check
/// @return If the agent has a doctor assigned, the destination
boost::optional<sti::doctors_queue::position> sti::real_doctors::is_my_turn(const specialty_type& type, const agent_id& id)
{
    const auto& queue = _front[type];
    const auto  it    = std::find_if(queue.begin(), queue.end(),
                                 [&](const auto& turn) {
                                     return turn.second == id;
                                 });

    if (it != queue.end()) {
        return it->first;
    }

    return {};
}

/// @brief Sync the queues between the processes
void sti::real_doctors::sync()
{
    const auto my_rank    = _communicator->rank();
    const auto world_size = _communicator->size();
    auto       to_enqueue = std::vector(static_cast<std::size_t>(world_size), std::vector<std::pair<specialty_type, patient_turn>> {});
    auto       to_dequeue = std::vector(static_cast<std::size_t>(world_size), std::vector<std::pair<specialty_type, repast::AgentId>> {});
    auto       mpi_tag    = _base_tag;

    // Receive the new enqueue
    for (auto p = 0; p < world_size; ++p) {
        if (p != my_rank) {
            _communicator->recv(p, mpi_tag, to_enqueue.at(static_cast<std::size_t>(p)));
        }
    }
    mpi_tag++;

    // Receive the new dequeues
    for (auto p = 0; p < world_size; ++p) {
        if (p != my_rank) {
            _communicator->recv(p, mpi_tag, to_dequeue.at(static_cast<std::size_t>(p)));
        }
    }
    mpi_tag++;

    // Perform the enqueues
    for (const auto& vector : to_enqueue) {
        for (const auto& new_enqueue : vector) {
            insert_in_order(new_enqueue.first, new_enqueue.second);
        }
    }

    // Perform the dequeues
    for (const auto& vector : to_dequeue) {
        for (const auto& new_dequeue : vector) {
            remove_patient(new_dequeue.first, new_dequeue.second);
        }
    }

    // Update the front, poping patients from the queues
    for (auto& [specialty, doctors] : _front) {
        auto& patients = _patients_queue.at(specialty);
        for (auto& doctor_location : doctors) {
            if (!doctor_location.second.is_initialized() && !patients.empty()) {
                doctor_location.second = patients.front().id;
                patients.pop_front();
            }
        }
    }

    // If the print flag is true, print the front
    if constexpr (sti::debug::doctors_print_front) {
        auto os = std::ostringstream {};
        os << "Current doctors: \n";
        for (const auto& [doc_type, queue] : _front) {
            os << "-> " << doc_type << "\n";
            for (const auto& turn : queue) {
                os << "   -> "
                   << turn.first << " "
                   << turn.second
                   << "\n";
            }
        }
        if (os.str().size() > 18) {
            std::cout << os.str();
        }
    } // if constexpr

    // Broadcast the new_front
    boost::mpi::broadcast(*_communicator, _front, _my_rank);
}

////////////////////////////////////////////////////////////////////////////////
// REAL_DOCTORS HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/// @brief Insert a new patient turn in order
/// @param type The doctor type/specialty to enqueue in
/// @param turn The patient turn, containing id and timeout/priority
void sti::real_doctors::insert_in_order(const specialty_type& type, const patient_turn& turn)
{
    // The patients must be inserted according to the assigned priority, which
    // is implemented with a timeout/'wait_until <timeout> before leaving'. The
    // queue must remain sorted, where queue[i].timeout < queue[i+1].timeout.

    auto&      queue     = _patients_queue[type];
    const auto insert_it = std::find_if(queue.begin(), queue.end(),
                                        [&](const auto& queue_turn) {
                                            // Returns true when the turn-in-queue
                                            // timeouts after the inserting turn
                                            return turn.timeout < queue_turn.timeout;
                                        });

    queue.insert(insert_it, turn);
}

/// @brief Remove an agent from a queue
/// @param type The doctor type to dequeue from
/// @param id The agent id
void sti::real_doctors::remove_patient(const specialty_type& type, const agent_id& id)
{
    // Check if the patient is in the front
    auto& boxes  = _front.at(type);
    auto  box_it = std::find_if(boxes.begin(), boxes.end(),
                               [&](const auto& kv) { return kv.second == id; });

    if (box_it != boxes.end()) {
        box_it->second = boost::none;
    } else {
        _patients_queue.at(type).remove_if([&](const auto& turn) {
            return turn.id == id;
        });
    }
}