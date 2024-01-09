#ifndef C6957D25_A8B8_49A2_AB97_EF6DCEB5EAE4
#define C6957D25_A8B8_49A2_AB97_EF6DCEB5EAE4

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include "threading/include/nearly_lockless_fifo.hpp"
#include "util/include/logger.hpp"

namespace util {

// Creates a new LoggerImpl.
template<typename TInfoStream, typename TErrorStream>
std::unique_ptr<Logger> CreateLogger(TInfoStream&& info_stream,
                                     TErrorStream&& error_stream);

// Implementation of Logger. Reads log messages on any thread, then queues them
// up in a thread-safe queue. When available the logger dequeues them and writes
// to the provided streams from a dedicated thread created in the class's ctor.
template<typename TInfoStream, typename TErrorStream>
class LoggerImpl : public Logger {
 public:
  LoggerImpl(TInfoStream& info_stream, TErrorStream& error_stream)
    : info_stream_(info_stream),
      error_stream_(error_stream),
      logging_thread_([this]() { this->ReadAll(); }) {}

  LoggerImpl(LoggerImpl&& other) = delete;
  LoggerImpl(const LoggerImpl& other) = delete;

  // Dtor blocks on completing reading of all queued messages.
  ~LoggerImpl() override {
    StopSoon();

    std::unique_lock<std::mutex> lock(should_stop_mutex_);
    while(is_running_.load() == true) {
      should_stop_cv_.wait(lock);
    }
  }

  // Causes the logger to exit once all messages have been read.
  void StopSoon() override {
    should_stop_.store(true);
  }
  
 private:
  // Reads all messages from the queue, waiting for more when the queue is
  // empty.
  void ReadAll() {
    while (!should_stop_.load()) {
      if (log_messages_.is_empty()) {
        // NOTE: It is possible for "deadlock" to occur here if the queue
        // becomes non-empty after the is_empty() check and before the
        // wait_for() call. In practice though, because this is a logger,
        // waiting up to 10 ms for the timer to run out is not a "bad" outcome,
        // as it will not affect the execution threads. This could be avoided by
        // locking |can_read_mutex_| prior to the can_read_.notify_one() call in
        // LogMessageImpl(), but that introduces locking on the producer side,
        // which is worse than a few extra ms delay in logging (in an edge
        // case).
        std::unique_lock<std::mutex> lock(can_read_mutex_);
        while(log_messages_.is_empty()) {
          can_read_.wait_for(lock, std::chrono::milliseconds(10));
        }
      }

      for(auto message = log_messages_.Dequeue();
          !!message;
          message = log_messages_.Dequeue()) {
        auto& msg = message.value();
        if (msg.level() <= Logger::LogLevel::kInfo) {
          WriteLog(msg, info_stream_);
        } else {
          WriteLog(msg, error_stream_);
        }
      }
    }

    is_running_.store(false);

    std::unique_lock<std::mutex> lock(can_read_mutex_);
    should_stop_cv_.notify_all();
  }

  template<typename TStream>
  void WriteLog(LogMessage& msg, TStream& stream) {
    stream << "[" << msg.level() << ":" << msg.file() << "(" << msg.line()
           << "):" << msg.thread_id() << "] " << msg.stream().rdbuf() << '\n';
    msg.IsDoneLogging();
  }

  void LogMessageImpl(LogMessage&& message) override {
    log_messages_.Enqueue(std::move(message));

    can_read_.notify_one();
  }

  std::atomic_bool is_running_{ true };

  NearlyLocklessFifo<LogMessage> log_messages_;

  // Streams for writing logs.
  TInfoStream& info_stream_;
  TErrorStream& error_stream_;

  // Tracks if any messages are available to be read.
  std::mutex can_read_mutex_;
  std::condition_variable can_read_;

  // Control stopping the logger.
  std::atomic_bool should_stop_{ false };
  std::mutex should_stop_mutex_;
  std::condition_variable should_stop_cv_;

  // Thread running ReadAll(), as created in the ctor.
  const std::thread logging_thread_;
};

template<typename TInfoStream, typename TErrorStream>
class LoggerImplWithOwnership : public Logger {
 public:
  template<typename TFirst, typename TSecond>
  LoggerImplWithOwnership(TFirst&& first, TSecond&& second) 
    : info_stream_(std::forward(first)),
      error_stream_(std::forward(second)),
      logger_impl_(info_stream_, error_stream_) {}

 private:
  inline void StopSoon() override {
    logger_impl_.StopSoon();
  }

  inline void LogMessageImpl(LogMessage&& msg) override {
    logger_impl_.LogMessageImpl(std::move(msg));
  } 

  TInfoStream info_stream_;
  TErrorStream error_stream_;

  LoggerImpl<TInfoStream, TErrorStream> logger_impl_;
};

template<typename TInfoStream, typename TErrorStream>
std::unique_ptr<Logger> CreateLoggerImpl(
    TInfoStream&& info_stream, TErrorStream&& error_stream) {
  // Force SFINAE
  static_assert(std::is_move_constructible<TInfoStream>::value ||
                std::is_copy_constructible<TInfoStream>::value);
  static_assert(std::is_move_constructible<TErrorStream>::value ||
                std::is_copy_constructible<TErrorStream>::value);

  return std::unique_ptr<Logger>(
      new LoggerImplWithOwnership<
          std::decay<TInfoStream>, std::decay<TErrorStream>>(
              std::forward<TInfoStream>(info_stream),
              std::forward<TErrorStream>(error_stream)));
}

template<typename TInfoStream, typename TErrorStream>
std::unique_ptr<Logger> CreateLoggerImpl(
    TInfoStream& info_stream, TErrorStream& error_stream) {
  // Force SFINAE
  static_assert(!std::is_move_constructible<TInfoStream>::value &&
                !std::is_copy_constructible<TInfoStream>::value &&
                !std::is_move_constructible<TErrorStream>::value &&
                !std::is_copy_constructible<TErrorStream>::value);
  return std::unique_ptr<Logger>(
      new LoggerImpl<TInfoStream, TErrorStream>(info_stream, error_stream));
}

template<typename TInfoStream, typename TErrorStream>
std::unique_ptr<Logger> CreateLogger(TInfoStream&& info_stream,
                                     TErrorStream&& error_stream) {
  return CreateLoggerImpl(std::forward<TInfoStream>(info_stream),
                          std::forward<TErrorStream>(error_stream));
}

}  // namespace

#endif /* C6957D25_A8B8_49A2_AB97_EF6DCEB5EAE4 */
