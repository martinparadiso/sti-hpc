#include "real_doctors.hpp"

#include "proxy_doctors.hpp"
#include <algorithm>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/mpi/communicator.hpp>
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
    , _doctors {
        [&]() {
            auto queue = decltype(_doctors) {};
            for (const auto& doctor : hospital.doctors()) {
                queue[doctor.type].push_back(doctor.patient_chair.continuous());
            }
            return queue;
        }()
    }
    , _patients_queue {}
    , _front {}
{
}

////////////////////////////////////////////////////////////////////////////////
// REAL_DOCTORS METHODS
////////////////////////////////////////////////////////////////////////////////

/// @brief Enqueue in a doctor type
/// @param type The doctor specialization to enqueue in
/// @param id The agent id
/// @param timeout Instant of time that the patient will leave if doesn't receive attention
void sti::real_doctors::enqueue(const doctor_type& type, const repast::AgentId& id, const datetime& timeout)
{
    insert_in_order(type, { id, timeout });
}

/// @brief Remove an agent from the queues
/// @param type The doctor type/specialty to dequeue from
/// @param id The agent id to dequeue
void sti::real_doctors::dequeue(const doctor_type& type, const agent_id& id)
{
    remove_patient(type, id);
}

/// @brief Check if the agent has a turn assigned. Returns the location of the doctor to go
/// @param type The doctor type/specialty to check
/// @param id The agent ID to check
/// @return If the agent has a doctor assigned, the destination
boost::optional<sti::doctors_queue::position> sti::real_doctors::is_my_turn(const doctor_type& type, const agent_id& id)
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
    auto       to_enqueue = std::vector(static_cast<std::size_t>(world_size), std::vector<std::pair<doctor_type, patient_turn>> {});
    auto       to_dequeue = std::vector(static_cast<std::size_t>(world_size), std::vector<std::pair<doctor_type, repast::AgentId>> {});
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

    // Construct the new front
    for (auto& [doc_type, queue] : _front) {

        queue.clear(); // Remove the existing patients

        const auto& doctors  = _doctors.at(doc_type);
        const auto& patients = _patients_queue.at(doc_type);

        auto doctor  = doctors.begin();
        auto patient = patients.begin();

        // Insert patients in the queue until there are no more doctors or
        // patients, whatever happens first
        while (doctor != doctors.end() && patient != patients.end()) {

            queue.push_back({ *doctor, patient->id });

            ++doctor;
            ++patient;
        }
    }

    // If the print flag is true, print the front
    if constexpr (sti::debug::doctors_print_front) {
        std::cout << "Current doctors: \n";
        for (const auto& [doc_type, queue] : _front) {
            std::cout << "-> " << doc_type << "\n";
            for (const auto& turn : queue) {
                std::cout << "   -> "
                          << turn.first << " "
                          << turn.second
                          << "\n";
            }
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
void sti::real_doctors::insert_in_order(const doctor_type& type, const patient_turn& turn)
{
    auto& queue = _patients_queue[type];

    // The patients must be inserted according to the assigned priority, which
    // is implemented with a timeout/'wait_until <timeout> before leaving'. The
    // queue must remain sorted, where queue[i].timeout < queue[i+1].timeout.

    // The first P positions are the 'current turns', those are ignored durning
    // the insertion lookup because they represent patients that are either
    // walking to the assigned doctor, or are currently in the doctor. Modifying
    // this positions will break the logic, because the new patient will think
    // is his turn.
    // P is in the range [0, number_of_doctors], depending of the number of
    // patients enqueued in the doctor type. If there are more patients than
    // doctors, the value will be number_of_doctors.

    // std::list doesn't suport random access, (queue.begin() + P), must iterate P
    // times incrementing the iterator
    const auto front_size  = _doctors[type].size();
    const auto queue_begin = [&]() {
        auto begin = queue.begin();
        for (auto p = 0UL; p < front_size; ++p) {
            ++begin;
        }
        return begin;
    }();

    // The iteration lookup begins in position D, where D is the number of
    // doctors for the specified type. If there are less patients
    // (queue.size()) than doctors, the first iterator will be 'after' the last
    // iterator. In this scenario find_if() will return the last iterator, so
    // the insertion should be correct. If there are more patients than doctors,
    // the insertion will follow the criteria described in the firsts comments,
    // using the timeout to determine 'more urgent' patients.
    const auto insert_it = std::find_if(queue_begin, queue.end(),
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
void sti::real_doctors::remove_patient(const doctor_type& type, const agent_id& id)
{
    _patients_queue[type].remove_if([&](const auto& turn) {
        return turn.id == id;
    });
}