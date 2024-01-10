#ifndef B654B613_A379_4ACC_AF2D_8F6CA201E044
#define B654B613_A379_4ACC_AF2D_8F6CA201E044

#include <cstdint>
#include <type_traits>
#include <utility>

#include "memory/stack_ptr.hpp"

namespace util{

struct nullopt_t {
  // It must not be default-constructible to avoid ambiguity for opt = {}.
  explicit constexpr nullopt_t(bool) noexcept {}
};

constexpr nullopt_t nullopt(true);

// This class defines an optional data type for use when a value may or may not
// be available. This class is intended for STACK ALLOCATED DATA ONLY. For heap
// allocated data, use an std::unique_ptr instead.
//
// TODO: Ensure that the copy / move ctors are properly deleted when the
// underlying types don't support it.
template<typename TType>
class Optional {
 public:
  // Constructors of various types.
  Optional() noexcept = default;

  Optional(nullopt_t nullopt) {}

  // Constructs the underlying data TType using |args|.
  //
  // NOTE: This should work as a copy / move ctor for the underlying data if the
  // type supports it.
  template<typename... TArgs,
           typename = typename std::enable_if<
               std::is_constructible<TType, TArgs...>::value, bool>::type>
  Optional(TArgs&&... args) {
    data_ptr_ = CreateStackPtr<TType>(data_, std::forward(args)...);
  }

  // Moves |other| instance to this one if it supports a move ctor.
  template<typename U = TType,
           typename = typename std::enable_if<
               std::is_constructible<TType, U&&>::value, bool>::type>
  Optional(Optional<U>&& other) {
    if (other.data_ptr_) {
      data_ptr_ = CreateStackPtr<TType>(data_, std::move(*other.data_ptr_));
      other.data_ptr_.reset();
    }
  }
  template<typename U = TType,
           typename = typename std::enable_if<
               std::is_constructible<TType, U&&>::value, bool>::type>
  Optional& operator=(Optional<U>&& other) {
    if (other.data_ptr_) {
      data_ptr_ = CreateStackPtr<TType>(data_, std::move(*other.data_ptr_));
      other.data_ptr_.reset();
    }
    return *this;
  }
  template<typename U = TType,
           typename = typename std::enable_if<
               std::is_constructible<TType, U&&>::value &&
               std::is_rvalue_reference<U&&>::value, bool>::type>
  Optional(U&& other) {
    data_ptr_ = CreateStackPtr<TType, U>(data_, std::move(other));
  }

  // Copies |other| to this one if it supports a copy ctor.
  template<typename U = TType,
           typename = typename std::enable_if<
               std::is_constructible<TType, const U&>::value, bool>::type>
  Optional(const Optional<TType>& other) {
    if (other.data_ptr_) {
      data_ptr_ = CreateStackPtr<TType>(data_, *other.data_ptr_);
    }
  }
  template<typename U = TType,
           typename = typename std::enable_if<
               std::is_constructible<TType, const U&>::value, bool>::type>
  Optional& operator=(const Optional<TType>& other) {
    if (other.data_ptr_) {
      data_ptr_ = CreateStackPtr<TType>(data_, *other.data_ptr_);
    }
    return *this;
  }
  template<typename U = TType,
           typename = typename std::enable_if<
               std::is_constructible<TType, const U&>::value, bool>::type>
  Optional& operator=(const TType& other) {
    data_ptr_ = CreateStackPtr<TType>(data_, *other.data_ptr_);
    return *this;
  }
  
  // Operators.
  operator bool() {
    return !!data_ptr_;
  }

  const TType* operator->() const {
    assert(!!data_ptr_);
    return data_ptr_.get();
  }

  TType* operator->() {
    assert(!!data_);
    return data_ptr_.get();
  }

  const TType& operator*() const & {
    assert(!!data_ptr_);
    return *data_ptr_;
  }

  TType& operator*() & {
    assert(!!data_ptr_);
    return *data_ptr_;
  }

  TType&& operator*() && {
    assert(!!data_ptr_);
    return *data_ptr_;
  }

  bool has_value() const {
    return !!data_ptr_;
  }

  const TType& value() const & {
    return *data_ptr_;
  }

  TType& value() & {
    return *data_ptr_;
  }

  TType&& value() && {
    return *data_ptr_;
  }

  void reset() {
    data_ptr_.reset();
  }

 private:
  uint8_t data_[sizeof(TType)];
  StackPtr<TType> data_ptr_;
};

}  // namespace util

#endif /* B654B613_A379_4ACC_AF2D_8F6CA201E044 */
