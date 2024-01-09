#ifndef DBC3B62B_4E2A_49CB_8598_AFE1922E4916
#define DBC3B62B_4E2A_49CB_8598_AFE1922E4916

#include <ostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

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

// TODO: This call leaks memory. Change the design to make it a user-defined
// instance.
#define INITIALIZE_LOGGER(info_stream, error_stream) \
    util::Logger::CreateGlobalInstance(std::move(info_stream), \
                                       std::move(error_stream))
#define LOG_UTIL_VERBOSE UTIL_STREAM_HELPER(kVerbose)
#define LOG_UTIL_INFO UTIL_STREAM_HELPER(kInfo)
#define LOG_UTIL_WARNING UTIL_STREAM_HELPER(kWarning)
#define LOG_UTIL_ERROR UTIL_STREAM_HELPER(kError)
#define LOG_UTIL_CRITICAL UTIL_STREAM_HELPER(kCritical)

#else

#define INITIALIZE_LOGGER(info_stream, error_stream)
#define LOG_UTIL_VERBOSE
#define LOG_UTIL_INFO 
#define LOG_UTIL_WARNING
#define LOG_UTIL_ERROR 
#define LOG_UTIL_CRITICAL

#endif

#define UTIL_STREAM_HELPER(level_enum) \
  UTIL_STREAM_HELPER_IMPL(level_enum, __FILE__, __LINE__)

#define UTIL_STREAM_HELPER_IMPL(level_enum, file, line) \
  internal::Voidify() & (Logger::CreateLogMessage( \
      Logger::LogLevel::level_enum, file, line).stream())

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
    LogMessage(const LogMessage& other) = delete;
    LogMessage(LogMessage&& other)
        : level_(other.level_),
          line_(other.line_),
          thread_id_(other.thread_id_),
          file_(std::move(other.file_)),
          stream_(std::move(other.stream_)) {
      other.IsDoneLogging();
    }

    LogMessage& operator=(const LogMessage& other) = delete;
    LogMessage& operator=(LogMessage&& other) = default;

    ~LogMessage() {
      if (!should_log_) {
        return;
      }

      Logger::GetGlobalInstance()->LogMessageImpl(std::move(*this));
    }

    // To be called when this object should no longer generate a log statement.
    void IsDoneLogging() { should_log_ = false; }

    // Accessors.
    std::ostream& stream() { return stream_; }
    const LogLevel level() const { return level_; }
    const std::string& file() const { return file_; }
    const int line() const { return line_; }
    const std::thread::id& thread_id() const { return thread_id_; }

  protected:
    bool should_log_ = true;

    const LogLevel level_;
    std::string file_;
    const int line_;
    const std::thread::id thread_id_;
    std::stringstream stream_;
  };

  Logger() = default;
  virtual ~Logger() = default;

  // Creates the singleton Logger instance. May only be called once.
  //
  // TODO: The implementation is templated, so these don't need to be ostream
  // types.
  static void CreateGlobalInstance(std::ostream& info_stream, std::ostream& error_stream);

  // Gets the global instance of the logger for this process.
  static Logger* GetGlobalInstance();

  // Logs a message.
  static LogMessage CreateLogMessage(LogLevel level, const char* file, int line) {
    return LogMessage(level, file, line, std::this_thread::get_id());
  }

  virtual void StopSoon() = 0;

 protected:
  // Implementation-specific logging function.
  virtual void LogMessageImpl(LogMessage&& msg) = 0;
};

namespace internal {

class Voidify {
 public:
  void operator&(std::ostream&) {}
};

}  // namespace internal

}  // namespace

#endif /* DBC3B62B_4E2A_49CB_8598_AFE1922E4916 */
