#ifndef C6957D25_A8B8_49A2_AB97_EF6DCEB5EAE4
#define C6957D25_A8B8_49A2_AB97_EF6DCEB5EAE4

#include <atomic>
#include <memory>

#include "util/include/logger.hpp"

namespace util {

// Creates a new LoggerImpl.
template<typename TInfoStream, typename TErrorStream>
std::unique_ptr<Logger> CreateLogger(TInfoStream&& info_stream,
                                     TErrorStream&& error_stream) {
  return std::unique_ptr<LoggerImpl<TInfoStream, TErrorStream>>(
      new LoggerImpl<LoggerImpl<TInfoStream, TErrorStream>>(
          std::forward(info_stream), std::forward(error_stream)));
}

// Implementation of Logger. Reads log messages on any thread, then queues them
// up in a thread-safe queue. When available the logger dequeues them and writes
// to the provided streams from a dedicated thread.
template<typename TInfoStream, typename TErrorStream>
class LoggerImpl : public Logger {
 public:
  LoggerImpl(TInfoStream&& info_stream, TErrorStream&& error_stream) : 
      info_stream_(std::forward(info_stream)),
      error_stream_(std::forward(error_stream)),
      logging_thread_([this]() { this->ReadAll(); }) {}

  LoggerImpl(LoggerImpl&& other) = delete;
  LoggerImpl(const LoggerImpl& other) = delete;

  // Dtor blocks on completing reading of all queued messages.
  ~LoggerImpl() {
    StopSoon();
    std::unique_lock<std::mutex> lock(should_stop_mutex_);
    can_read_.wait();
  }

  // Causes the logger to exit once all messages have been read.
  void StopSoon() {
    should_stop_.store(true);
  }
  
 private:
  // Reads all messages from the queue, waiting for more when the queue is
  // empty.
  ReadAll() {
    while (!should_stop_.load()) {
      log_messages_.MaintainQueue();
      {
        std::unique_lock<std::mutex> lock(can_read_mutex_);
        while(log_messages_.is_empty()) {
          can_read_.wait();
        }
      }


      for(auto message = log_messages_.Dequeue();
          !!message;
          message = log_messages_.Dequeue()) {
        auto& msg = message.value();
        if (msg.level() <= Logger::LogLevel::kInfo) {
          WriteLog(info_stream_, msg);
        } else {
          WriteLog(error_stream_, msg);
        }
      }
    }

    is_running_.store(false);

    std::unique_lock<std::mutex> lock(can_read_mutex_);
    should_stop_mutex_.notify_all();
  }

  template<typename TStream>
  void WriteLog(LogMessage& msg, TStream& stream) {
    stream << "[" << level << ":" << msg.file() << "(" << msg.line() << "): "
           << msg.thread_id() << "] " << message.stream().rdbuf() << '\n';
  }

  void LogMessageImpl(LogMessage& message) override {
    log_messages_.Enqueue(message);

    can_read_.notify_one();
  }

  std::atomic_bool is_running_{ true };

  NearlyLocklessFifo<LogMessage> log_messages_;

  // Streams for writing logs.
  TInfoStream info_stream_;
  TErrorStream error_stream_;

  // Tracks if any messages are available to be read.
  std::mutex can_read_mutex_;
  std::condition_variable can_read_;

  // Control stopping the logger.
  std::atomic_bool should_stop_{ false };
  std::mutex should_stop_mutex_;
  std::condition_variable should_stop_;

  // Thread running ReadAll(), as created in the ctor.
  const std::thread logging_thread_;
};

}  // namespace

#endif /* C6957D25_A8B8_49A2_AB97_EF6DCEB5EAE4 */
