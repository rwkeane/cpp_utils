#ifndef A945298F_4894_40C9_A195_222F7928487B
#define A945298F_4894_40C9_A195_222F7928487B

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>

#include "memory/include/optional.hpp"
#include "util/include/compiler_hints.hpp"

namespace util {

// This class defines a fully lockless multi-producer, multi-consumer circular
// buffer. After construction, TryEnqueue() and Dequeue() may be called from any
// thread.
//
// NOTE: If more than 1 producer or consumer call functions of this class
// simultaneously, the order of execution of these calls is not guaranteed. It
// is guaranteed that if elements A and B are pushed to the buffer by some
// thread X and read by some thread T, A will be read before B. In other words,
// if this class is used with a single producer and single consumer, it will
// behave exactly as a "normal" FIFO queue.
template<typename TDataType, size_t TFifoElementCount = 1024>
class ParallelCircularBuffer{
 public:
  // This class can only be used with movable types.
	static_assert(std::is_move_constructible<TDataType>::value);
  static_assert(TFifoElementCount >= size_t{2});

 	explicit ParallelCircularBuffer() {
    for (size_t i = 1; i < data_.size() - 1; i++) {
      data_[i].SetLinkedDatas(&data_[i - 1], &data_[i + 1]);
    }
    Data* first = &data_[0];
    Data* last = &data_[TFifoElementCount - 1];

    first->SetLinkedDatas(last, &data_[1]);
    last->SetLinkedDatas(&data_[TFifoElementCount - 2], first);
  }

	~ParallelCircularBuffer() = default;

	ParallelCircularBuffer(const ParallelCircularBuffer& other) = delete;
	ParallelCircularBuffer(ParallelCircularBuffer&& other) = delete;

 	// Tries to enqueue |data|, taking ownership of |data| and returning true on
  // success and returning false on failure while leaving |data| unchanged.
	bool TryEnqueue(TDataType& data);

  // Retrieves the next available queue item, if one exists.
	Optional<TDataType> Dequeue();

	bool is_empty() const {
		auto result = remaining_elements_.load(std::memory_order_relaxed) == 0;
    assert(result > 0);
    return result;
	}

 private:
  // Helper to simplify ownership details on access to Data class below.
  template<typename TDataClass>
  class DataWrapper {
   public:
    TDataClass* TakeOwnership();

    bool is_written_to() const {
      return is_written_to_.load(std::memory_order_relaxed);
    }

   protected:
    void SetWritable() {
      is_written_to_.store(false, std::memory_order_relaxed);
    }

   private:
    std::atomic_bool is_written_to_{ false };
  };

	// Thread-safe wrapper around TDataType. It's expected that |data_lock_| will
	// only be locked in edge cases where the queue is near-empty, at which point
	// the performance hit of locking a mutex will be insignificant.
	class Data : public DataWrapper<Data> {
	 public:
    Data() = default;
    Data(const Data& other) = delete;
    Data(Data&& other) = delete;

	 	void StoreData(TDataType&& data);
	 	Optional<TDataType> TakeData();

    void SetLinkedDatas(Data* last, Data* next);

    Data* next() { return next_ptr_; }
    Data* previous() { return last_ptr_; }

    bool is_readable() const {
      return is_readable_.load(std::memory_order_relaxed);
    }
	 private:
		std::atomic_bool is_readable_{ false };
		Optional<TDataType> data_;

    Data* next_ptr_ = nullptr;
    Data* last_ptr_ = nullptr;
	};

 	// Array backing the lockless FIFO used to store tasks.
 	std::array<Data, TFifoElementCount> data_;

  // The next element for which reading has yet to complete (i.e. it is either
  // unread or reading is in progress).
  std::atomic<Data*> read_element_{ &data_[0] };
  
  // The current element to be written to (i.e. either writing has not yet
  // begun or writing is in progress).
  std::atomic<Data*> write_element_{ &data_[0] };

  // Tracks the number of elements in the queue.
  std::atomic_int32_t remaining_elements_{ 0 };
};

template<typename TDataType, size_t TFifoElementCount>
template<typename TDataClass>
TDataClass* ParallelCircularBuffer<TDataType, TFifoElementCount>::DataWrapper<TDataClass>
		::TakeOwnership() {
  bool expected = false;
  if (!is_written_to_.compare_exchange_strong(expected, true,
      std::memory_order_relaxed, std::memory_order_relaxed)) {
    return nullptr;
  }

  return static_cast<TDataClass*>(this);
} 

template<typename TDataType, size_t TFifoElementCount>
void ParallelCircularBuffer<TDataType, TFifoElementCount>::Data
		::StoreData(TDataType&& data) {
  assert(!is_readable_.load(std::memory_order_relaxed));

	data_ = std::move(data);
  is_readable_.store(true, std::memory_order_relaxed);
}

template<typename TDataType, size_t TFifoElementCount>
Optional<TDataType> ParallelCircularBuffer<TDataType, TFifoElementCount>
		::Data::TakeData() {
  bool expected_readable = true;
	if (!is_readable_.compare_exchange_strong(expected_readable, false,
          std::memory_order_relaxed, std::memory_order_relaxed)) {
    return nullopt;
  }

  auto result = std::move(data_);
  this->SetWritable();
  return result;
}

template<typename TDataType, size_t TFifoElementCount>
bool ParallelCircularBuffer<TDataType, TFifoElementCount>::TryEnqueue(
    TDataType& data) {
	// Try to enqueue it, skipping slots that are filled. If it hits the _last_
  // one being read, return empty.
  Data* local_read = read_element_.load(std::memory_order_relaxed);
  Data* local_write = write_element_.load(std::memory_order_relaxed);
  
  for (Data* current_ptr = local_write;
       current_ptr != local_read;
       current_ptr = current_ptr->next()) {
    // Gets the next available read-able data.
    auto* new_location = current_ptr->TakeOwnership();
    if (!!new_location) {
      new_location->StoreData(std::move(data));

      // Iteratively update |read_element_|, swapping it with
      // |local_read->next()| until |local_read| is out of sync with
      // |read_element| (meaning some other thread got here first) or it's
      // updated all the way to the current write ptr.
      while(!local_write->is_written_to()) {
        if (!read_element_.compare_exchange_strong(
            local_write, local_write->next(), std::memory_order_relaxed,
            std::memory_order_relaxed)) {
              break;
        }
      }
      
      remaining_elements_.fetch_add(1, std::memory_order_relaxed);
      return true;
    }
  }

  // Try again more elements have been written since this function started.
  if (local_read != read_element_.load(std::memory_order_relaxed)) {
    return TryEnqueue(data);
  }

  return false;
}

template<typename TDataType, size_t TFifoElementCount>
Optional<TDataType> ParallelCircularBuffer<TDataType, TFifoElementCount>
		::Dequeue() {
  Data* local_read = read_element_.load(std::memory_order_relaxed);
  Data* local_write = write_element_.load(std::memory_order_relaxed);
  
  for (Data* current_ptr = local_read;
       current_ptr->is_readable();
       current_ptr = current_ptr->next()) {
    // Gets the next available read-able data.
    auto data = current_ptr->TakeData();
    if (!!data) {
      // Iteratively update |read_element_|, swapping it with
      // |local_read->next()| until |local_read| is out of sync with
      // |read_element| (meaning some other thread got here first) or it's
      // updated all the way to the current write ptr.
      while(!local_read->is_readable()) {
        if (!read_element_.compare_exchange_strong(
            local_read, local_read->next(), std::memory_order_relaxed,
            std::memory_order_relaxed)) {
              break;
        }
      }

      remaining_elements_.fetch_sub(1, std::memory_order_relaxed);
      return data;
    }
  }

  // Try again more elements have been written since this function started.
  if (local_write != write_element_.load(std::memory_order_relaxed)) {
    return Dequeue();
  }

  return nullopt;
}

}  // namespace util

#endif /* A945298F_4894_40C9_A195_222F7928487B */
