#ifndef CCE09B41_4016_4075_AB48_C0F1FD2ADB64
#define CCE09B41_4016_4075_AB48_C0F1FD2ADB64

#include <type_traits>
#include <utility>
#include <cstdint>

namespace util{

template<typename TType> class StackDeleter;

// This class defines a stack-owned unique_ptr instance, to be used in the
// implemnetation of Optional.
template<typename TType>
using StackPtr = std::unique_ptr<TType, StackDeleter<TType>>;

template<typename TType>
class StackDeleter {
 public:
  void operator()(TType* ptr) { ptr->~TType(); }
};

template<typename TType, typename... TArgs>
StackPtr<TType> CreateStackPtr(uint8_t storage[sizeof(TType)], TArgs&&... args) {
  return StackPtr<TType>(new (storage) TType(std::forward<TArgs>(args)...));
}

}  // namespace util

#endif /* CCE09B41_4016_4075_AB48_C0F1FD2ADB64 */
