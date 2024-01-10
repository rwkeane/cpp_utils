#ifndef FE182BF4_98A4_4BFF_A9FD_FBE612C21B38
#define FE182BF4_98A4_4BFF_A9FD_FBE612C21B38

// Hacky version of C++ 20's [[unlikely]]
#if !defined(UNLIKELY)
#if defined(__clang__) || defined(COMPILER_GCC)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define UNLIKELY(x) (x)
#endif
#endif

// Hacky version of C++ 20's [[likely]]
#if !defined(LIKELY)
#if defined(__clang__) || defined(COMPILER_GCC)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#else
#define LIKELY(x) (x)
#endif
#endif

#endif /* FE182BF4_98A4_4BFF_A9FD_FBE612C21B38 */
