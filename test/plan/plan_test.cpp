// Basic plan loader test, create a map with the Python util, load in C++ and
// verify the generated map. Note: this does not check the file itself, only
// the map generated from the loader.

#include <cassert>
#include <cstdlib>
#include <string_view>

#include "plan/plan_file.hpp"
#include "plan/plan_tile.hpp"

struct config {
    constexpr static auto script = std::string_view("./plan_generator.py");
};

int main()
{

    using TILES = sti::plan_tile::TILE_ENUM;

    // Generate the map file
    std::system(config::script.data());

    const auto plan = sti::load_plan("./test.hosp");

    // Verify header
    assert(plan.width() == 50);
    assert(plan.height() == 50);

    // Verify border walls, entry and exit
    for (auto i = 0U; i < 50; i++) {
        assert(plan.at(0, i).get_type() == TILES::WALL);
        assert(plan.at(49, i).get_type() == TILES::WALL);
        assert(plan.at(i, 49).get_type() == TILES::WALL);
        
        // Except entry and exit in (23, 0) and (24, 0)
        if (i == 23)
            assert(plan.at(i, 0).get_type() == TILES::ENTRY);
        else if (i == 24)
            assert(plan.at(i, 0).get_type() == TILES::EXIT);
        else
            assert(plan.at(i, 0).get_type() == TILES::WALL);
    }

    // Check reception walls and reception
    for (auto i = 21U; i < 27U; i++) {
        assert(plan.at(i, 5).get_type() == TILES::WALL);
    }
    assert(plan.at(23,4).get_type() == TILES::RECEPTIONIST);
    
    // Check triage
    for (auto i = 0U; i < 5U; i++) {
        assert( plan.at(30 + i, 2).get_type() == TILES::WALL );
        assert( plan.at(30 + i, 6).get_type() == TILES::WALL );
        assert( plan.at(35, 2 + i).get_type() == TILES::WALL );
    }
    assert( plan.at(30, 4).get_type() == TILES::TRIAGE );

    // Check Doctors
    for (auto y = 0U; y < 4U; y++) {
        for (auto x = 0U; x < 32U; x++) {
            assert( plan.at(x + 10, y + 10).get_type() == TILES::DOCTOR );
            const auto doctor = plan.at(x + 10, y + 10).get<sti::doctor_tile>();
            assert( doctor.specialty_id == y * 32 + x );
        }
    }

    // Check group of chairs
    for (auto x = 0U; x < 5U; x += 2) {
        for (auto y = 0U; y < 5U; y += 2) {
            assert( plan.at(20 + x, 20 + y).get_type() == TILES::CHAIR );
        }
    }

    // Check ICU
    for (auto i = 48U; i > 39U; i--) {
        if (i != 44) assert( plan.at(i, 40).get_type() == TILES::WALL );
        assert( plan.at(40, i).get_type() == TILES::WALL );
    }
    assert( plan.at(44, 40).get_type() == TILES::ICU );


    // Basic check of "special locations" ======================================

    assert(plan.get(TILES::ENTRY).size() == 1);
    assert(plan.get(TILES::ENTRY).begin()->x == 23);
    assert(plan.get(TILES::ENTRY).begin()->y == 0);

    assert(plan.get(TILES::EXIT).size() == 1);
    assert(plan.get(TILES::EXIT).begin()->x == 24);
    assert(plan.get(TILES::EXIT).begin()->y == 0);

    assert(plan.get(TILES::RECEPTIONIST).size() == 1);
    assert(plan.get(TILES::RECEPTIONIST).begin()->x == 23);
    assert(plan.get(TILES::RECEPTIONIST).begin()->y == 4);

    for (auto doc_coord : plan.get(TILES::DOCTOR)) {
        const auto doc_tile = plan.at(doc_coord);

        // The doctor type should be:
        const auto expected_spec = (doc_coord.y - 10) * 32 + (doc_coord.x - 10);
        const auto doctor = doc_tile.get<sti::doctor_tile>();
        assert(doctor.specialty_id == expected_spec);
    }

    assert(plan.get(TILES::TRIAGE).size() == 1);
    assert(plan.get(TILES::TRIAGE).begin()->x == 30);
    assert(plan.get(TILES::TRIAGE).begin()->y == 4);

    assert(plan.get(TILES::CHAIR).size() == 9);

    assert(plan.get(TILES::ICU).size() == 1);
    assert(plan.get(TILES::ICU).begin()->x == 44);
    assert(plan.get(TILES::ICU).begin()->y == 40);
    
    return 0;
}
