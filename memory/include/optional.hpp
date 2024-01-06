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
           typename std::enable_if<std::is_constructible<TType, TArgs...>>::type>
  Optional(TArgs&&... args) {
    this->data_ptr = CreateStackPtr(data_, std::forward(args)...)
  }

  // Moves |other| instance to this one if it supports a move ctor.
  template<typename std::enable_if<std::is_constructible<TType, TType&&>>::type>
  Optional(Optional<TType>&& other) {
    if (other.data_ptr_) {
      this->data_ptr = CreateStackPtr(this->data_, std::move(*other.data_ptr_));
      other.data_ptr_.reset();
    }
  }
  template<typename std::enable_if<std::is_constructible<TType, TType&&>>::type>
  Optional(Optional<TType>&& other) {
    if (other.data_ptr_) {
      this->data_ptr = CreateStackPtr(this->data_, std::move(*other.data_ptr_));
      other.data_ptr_.reset();
    }
    return *this;
  }
  template<typename std::enable_if<std::is_constructible<TType, TType&&>>::type>
  Optional(TType&& other) {
    this->data_ptr = CreateStackPtr(this->data_, std::move(other));
    return *this;
  }


  // Copies |other| to this one if it supports a copy ctor.
  template<typename std::enable_if<std::is_constructible<TType, const TType&>>::type>
  Optional(const Optional<TType>& other) {
    if (other.data_ptr_) {
      this->data_ptr = CreateStackPtr(this->data_, *other.data_ptr_);
    }
  }
  template<typename std::enable_if<std::is_constructible<TType, const TType&>>::type>
  Optional& operator=(const Optional<TType>& other) {
    if (other.data_ptr_) {
      this->data_ptr = CreateStackPtr(this->data_, *other.data_ptr_);
    }
    return *this;
  }
  template<typename std::enable_if<std::is_constructible<TType, const TType&>>::type>
  Optional& operator=(const TType& other) {
    this->data_ptr = CreateStackPtr(this->data_, *other.data_ptr_);
    return *this;
  }
  
  // Operators.
  operator bool() {
    return !!data_;
  }

  const TType* operator->() const {
    std::assert(!!data_);
    return data_.get();
  }

  TType* operator->() {
    std::assert(!!data_);
    return data_.get();
  }

  const TType& operator*() const & {
    std::assert(!!data_);
    return *data_;
  }

  TType& operator*() & {
    std::assert(!!data_);
    return *data_;
  }

  TType&& operator*() && {
    std::assert(!!data_);
  }

  bool has_value() const {
    return !!data_;
  }

  const TValue& value() const & {
    return *this;
  }

  TValue& value() & {
    return *this;
  }

  TValue&& value() && {
    return *this;
  }

 private:
  class StackDeleter {
   public:
    void operator()(TType* ptr) { ptr->~TType(); }
  };
  using StackPtr = std::unique_ptr<TType, StackDeleter>;

  uint8_t data_[sizeof(TType)];
  StackPtr data_ptr_;
};

}  // namespace util

#endif /* B654B613_A379_4ACC_AF2D_8F6CA201E044 */
