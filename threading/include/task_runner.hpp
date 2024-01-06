#ifndef B635B5F6_879E_41EA_B94D_F33B63F7240A
#define B635B5F6_879E_41EA_B94D_F33B63F7240A

#include <chrono>
#include <future>
#include <utility>

namespace util {

// A thread-safe API surface that allows for posting tasks. Posted tasks are
// expected to be dispatched to executing threads in the order in which they are
// posted via PostTask(). To phrase this differently, if A is posted to the
// TaskRunner before B, then A is expected to be dispatched to an executing
// thread before B.
// NOTE: This does NOT guarantee that if A was posted before B, then A will run
// before B if A and B are dispatched to different threads. 
class TaskRunner {
 public:
  using Task = std::packaged_task<void()>;
  using Timespan = std::chrono::milliseconds;

  virtual ~TaskRunner() = default;

  // Takes any callable target (function, lambda-expression, std::bind result,
  // etc.) that should be run at the first convenient time.
  template <typename Functor>
  inline void PostTask(Functor f) {
    PostPackagedTask(Task(std::move(f)));
  }

  // Takes any callable target (function, lambda-expression, std::bind result,
  // etc.) that should be run no sooner than |delay| time from now. Note that
  // the Task might run after an additional delay, especially under heavier
  // system load. There is no deadline concept.
  template <typename Functor>
  inline void PostTaskWithDelay(Functor f, Timespan delay) {
    PostPackagedTaskWithDelay(Task(std::move(f)), delay);
  }

  // Return true if the calling thread is a thread currently executing task
  // runner tasks.
  virtual bool IsRunningOnTaskRunner() const = 0;

 protected:
  // Implementations should provide the behavior explained in the comments above
  // for PostTask[WithDelay]().
  virtual void PostPackagedTask(Task task) = 0;
  virtual void PostPackagedTaskWithDelay(Task task, Timespan delay) = 0;
};

template <>
inline void TaskRunner::PostTask(Task f) {
  PostPackagedTask(std::move(f));
}

template<>
inline void TaskRunner::PostTaskWithDelay(Task task, Timespan delay) {
  PostPackagedTaskWithDelay(std::move(task), delay);
}

}  // namespace util

#endif /* B635B5F6_879E_41EA_B94D_F33B63F7240A */
