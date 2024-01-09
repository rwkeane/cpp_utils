#ifndef D56B62A1_1E8A_4102_B97A_345B93505BE1
#define D56B62A1_1E8A_4102_B97A_345B93505BE1

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <utility>
#include <vector>

#include "threading/include/nearly_lockless_fifo.hpp"
#include "threading/include/task_runner.hpp"

namespace util {

// High-performance implementation of TaskRunner for the use case of multiple
// producer threads and multiple consumer threads.
//
// Tasks are stored in a "nearly-lockless" FIFO of size |TFifoElementCount|,
// which is expected to never have contention for a mutex, while delayed tasks
// are protected by a mutex and regularly enqueued into the underlying |data_|
// FIFO.
template<size_t TFifoElementCount>
class MultithreadedTaskRunner : public TaskRunner {
 public:
 	MultithreadedTaskRunner();
 	~MultithreadedTaskRunner() override = default;
  
  MultithreadedTaskRunner(const MultithreadedTaskRunner& other) = delete;
  MultithreadedTaskRunner(MultithreadedTaskRunner&& other) = delete;
  MultithreadedTaskRunner operator=(const MultithreadedTaskRunner& other) = delete;
  MultithreadedTaskRunner operator=(MultithreadedTaskRunner&& other) = delete;

  virtual void LoopExecution();

	// TaskRunner implementation.
	void PostPackagedTask(Task task) final;
	void PostPackagedTaskWithDelay(Task task, Timespan delay) final;
	bool IsRunningOnTaskRunner() const override;

 private:
 	using DelayedTask =
 			std::pair<Task, std::chrono::time_point<std::chrono::system_clock>>;

	void EnqueDelayedTasks();
	bool TryExecuteTask();

	// Tracks what threads are currently being used by this TaskRunner.
	std::vector<std::thread::id> executing_threads_;
 	mutable std::mutex executing_threads_lock_;
 	std::atomic_bool is_running_{false};

	// Set of tasks posted with PostTaskWithDelay().
 	std::vector<DelayedTask> delayed_tasks_;
 	std::mutex delayed_tasks_lock_;

 	NearlyLocklessFifo<Task, TFifoElementCount> task_queue_;

	// Mutex used for the condition variable for waiting when no work is
	// available. In the expected case, this should never be used.
	std::mutex queue_empty_mutex_;
	std::condition_variable queue_empty_cv_;
};

template<size_t TFifoElementCount>
MultithreadedTaskRunner<TFifoElementCount>::MultithreadedTaskRunner() {
	static_assert(TFifoElementCount > size_t{16});

	PostTask([this]() {
		EnqueDelayedTasks();
	});
}

template<size_t TFifoElementCount>
void MultithreadedTaskRunner<TFifoElementCount>::LoopExecution() {
	const auto current_id = std::this_thread::get_id();

	{
		std::lock_guard<std::mutex> lock(executing_threads_lock_);
		assert(std::find_if(executing_threads_.begin(), executing_threads_.end(), 
				[current_id](const std::thread::id id) {
					return current_id == id;
				}) == executing_threads_.end());

		executing_threads_.push_back(current_id);
	}

	is_running_.store(true);
	while(is_running_.load()) {
		while (!TryExecuteTask()) {
			// NOTE: Do not use std::condition_variable as would introduce contention
			// for a mutex.
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}
	}

	{
		const auto current_id = std::this_thread::get_id();

		std::lock_guard<std::mutex> lock(executing_threads_lock_);
		auto it =
				std::find_if(
						executing_threads_.begin(), executing_threads_.end(), 
						[current_id](const std::thread::id id) {
							return current_id == id;
						});
		assert(it != executing_threads_.end());
		executing_threads_.erase(it);

		if (executing_threads_.empty()) {
			is_running_.store(false);
		}
	}
}

template<size_t TFifoElementCount>
bool MultithreadedTaskRunner<TFifoElementCount>::IsRunningOnTaskRunner() const {
	const auto current_id = std::this_thread::get_id();

	std::lock_guard<std::mutex> lock(executing_threads_lock_);
	return std::find_if(executing_threads_.begin(), executing_threads_.end(), 
				[current_id](const std::thread::id id) {
					return current_id == id;
				}) != executing_threads_.end();
}

template<size_t TFifoElementCount>
bool MultithreadedTaskRunner<TFifoElementCount>::TryExecuteTask() {
	auto task = task_queue_.Dequeue();
	if (!task) {
		return false;
	}

	task.value()();
	return true;
}

template<size_t TFifoElementCount>
void MultithreadedTaskRunner<TFifoElementCount>::PostPackagedTask(Task task) {
	task_queue_.Enqueue(std::move(task));
}

template<size_t TFifoElementCount>
void MultithreadedTaskRunner<TFifoElementCount>::PostPackagedTaskWithDelay(
		TaskRunner::Task task, TaskRunner::Timespan delay) {
	std::lock_guard<std::mutex> lock(delayed_tasks_lock_);
	delayed_tasks_.emplace_back(std::move(task),
															std::chrono::system_clock::now() + delay);
}

template<size_t TFifoElementCount>
void MultithreadedTaskRunner<TFifoElementCount>::EnqueDelayedTasks() {
	{
		std::lock_guard<std::mutex> lock(delayed_tasks_lock_);

		// NOTE: Do not use a condition_variable here or it could cause deadlock on
		// a single-threaded runtime.
		if (!delayed_tasks_.empty()) {
			auto time_now = std::chrono::system_clock::now();

			// Sort the tasks is deceasing delay time order.
			//
			// TODO: Manually sort these because we know the pre-existing tasks are
			// already sorted.
			std::sort(delayed_tasks_.rbegin(), delayed_tasks_.rend(),
					[](const DelayedTask& first, const DelayedTask& second) {
				return first.second < second.second;
			});

			// Post the tasks that have been waiting long enough. Because the elements
			// are sorted with the least timestamps at the end, the last element is
			// always the one removed so this operation is fast.
			auto it = delayed_tasks_.rbegin();
			for (; it != delayed_tasks_.rend(); it++) {
				if (it->second <= time_now) {
					Task& task = it->first;
					PostTask(std::move(task));
				}
			}
			delayed_tasks_.erase(it.base(), delayed_tasks_.end());
		}
	}

	// Re-run this task again soon. 
	// NOTE: Cannot be called "WithDelay" or the delayed tasks will never be
	// enqueued.
	PostTask([this]() {
		EnqueDelayedTasks();
	});
}

}  // namespace util

#endif /* D56B62A1_1E8A_4102_B97A_345B93505BE1 */
