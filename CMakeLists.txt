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


# Debug and utils ==============================================================

# Clang tidy function
function(tidy target)
	set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY clang-tidy)
endfunction()

# Address sanitation
option(ASAN "Enable address sanitation options" OFF)
if(ASAN)
	message("Builing with address sanitation")
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

# Demo ========================================================================
add_executable(sti-demo "src/main.cpp"
						"src/clock.cpp"
						"src/entry.cpp"
						"src/model.cpp"
						"src/chair_manager.cpp"
						"src/space_wrapper.cpp"
						"src/infection_logic/human_infection_cycle.cpp"
						"src/infection_logic/object_infection_cycle.cpp"
			  )
target_include_directories(sti-demo PUBLIC include/)
target_compile_options(sti-demo PRIVATE -Wall -Wextra -Wnarrowing -Wconversion -Wpedantic)

# Boost
target_link_directories(sti-demo PRIVATE "${PROJECT_SOURCE_DIR}/lib/boost/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/lib/boost/include/)
target_link_libraries(sti-demo PUBLIC boost_system-mt-x64 boost_filesystem-mt-x64 boost_serialization-mt-x64 boost_mpi-mt-x64)

# MPICH
target_link_directories(sti-demo PRIVATE "${LIB_ROOT_PATH}/mpich/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${LIB_ROOT_PATH}/mpich/include/)
target_link_libraries(sti-demo PUBLIC mpi)

# cURL
target_link_directories(sti-demo PRIVATE "${LIB_ROOT_PATH}/curl/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${LIB_ROOT_PATH}/curl/include/)
target_link_libraries(sti-demo PUBLIC curl)

# NetCDF
target_link_directories(sti-demo PRIVATE "${LIB_ROOT_PATH}/netcdf/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${LIB_ROOT_PATH}/netcdf/include/)
target_link_libraries(sti-demo PUBLIC netcdf)

# NetCDF-C++
target_link_directories(sti-demo PRIVATE "${LIB_ROOT_PATH}/netcdf-cxx/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${LIB_ROOT_PATH}/netcdf-cxx/include/)
target_link_libraries(sti-demo PUBLIC netcdf_c++)

# Repast HPC

target_link_directories(sti-demo PRIVATE "${PROJECT_SOURCE_DIR}/lib/repast/lib")
target_include_directories(sti-demo SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/lib/repast/include/)
target_link_libraries(sti-demo PUBLIC repast_hpc-2.3.1 relogo-2.3.1)

tidy(sti-demo)

# Add python wrapper for execution ============================================
add_custom_command(TARGET sti-demo POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy
				   ${CMAKE_SOURCE_DIR}/utils/runner.py
				   ${CMAKE_CURRENT_BINARY_DIR}/runner.py)			

# Test ========================================================================
add_subdirectory(test)
