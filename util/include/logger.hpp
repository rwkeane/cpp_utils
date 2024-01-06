#ifndef DBC3B62B_4E2A_49CB_8598_AFE1922E4916
#define DBC3B62B_4E2A_49CB_8598_AFE1922E4916

#include <ostream>
#include <sstream>
#include <utility>
#include <thread>

namespace util {

// This class defines a number of thread-safe logging utilities. To use the
// logger, do the following:
//
//   1. Call the INITIALIZE_LOGGER function prior to the first logging function.
//   2. Use the LOG_UTIL_* macros to get an ostream to which logs can be
//      written. Following the end of the line, the stream will be processed as
//      written.
//
// Logging can be enabled / disabled by defining the ENABLE_UTIL_LOGGING
// constant. Note that (AFTER the INITIALIZE LOGGER call), all functions defined
// by this class are thread-safe.
//
// TODO: Move this to a build flag.
#define ENABLE_UTIL_LOGGING

// Logging Macros to be used by the library user.
//
// TODO: Add fine-grained log level controls.
#ifdef ENABLE_UTIL_LOGGING

// NOTE: This call leaks memory by design.
#define INITIALIZE_LOGGER(info_stream, error_stream) \
    util::Logger::CreateGlobalInstance(std::move(info_stream), \
                                       std::move(error_stream))
#define LOG_UTIL_VERBOSE UTIL_STREAM_HELPER(Logger::LogLevel::kVerbose)
#define LOG_UTIL_INFO UTIL_STREAM_HELPER(Logger::LogLevel::kVerbose)
#define LOG_UTIL_WARNING UTIL_STREAM_HELPER(Logger::LogLevel::kVerbose)
#define LOG_UTIL_ERROR UTIL_STREAM_HELPER(Logger::LogLevel::kVerbose)
#define LOG_UTIL_CRITICAL UTIL_STREAM_HELPER(Logger::LogLevel::kVerbose)

#else

#define INITIALIZE_LOGGER(info_stream, error_stream)
#define LOG_UTIL_VERBOSE
#define LOG_UTIL_INFO 
#define LOG_UTIL_WARNING
#define LOG_UTIL_ERROR 
#define LOG_UTIL_CRITICAL

#endif

#define UTIL_STREAM_HELPER(level_enum) \
  internal::Voidify() & (Logger::CreateLogMessage( \
      Logger::LogLevel::level_enum, __FILE__,__LINE__).stream())

// Top-level of the class responsible for handling thread-safe logging. Children
// must complete its implementation and handle reading to the provided streams.
class Logger {
 public:
  enum LogLevel {
    // Very detailed information, often used for evaluating performance.
    kVerbose = 0,

    // Used occasionally to note events of interest, but not for indicating any
    // problems. This is also used for general console messaging.
    kInfo = 1,

    // Indicates a problem that may or may not lead to an operational failure.
    kWarning = 2,

    // Indicates an operational failure that may or may not cause a component to
    // stop working.
    kError = 3,

    // Indicates a logic flaw, corruption, impossible/unanticipated situation,
    // or operational failure so serious that the code should exit.
    kFatal = 4,
  };
  
  class LogMessage {
   public:
    LogMessage(LogLevel level, const char* file, int line,
               std::thread::id thread_id)
        : level_(level), file_(file), line_(line), thread_id_(thread_id) {}

    ~LogMessage() {
      Logger::GetGlobalInstance()->LogMessageImpl(*this);
      if (level_ == LogLevel::kFatal) {
        // TODO: Exit program
      }
    }

    std::ostream& stream() { return stream_; }
    const LogLevel level() const { return level_; }
    const char* file() const { return file_; }
    const std::thread::id& thread_id() const { return thread_id_; }

  protected:
    const LogLevel level_;

    // The file here comes from the __FILE__ macro, which should persist while
    // we are doing the logging. Hence, keeping it unmanaged here and not
    // creating a copy should be safe.
    const char* const file_;
    const int line_;
    const std::thread::id thread_id_;
    std::stringstream stream_;
  };

  Logger() = default;
  virtual ~Logger() = default;

  // Creates the singleton Logger instance. May only be called once.
  static void CreateGlobalInstance(std::ostream& info_stream, std::ostream& error_stream);

  // Gets the global instance of the logger for this process.
  static Logger* GetGlobalInstance();

  // Logs a message.
  static LogMessage CreateLogMessage(LogLevel level, const char* file, int line) {
    return LogMessage(level, file, line, std::this_thread::get_id());
  }

 protected:
  virtual void LogMessageImpl(LogMessage& msg) = 0;
};

namespace internal {

class Voidify {
 public:
  void operator&(std::ostream&) {}
};

}  // namespace internal

}  // namespace

#endif /* DBC3B62B_4E2A_49CB_8598_AFE1922E4916 */
