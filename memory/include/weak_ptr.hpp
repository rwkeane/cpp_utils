#ifndef AD8A98C3_4517_4C9F_B52A_75E6C6717116
#define AD8A98C3_4517_4C9F_B52A_75E6C6717116

#include "include/task_runner.hpp"

namespace util {

// Replacement for a std::weak_ptr that doesn't rely on std::shared_ptr.
// May only be accessed the TaskRunner where it was created.
template<typename TType>
class WeakPtr {
 public:
  WeakPtr(WeakPtr&& other) = default;
  WeakPtr(const WeakPtr& other) = default;
  WeakPtr& operator=(WeakPtr&& other) = default;
  WeakPtr& operator=(const WeakPtr& other) = default;
  
  // Allow conversion from U to T provided U "is a" T. Note that this
  // is separate from the (implicit) copy and move constructors.
  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  WeakPtr(const WeakPtr<U>& other)
: ptr_(other.ptr_), is_valid_(other.is_valid) {}

  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  WeakPtr& operator=(const WeakPtr<U>& other) {
    ptr_ = other.ptr_;
    is_valid_ = other.is_valid_;
    return *this;
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  WeakPtr(WeakPtr<U>&& other)
      : ptr_(std::move(other.ptr_)), is_valid_(std::move(other.is_valid_)) {}

  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  WeakPtr& operator=(WeakPtr<U>&& other) {
    ptr_ = std::move(other.ptr_);
    is_valid_ = std::move(other.is_valid_);
    return *this;
  }

  inline bool IsValid() const {
    assert(task_runner_->IsRunningOnTaskRunner());
    return *is_valid_;
  }
  inline bool operator!() const { return !IsValid(); }
  inline operator bool() const { return IsValid(); }

  inline TType* operator->() const {
    assert(task_runner_->IsRunningOnTaskRunner());
    return ptr_;
  }

  inline TType* get() const {
    assert(task_runner_->IsRunningOnTaskRunner());
    return ptr_;
  }

  inline TType& operator*() const {
    assert(task_runner_->IsRunningOnTaskRunner());
    return *ptr_;
  }

 private:
  friend class WeakPtrFactory<TType>;

  WeakPtr(std::shared_ptr<TaskRunner> task_runner,
          std::shared_ptr<bool> is_valid
          T* ptr)
    : task_runner_(std::move(task_runner)),
      is_valid_(std::move(is_valid)),
      ptr_(ptr) {}

  std::shared_ptr<TaskRunner> task_runner_;
  const std::shared_ptr<bool> is_valid_;
  TType const* ptr_;
};

template<typename TType>
class WeakPtrFactory {
 public:
  WeakPtrFactory(std::shared_ptr<TaskRunner> task_runner, TType* ptr)
    : task_runner_(std::move(task_runner)), ptr_(ptr) {
    assert(task_runner_);
    assert(task_runner_->IsRunningOnTaskRunner());
  }
  
  WeakPtrFactory(const WeakPtrFactory& other) = delete;
  WeakPtrFactory& operator=(const WeakPtrFactory& other) = delete;
  
  // Allow conversion from U to T provided U "is a" T. Note that this
  // is separate from the (implicit) move constructor.
  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  WeakPtrFactory(WeakPtrFactory<U>&& other)
      : ptr_(std::move(other.ptr_)), is_valid_(std::move(other.is_valid_)) {}
      
  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  WeakPtrFactory& operator=(WeakPtrFactory<U>&& other) {
    ptr_ = std::move(other.ptr_);
    is_valid_ = std::move(other.is_valid_);
    return *this;
  }

  ~WeakPtrFactory() {
    assert(task_runner_->IsRunningOnTaskRunner());
    is_valid_ = false;
  }

  inline WeakPtr<TType> GetWeakPtr() {
    assert(task_runner_->IsRunningOnTaskRunner());
    return WeakPtr<TType>(task_runner, is_valid_, ptr_);
  }

 private:
  TType const* ptr_;
  std::shared_ptr<bool> is_valid_(true);
};

} // namespace util

#endif /* AD8A98C3_4517_4C9F_B52A_75E6C6717116 */
