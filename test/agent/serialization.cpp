#include <cassert>

#include "contagious_agent.hpp"

int main() {

    // Create a patient a
    const auto patient_a = sti::patient_agent{0.5, 10U};

    // Serialize the data
    const auto data = patient_a.serialize();

    // Create a new patient b, with another default, and change it's content to
    // a
    auto patient_b = sti::patient_agent{0.4, 20U};
    patient_b.update(data);

    assert(patient_a.entry_time().epoch() == 10U);
    assert(patient_a.infect_chance() == 0.5);

    assert(patient_b.entry_time().epoch() == 10U);
    assert(patient_b.infect_chance() == 0.5);

    return 0;
}