target_sources(cpp_utils
    PUBLIC
        include/nearly_lockless_fifo.hpp
        include/task_runner_factory.hpp
        include/task_runner.hpp
    PRIVATE
        multithreaded_task_runner.hpp
        parallel_circular_buffer.hpp
        single_threaded_task_runner.hpp
)

target_sources(run_unittests
    PUBLIC test/parallel_circular_buffer_unittest.cpp
)