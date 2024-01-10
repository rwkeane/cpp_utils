#include <cassert>

#include "util/logger_impl.hpp"

namespace util {

Logger* g_logger_singleton_ = nullptr;

// static
void Logger::CreateGlobalInstance(std::ostream& info_stream, std::ostream& error_stream) {
  assert(!g_logger_singleton_);

  g_logger_singleton_ = CreateLogger(info_stream, error_stream).release();
}

// static
Logger* Logger::GetGlobalInstance() {
  assert(g_logger_singleton_);

  return g_logger_singleton_;
}

}  // namespace