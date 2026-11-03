#pragma once

#include <stddef.h>

#if defined(__SANITIZE_ADDRESS__)

#include <sanitizer/asan_interface.h>

#define PBL_ASAN_NO_SANITIZE __declspec(no_sanitize_address)
#define PBL_ASAN_NO_INLINE __declspec(noinline)

inline void PblAsanPoison(const volatile void* p, size_t n)
{
    ASAN_POISON_MEMORY_REGION(p, n);
}

inline void PblAsanUnpoison(const volatile void* p, size_t n)
{
    ASAN_UNPOISON_MEMORY_REGION(p, n);
}

inline void PblAsanNoReturn()
{
    __asan_handle_no_return();
}

#else

#define PBL_ASAN_NO_SANITIZE
#define PBL_ASAN_NO_INLINE

inline void PblAsanPoison(const volatile void*, size_t)
{
}

inline void PblAsanUnpoison(const volatile void*, size_t)
{
}

inline void PblAsanNoReturn()
{
}

#endif
