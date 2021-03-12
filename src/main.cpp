#include <boost/lexical_cast.hpp>
#include <memory>
#include <iostream>
#include <string>
#include <unistd.h>

#include <boost/mpi.hpp>
#include <repast_hpc/RepastProcess.h>

#include "model.hpp"
#include "print.hpp"

int main(int argc, char** argv)
{

    // Get the files used for the simulation
    const auto args = std::vector<char*> { argv, argv + argc }; // NOLINT

    const auto config_file = std::string { args[1] }; // Configuration file is arg 1
    const auto props_file  = std::string { args[2] }; // Properties file is arg 2

    // Start MPI
    boost::mpi::environment  env(argc, argv);
    boost::mpi::communicator world;

    // Look for the debug flag
    if (args.size() == 4) {
        const auto third_flag = std::string { args[3] };
        if (third_flag.substr(0, 8) == "--debug=") {
            // Set a volatile int and expect the debugger to change it
            const auto ps_str        = third_flag.substr(8, third_flag.size() - 8);
            const auto debug_process = boost::lexical_cast<int>(ps_str);
            if (world.rank() == debug_process) {
                volatile auto connected = false;
                const auto    pid       = getpid();
                std::cout << "Waiting for connection, PID: " << pid << std::endl;
                while (!connected) {
                    sleep(2);
                }
            }
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