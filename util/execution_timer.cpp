#include "util/include/execution_timer.hpp"

namespace util {

#ifdef ENABLE_UTIL_EXECUTION_TIMING

ExecutionTimer::ExecutionTimer(
    const char* func_name, const char* file, int line)
  : name_(func_name),
    file_(file),
    line_(line),
    start_time_(std::chrono::system_clock::now()) {}

ExecutionTimer::~ExecutionTimer() {
  auto end_time = std::chrono::system_clock::now();
  auto elapsed = std::chrono::duration<double,std::milli>(end_time - start_time_);

  UTIL_STREAM_HELPER_IMPL(kInfo, file_, line_)
      << name_ << " completed execution in time " << elapsed.count() << ".";
}

#endif

}  // namespace util