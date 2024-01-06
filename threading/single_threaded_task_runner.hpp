#ifndef B877E41A_01C3_484E_A139_4A515868FBB3
#define B877E41A_01C3_484E_A139_4A515868FBB3

#include "threading/multithreaded_task_runner.hpp"

namespace util {

// A TaskRunner implementation for a single consumer thread and multiple
// producer threads.
//
// TODO: Write a better optimized version for a single thread consumer that
// uses less thread synchronization and blocks when no tasks as available
// instead of sleeping.
template<size_t TFifoElementCount>
class SingleThreadedTaskRunner : public MultithreadedTaskRunner {
 public:
  SingleThreadedTaskRunner();
  ~SingleThreadedTaskRunner() override = default;
  
  SingleThreadedTaskRunner(const SingleThreadedTaskRunner& other) = delete;
  SingleThreadedTaskRunner(SingleThreadedTaskRunner&& other) = delete;
  SingleThreadedTaskRunner=(const SingleThreadedTaskRunner& other) = delete;
  SingleThreadedTaskRunner=(SingleThreadedTaskRunner&& other) = delete;

  void LoopExecution() override {
    assert(running_thread_id_.load() == std::thread::id{});

    running_thread_id_.store(std::this_thread::get_id());
    MultithreadedTaskRunner<TFifoElementCount>::LoopExecution();
  }

  bool IsRunningOnTaskRunner() const override {
    return std::this_thread::get_id() == running_thread_id_.load();
  }

 private:
  std::atomic<std::thread::id> running_thread_id_;
};

}  // namespace util

#endif /* B877E41A_01C3_484E_A139_4A515868FBB3 */
