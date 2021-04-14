#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <iostream>
#include <string>
#include <unistd.h>

#include <boost/mpi.hpp>
#include <repast_hpc/RepastProcess.h>

#include "model.hpp"

int main(int argc, char** argv)
{

    // Convert the arguments to a C++ vector
    const auto args = std::vector<std::string> { argv, argv + argc }; // NOLINT

    const auto config_file = args.at(1); // Configuration file is arg 1
    const auto props_file  = args.at(2); // Properties file is arg 2

    // Start MPI
    boost::mpi::environment  env(argc, argv);
    boost::mpi::communicator world;

    // Look for the debug flag
    const auto it = std::find_if(args.begin(), args.end(), [](const auto& arg) {
        if (arg.length() >= 8) {
            if (arg.substr(0, 8) == "--debug=") {
                return true;
            }
        }
        return false;
    });

    if (it != args.end()) {
        const auto third_flag    = *it;
        const auto ps_str        = third_flag.substr(8, third_flag.size() - 8);
        const auto debug_process = std::stoi(ps_str);
        if (world.rank() == debug_process) {
            const auto pid = getpid();
            std::cout << "Waiting for debugger attachment, PID: " << pid << std::endl;

            // Set a volatile bool, the debugger must change it
            volatile auto connected = false;
            while (!connected) sleep(1);
            std::cout << "Debugger attached!" << std::endl;
        }
    }

    repast::RepastProcess::init(config_file);

    auto                    model  = std::make_unique<sti::model>(props_file, argc, argv, &world);
    repast::ScheduleRunner& runner = repast::RepastProcess::instance()->getScheduleRunner();

    model->init();
    model->init_schedule(runner);

    runner.run();

    model.reset();

    repast::RepastProcess::instance()->done();

    return 0;
}