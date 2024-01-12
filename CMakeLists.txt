cmake_minimum_required(VERSION 3.13)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# NOTE: Must be 14 for testing dependencies. For cpp_utils target, it is
# manually set to C++11.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILER "gcc")

# Set build flags for all targets.
#
# -fno-exceptions causes the compiler to choose the implementation of the STL
# that uses abort() calls instead of throws, as well as issue compile errors
# for throw calls in user ccode. The no*unwind-tables flags disable generation
# of static unwind tables that are typically used for exceptions, resulting
# in typically smaller object sizes. For some information, see:
# https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html#index-funwind-tables-1203
#
# TODO: Optimize for production builds with -g1 and such.
set(CMAKE_CXX_FLAGS
    "-g2 -O2 -lm -ldl -Wall -Werror -Wpointer-arith -finline-functions \
    -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables \
    -Wall -Wno-sign-compare -Wpointer-arith -Wformat-security \
    -Wstrict-aliasing -Wcast-align -Wmissing-field-initializers")

# Add first to ensure dependencies are built before they are needed.
add_subdirectory(third_party)

project(cpp_utils LANGUAGES CXX)
add_library(cpp_utils "")
target_compile_features(cpp_utils PRIVATE cxx_std_11)
set_target_properties(cpp_utils PROPERTIES LINKER_LANGUAGE CXX)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)

# Testing support
# enable_testing() 
add_executable(run_unittests "")
set_target_properties(run_unittests PROPERTIES LINKER_LANGUAGE CXX)
add_dependencies(run_unittests cpp_utils)

# My source code
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party/googletest/googletest/include
    ${PROJECT_SOURCE_DIR}/third_party/googletest/googlemock/include)

add_subdirectory(memory)
add_subdirectory(threading)
add_subdirectory(util)

# target_link_libraries(run_unittests googletest)
target_link_libraries(run_unittests gtest_main gmock_main -lm -ldl stdc++)

# gtest_discover_tests(run_unittests)