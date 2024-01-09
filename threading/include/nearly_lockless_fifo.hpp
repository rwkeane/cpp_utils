#ifndef F4036F42_4CE4_4B4C_95CD_C0E1E2533545
#define F4036F42_4CE4_4B4C_95CD_C0E1E2533545

#include <array>
#include <atomic>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "memory/include/optional.hpp"
#include "threading/parallel_circular_buffer.hpp"
#include "util/include/compiler_hints.hpp"

namespace util {

// This class defines a fully parallelized multi-producer multi-consumer
// "nearly-lockless" FIFO queue. Specifically, contention for a mutex may only
// occur when the underlying |data_| queue is full. For the expected use case
// where nowhere near |TFifoElementCount| elements are ever queued up at the
// same time, but the queue is never empty, this implementation should never
// lock a mutex. 
template<typename TDataType, size_t TFifoElementCount = 1024>
class NearlyLocklessFifo {
 public:
  // Use SFINAR to ensure this class can only be used with movable types.
	static_assert(std::is_move_constructible<TDataType>::value);

 	NearlyLocklessFifo() = default;
	~NearlyLocklessFifo() = default;

	NearlyLocklessFifo(const NearlyLocklessFifo& other) = delete;
	NearlyLocklessFifo(NearlyLocklessFifo&& other) = delete;

 	// Standard Queue operations.
	void Enqueue(TDataType&& data);
	Optional<TDataType> Dequeue();

	// Accessors
	bool is_empty() {
	  if (!data_.is_empty()) {
			return false;
		}

		std::unique_lock<std::mutex> lock(overflow_queue_lock_);
		return overflow_queue_.empty();
	}

 private:
	// Maintanance is expected to be performed regularly on the underlying queue.
	// Else, |TDataType|s may eventually stop flowing.
	bool queue_needs_maintanance() const;
	bool MaintainQueue();

	// Thread-safe accessors for |data_|.
 	bool TryPushToArray(TDataType& data);
 	virtual Optional<TDataType> TryPopFromArray();

 	// Queue of tasks to execute that don't fit in |data_|. Will be dequeued and
 	// pushed to |data_| once |data_| is only half-full.
 	//
 	// NOTE: Use Optional to support types that should or cannot be trivially
 	// constructed.
 	std::vector<Optional<TDataType>> overflow_queue_;
 	std::mutex overflow_queue_lock_;
 	std::atomic_bool is_overflow_queue_flushing_{false};
 	std::atomic_bool is_overflow_queue_in_use_{false};

  std::atomic_int32_t elements_written_so_far_{ 0 };

 	// Array backing the lockless FIFO used to store tasks.
 	ParallelCircularBuffer<TDataType, TFifoElementCount> data_;
};

template<typename TDataType, size_t TFifoElementCount>
void NearlyLocklessFifo<TDataType, TFifoElementCount>::Enqueue(
		TDataType&& data) {
	if (data_.TryEnqueue(data)) {
		return;
	}

	const int so_far = elements_written_so_far_.load(std::memory_order_relaxed);
	constexpr int check_interval = TFifoElementCount / 16;
	if (so_far % check_interval == 0) {
		MaintainQueue();
		
		if (data_.TryEnqueue(data)) {
			return;
		}
	}

	std::lock_guard<std::mutex> lock(overflow_queue_lock_);
	overflow_queue_.emplace_back(std::move(data));
}

template<typename TDataType, size_t TFifoElementCount>
Optional<TDataType> NearlyLocklessFifo<TDataType, TFifoElementCount>
		::Dequeue() {
	auto result = data_.Dequeue();
	if(!!result) {
		return result;
	}
	
	if (queue_needs_maintanance()) {
		MaintainQueue();
		return data_.Dequeue();
	}

	return result;
}

template<typename TDataType, size_t TFifoElementCount>
bool NearlyLocklessFifo<TDataType, TFifoElementCount>
		::queue_needs_maintanance() const {
	return is_overflow_queue_in_use_.load(std::memory_order_relaxed) &&
			!is_overflow_queue_flushing_.load(std::memory_order_relaxed);
}

template<typename TDataType, size_t TFifoElementCount>
bool NearlyLocklessFifo<TDataType, TFifoElementCount>::MaintainQueue() {
	if (!queue_needs_maintanance()) {
		return false;
	}

	if (is_overflow_queue_flushing_.exchange(true)) {
		return false;
	}

	if (UNLIKELY(!queue_needs_maintanance())) {
		return false;
	}

	// Perform all modifications into the queue outside of the mutex section to
	// boost performance.
	std::vector<Optional<TDataType>> local_overflow_queue;
	{
		std::unique_lock<std::mutex> lock(overflow_queue_lock_);
		overflow_queue_.swap(local_overflow_queue);
	}

	auto it = local_overflow_queue.begin();
	for (;it != local_overflow_queue.end(); it++) {
		assert(*it);
		if (!TryPushToArray(it->value())) {
			break;
		}
	}
	local_overflow_queue.erase(local_overflow_queue.begin(), it);
	
	// Now we have to go back and modify the original queue. First, handle any
	// new elements added to the overflow queue since the prior swap.
	std::unique_lock<std::mutex> second_lock(overflow_queue_lock_);

	auto it2 = overflow_queue_.begin();
	if (it == local_overflow_queue.end()) {
		for (;it2 != overflow_queue_.end(); it2++) {
			if (!TryPushToArray(it2->value())) {
				break;
			}
		}
	}

	// Add all remaining elements to the end of the local queue, then swap the
	// queues back.
	for (;it2 != overflow_queue_.end(); it2++) {
		local_overflow_queue.push_back(std::move(*it2));
	}
	overflow_queue_.swap(local_overflow_queue);

	if (overflow_queue_.empty()) {
		is_overflow_queue_in_use_.store(false, std::memory_order_relaxed);
	}
	is_overflow_queue_flushing_.store(false, std::memory_order_relaxed);

	return true;
}

}  // namespace util

#endif /* F4036F42_4CE4_4B4C_95CD_C0E1E2533545 */
