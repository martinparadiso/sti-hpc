#include "mp_queue.hpp"
#include "mp_queue/real_mp_queue.hpp"
#include "mp_queue/proxy_mp_queue.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/mpi.hpp>
#include <sstream>
#include <string>

int main(int argc, char* argv[])
{

    // Configuration
    constexpr auto mpi_tag   = 31432;
    using queue_type         = sti::mp_queue<std::string, mpi_tag>;
    constexpr auto host_rank = 1;
    constexpr auto gen_rank  = 0;
    constexpr auto get_rank  = 2;
    constexpr auto both_rank = 3;
    constexpr auto ticks     = 10;

    // MPI Initialization
    boost::mpi::environment  env(argc, argv);
    boost::mpi::communicator world;

    // Look for the debug flag
    const auto args = std::vector<std::string> { argv, argv + argc };
    if (args.size() == 2) {
        const auto third_flag = std::string { args[1] };
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

    // Use ptr to queue_type to check polymorphism
    auto* mp_queue = [&]() -> queue_type* {
        if (world.rank() == host_rank) {
            return new sti::real_mp_queue<std::string, mpi_tag> { &world };
        }
        return new sti::proxy_mp_queue<std::string, mpi_tag> { &world, host_rank };
    }();

    // Run for T ticks
    for (auto t = 0; t < ticks; ++t) {

        // Sync at the beggining
        mp_queue->sync();

        // Maintain the queue
        if (world.rank() == host_rank) {
        }

        // Put values in the queue
        else if (world.rank() == gen_rank) {
            auto os = std::ostringstream {};
            os << "string from " << gen_rank << " "
               << "tick " << t;
            mp_queue->put(os.str());

            // Durning the first 5 ticks generate two values
            if (t < 5) {
                mp_queue->put(os.str());
            }
        }

        // Retrieve values from the queue
        else if (world.rank() == get_rank) {

            // Request and process 2 values
            for (auto r = 0; r < 2; r++) {

                const auto optional = mp_queue->get();

                // If it's at least the second iteration, there must be a value
                if (t == 0 && optional) {
                    std::cerr << "Error, there was a value in the first iteration" << std::endl;
                    return 1;
                }

                if (t > 0 && !optional) {
                    std::cerr << "No value in tick " << t << std::endl;
                    return 2;
                }

                if (t > 0) {

                    const auto value = optional.get();
                }
                // Finally request values for the next tick
                mp_queue->request_element();
            }
        }

        // Put and retrieve values
        else if (world.rank() == both_rank) {

            auto os = std::ostringstream {};
            os << "string from " << gen_rank << " "
               << "tick " << t;
            mp_queue->put(os.str());

            // Durning the first 5 ticks push 2 values
            if (t < 5) {
                mp_queue->put(os.str());
            }

            const auto optional = mp_queue->get();

            // If it's at least the second iteration, there must be a value
            if (t == 0 && optional) {
                std::cerr << "Error, there was a value in the first iteration" << std::endl;
                return 1;
            }

            if (t > 0 && !optional) {
                std::cerr << "No value in tick " << t << std::endl;
                return 2;
            }

            if (t > 0) {

                const auto value = optional.get();
            }
            // Finally requests elements for the next tick
            mp_queue->request_element();
        }
    }

    delete mp_queue;

    return 0;
}
