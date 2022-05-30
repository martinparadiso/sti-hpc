cmake_minimum_required(VERSION 3.16)
project(sti-hpc VERSION 0.1.0 LANGUAGES CXX)

# Folder containing all dependencies
set(LIB_ROOT_PATH "$ENV{HOME}/repast/")

# Set standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands (for clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable autoremoval of implicit includes, it removes neccesary includes
unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)

# Debug and utils ==============================================================

# Clang tidy function
option(CLANG_TIDY "Enable clang-tidy analisys" OFF)
function(tidy target)
    if(CLANG_TIDY)
        set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY clang-tidy)
    endif()
endfunction()

# Address sanitation
option(ASAN "Enable address sanitization options" OFF)
if(ASAN)
    message("Builing with address sanitization")
endif()

function(sanitize_address target)
    if(ASAN)
        target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target} PUBLIC -fsanitize=address)
    endif()
endfunction()

# Enable LTO
option(ENABLE_LTO "Enable Link Time Optimization" OFF)
if (ENABLE_LTO)
    message("Linking with LTO")
endif()

function(optional_lto target)
    if (ENABLE_LTO)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endfunction()

# Set the mpi runner variable
set(MPIEXEC_BIN "${PROJECT_SOURCE_DIR}/lib/mpich/bin/mpiexec")

# Demo ========================================================================
add_executable(sti-demo 
                        "src/agent_factory.cpp"
                        "src/chair_manager.cpp"
                        "src/clock.cpp"
                        "src/doctors/doctors.cpp"
                        "src/doctors/proxy_doctors.cpp"
                        "src/doctors/real_doctors.cpp"
                        "src/entry.cpp"
                        "src/exit.cpp"
                        "src/hospital_plan.cpp"
                        "src/icu/proxy_icu.cpp"
                        "src/icu/real_icu.cpp"
                        "src/icu/icu.cpp"
                        "src/infection_logic/human_infection_cycle.cpp"
                        "src/infection_logic/infection_factory.cpp"
                        "src/infection_logic/icu_environment.cpp"
                        "src/infection_logic/object_infection.cpp"
                        "src/main.cpp"
                        "src/model.cpp"
                        "src/pathfinder.cpp"
                        "src/patient_fsm.cpp"
                        "src/patient.cpp"
                        "src/person.cpp"
                        "src/queue_manager/proxy_queue_manager.cpp"
                        "src/queue_manager/real_queue_manager.cpp"
                        "src/reception.cpp"
                        "src/space_wrapper.cpp"
                        "src/staff_manager.cpp"
                        "src/triage.cpp"
                        "src/utils.cpp"
              )
target_include_directories(sti-demo PUBLIC include/)
target_compile_options(sti-demo PRIVATE -Wall -Wextra -Wnarrowing -Wconversion -Wpedantic -Wshadow)
optional_lto(sti-demo)
tidy(sti-demo)
sanitize_address(sti-demo)

# Boost
target_link_directories(sti-demo PRIVATE "${PROJECT_SOURCE_DIR}/lib/boost/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/lib/boost/include/)
target_link_libraries(sti-demo PUBLIC boost_system-mt-x64 boost_filesystem-mt-x64 boost_serialization-mt-x64 boost_mpi-mt-x64 boost_json-mt-x64)

# MPICH
target_link_directories(sti-demo PRIVATE "${PROJECT_SOURCE_DIR}/lib/mpich/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/lib/mpich/include/)
target_link_libraries(sti-demo PUBLIC mpi)

# cURL
target_link_directories(sti-demo PRIVATE "${PROJECT_SOURCE_DIR}/lib/curl/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/lib/curl/include/)
target_link_libraries(sti-demo PUBLIC curl)

# NetCDF
target_link_directories(sti-demo PRIVATE "${PROJECT_SOURCE_DIR}/lib/netcdf/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/lib/netcdf/include/)
target_link_libraries(sti-demo PUBLIC netcdf)

# NetCDF-C++
target_link_directories(sti-demo PRIVATE "${PROJECT_SOURCE_DIR}/lib/netcdf-cxx/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/lib/netcdf-cxx/include/)
target_link_libraries(sti-demo PUBLIC netcdf_c++)

# Repast HPC

target_link_directories(sti-demo PRIVATE "${PROJECT_SOURCE_DIR}/lib/repast/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/lib/repast/include/)
target_link_libraries(sti-demo PUBLIC repast_hpc-2.3.1 relogo-2.3.1)

# Test ========================================================================
add_subdirectory(test)
