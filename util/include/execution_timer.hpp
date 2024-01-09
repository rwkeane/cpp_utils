#ifndef D17E910A_4B8E_4E8F_984C_DA70AE3E9822
#define D17E910A_4B8E_4E8F_984C_DA70AE3E9822

#include <chrono>

#include "util/include/logger.hpp"

namespace util {

// This file defines a macro TIME_OPERATION to be used for timing execution of
// runtime operations. Only this macro should be used. The below classes should
// not be used directly.

// TODO: Enable / disable with an additional build flag.
#define ENABLE_UTIL_EXECUTION_TIMING

#ifndef ENABLE_UTIL_EXECUTION_TIMING

#define TIME_OPERATION

#else

#ifndef ENABLE_UTIL_LOGGING
static_assert(false) << "Logging must be enabled for TIME_OPERATION's use.";
#endif

#define TIME_OPERATION (ExecutionTimer(__UTIL_FUNC_NAME__, __FILE__,__LINE__))

// Define the __UTIL_FUNC_NAME__ macro based on what compiler-specific
// function name macros are available.
#ifdef __PRETTY_FUNCTION__
#define __UTIL_FUNC_NAME__ __PRETTY_FUNCTION__
#else
#ifdef __FUNCSIG__
#define __UTIL_FUNC_NAME__ __FUNCSIG__
#else 
#ifdef __FUNCTION__
#define __UTIL_FUNC_NAME__ __FUNCTION__
#else 
#define __UTIL_FUNC_NAME__ __func__
#endif // #ifdef __FUNCTION__
#endif // #ifdef __FUNCSIG__
#endif // #ifdef __PRETTY_FUNCTION__

class ExecutionTimer {
public:
  ExecutionTimer(const char* func_name, const char* file, int line);
  ~ExecutionTimer();

 private:
  const char* name_;
  const char* file_;
  const int line_;

  std::chrono::time_point<std::chrono::system_clock> start_time_;
};

#endif // #ifndef ENABLE_UTIL_LOGGING

}  // namespace util

#endif /* D17E910A_4B8E_4E8F_984C_DA70AE3E9822 */
