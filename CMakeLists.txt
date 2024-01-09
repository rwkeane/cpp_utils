cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_COMPILER "gcc")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 11)

project(cpp_utils LANGUAGES CXX)

add_library(cpp_utils "")
set_target_properties(cpp_utils PROPERTIES LINKER_LANGUAGE CXX)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)

# add_subdirectory(third_party)

# My source code
include_directories(${PROJECT_SOURCE_DIR})

target_sources(cpp_utils
    PUBLIC
        memory/include/optional.hpp
        memory/include/weak_ptr.hpp
        threading/include/nearly_lockless_fifo.hpp
        threading/include/task_runner_factory.hpp
        threading/include/task_runner.hpp
        util/include/bind.hpp
        util/include/compiler_hints.hpp
        util/include/logger.hpp
    PRIVATE
        memory/stack_ptr.hpp
        threading/multithreaded_task_runner.hpp
        threading/single_threaded_task_runner.hpp
        util/logger_impl.cpp
        util/logger_impl.hpp
)