#ifndef D5AB2FA6_BE5C_4404_BC22_2A4FC8636FDE
#define D5AB2FA6_BE5C_4404_BC22_2A4FC8636FDE

#include "include/task_runner.hpp"
#include "threading/multithreaded_task_runner.hpp"
#include "threading/single_threaded_task_runner.hpp"

namespace util {

template<size_t TFifoElementCount = size_t{1024}>
std::shared_ptr<TaskRunner> CreateSingleThreadedTaskRunner() {
  return std::make_shared<SingleThreadedTaskRunner<TFifoElementCount>>();
}

template<size_t TFifoElementCount = size_t{1024}>
std::shared_ptr<TaskRunner> CreateMultithreadedTaskRunner(int threads) {
  if (threads <= 0) {
    return nullptr;
  }

  auto task_runner =
      std::make_shared<MultithreadedTaskRunner<TFifoElementCount>>();
  for (int i = 0; i < threads; i++) {
    std::thread thread([task_runner]() {
      task_runner.LoopExecution();
    });
    thread.detach();
  }

  return task_runner;
}

}  // namespace util

#endif /* D5AB2FA6_BE5C_4404_BC22_2A4FC8636FDE */
