#include <stddef.h>
#include <stdint.h>

static void* (*s_alloc)(size_t);
static void* (*s_realloc)(void*, size_t);
static void (*s_free)(void*);

void ossf2_set_allocator(void* (*a)(size_t), void* (*r)(void*, size_t), void (*f)(void*))
{
    s_alloc = a;
    s_realloc = r;
    s_free = f;
}

static int ossf2_misaligned(const void* p)
{
    return ((uintptr_t)p & 15) != 0;
}

int ossf2_posix_memalign(void** pp, size_t align, size_t size)
{
    void* p;

    *pp = 0;
    if (!s_alloc || !s_free || align > 16)
    {
        return 12;
    }
    p = s_alloc(size);
    if (!p)
    {
        return 12;
    }
    if (ossf2_misaligned(p))
    {
        s_free(p);
        return 12;
    }
    *pp = p;
    return 0;
}

void* ossf2_realloc(void* ptr, size_t size)
{
    if (!s_realloc || !s_free)
    {
        return 0;
    }
    return s_realloc(ptr, size);
}

void ossf2_free(void* ptr)
{
    if (ptr && s_free)
    {
        s_free(ptr);
    }
}
