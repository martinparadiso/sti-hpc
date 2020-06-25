cmake_minimum_required(VERSION 3.16)
project(sti-hpc VERSION 0.1.0 LANGUAGES CXX)

# Set standard to C++14
set_property(GLOBAL PROPERTY CXX_STANDARD 14)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)

# Export compile commands (for clangd)
set(CMAKE_EXPORT_COMPILE_OPTIONS ON)

# Clang sanitazion (disabled by default) ======================================

# Address sanitazion
option(ASAN "Enable GCC/Clang address sanitazion" OFF)

if(ASAN)
	add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
	link_libraries(-fsanitize=address)
endif()

# Set maximum level of warning ================================================
if(MSVC)
	add_compile_options(/W4 /WX)
else()
	add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

# Guideline Support Library
add_subdirectory(lib/GSL)

# Demo ========================================================================
add_executable(sti-demo src/main.cpp)
target_include_directories(sti-demo PRIVATE include/)
target_link_libraries(demo PRIVATE Microsoft.GSL::GSL)

# TODO link repast
# TODO link repast dependencies

# clang-tidy per-target (to avoid analizing external libraries)
set_target_properties(demo PROPERTIES CXX_CLANG_TIDY clang-tidy)
