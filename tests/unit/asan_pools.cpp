#include "third_party/doctest/doctest.h"

#include "testutil.h"
#include "nettest.h"
#include "RedEngineFL/Interface/RedGlyphMemoryManager.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Util/RedStack.h"
#include "RedEngineFL/Util/ext/Allocator.h"
#include "RedEngineFL/Util/ext/ListAllocator.h"
#include "RedEngineFL/Util/ext/SubAllocator.h"
#include "Battlefront2/Source/LuaMemory.h"
#include "Battlefront2/Source/NetPacket.h"
#include "LuaFL/include/lua.h"
#include "SoundFL/source/sndfreelist.h"

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#endif

namespace
{

struct PoolElem
{
    char ac[16];
};

int DeepRaise(lua_State* L, int iDepth)
{
    volatile char acBuf[256];
    for (int i = 0; i < 256; ++i)
    {
        acBuf[i] = (char)(iDepth + i);
    }
    if (iDepth < 8)
    {
        return DeepRaise(L, iDepth + 1) + acBuf[iDepth];
    }
    lua_pushstring(L, "deep");
    return lua_error(L) + acBuf[0];
}

int RaiseDeep(lua_State* L)
{
    return DeepRaise(L, 0);
}

int StackHeavy()
{
    volatile char acBuf[4096];
    for (int i = 0; i < 4096; ++i)
    {
        acBuf[i] = (char)(i & 0x7f);
    }
    int iSum = 0;
    for (int i = 0; i < 4096; ++i)
    {
        iSum += acBuf[i];
    }
    return iSum;
}

}

TEST_CASE("red heap in bounds cycle is clean" * doctest::test_suite("regression"))
{
    const unsigned int uiSize = 13;
    char* p = (char*)RedAllocFromHeap(0, uiSize, 0);
    REQUIRE(p);
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        p[i] = (char)(i + 1);
    }
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        CHECK(p[i] == (char)(i + 1));
    }
#if defined(__SANITIZE_ADDRESS__)
    CHECK(!__asan_address_is_poisoned(p));
    CHECK(!__asan_address_is_poisoned(p + uiSize - 1));
    CHECK(__asan_address_is_poisoned(p + uiSize));
#endif
    RedFreeToHeap(0, p, 0);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p));
#endif
}

TEST_CASE("memory pool in bounds cycle is clean" * doctest::test_suite("regression"))
{
    const unsigned int uiSize = 13;
    MemoryPool pool("inbounds", 16);
    pool.Create(4, 16, 1);
    char* p = (char*)pool.Allocate(uiSize);
    REQUIRE(p);
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        p[i] = (char)(i + 1);
    }
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        CHECK(p[i] == (char)(i + 1));
    }
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p + uiSize));
#endif
    pool.Free(p);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p));
#endif
}

TEST_CASE("lua bin in bounds cycle is clean" * doctest::test_suite("regression"))
{
    const unsigned int uiSize = 13;
    LuaMemory::CreatePools(true);
    char* p = (char*)Lua_Red_realloc(0, 0, uiSize);
    REQUIRE(p);
    CHECK(LuaMemory::LocatePool(uiSize, p));
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        p[i] = (char)(i + 1);
    }
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        CHECK(p[i] == (char)(i + 1));
    }
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p + uiSize));
#endif
    Lua_Red_Free(p, uiSize);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p));
#endif
    LuaMemory::Reset();
}

TEST_CASE("net packet in bounds cycle is clean" * doctest::test_suite("regression"))
{
    NetTestReset();
    NetPacket* pPacket = NetPacket::Alloc(false);
    REQUIRE(pPacket);
    char* p = (char*)pPacket;
    for (unsigned int i = 0; i < sizeof(NetBigPacket); ++i)
    {
        p[i] = (char)i;
    }
    for (unsigned int i = 0; i < sizeof(NetBigPacket); ++i)
    {
        CHECK(p[i] == (char)i);
    }
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p + sizeof(NetBigPacket)));
#endif
    pPacket->Free();
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p));
#endif
    NetTestReset();
}

TEST_CASE("lua error from deep c frames is clean" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L);
    CHECK(lua_cpcall(L, RaiseDeep, 0) == LUA_ERRRUN);
    CHECK(StackHeavy() == 32 * 127 * 64);
    lua_close(L);
}

TEST_CASE("red stack in bounds cycle is clean" * doctest::test_suite("regression"))
{
    const int iSize = 13;
    const int iBase = RedStack::sMemIdx;
    char* p = 0;
    {
        RedStack stack;
        p = (char*)stack.Alloc(iSize);
        REQUIRE(p == &RedStack::sMem[iBase]);
        CHECK(RedStack::sMemIdx == iBase + 16);
        for (int i = 0; i < iSize; ++i)
        {
            p[i] = (char)(i + 1);
        }
        for (int i = 0; i < iSize; ++i)
        {
            CHECK(p[i] == (char)(i + 1));
        }
#if defined(__SANITIZE_ADDRESS__)
        CHECK(__asan_address_is_poisoned(p + iSize));
#endif
    }
    CHECK(RedStack::sMemIdx == iBase);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p));
#endif
}

TEST_CASE("stack allocator in bounds cycle is clean" * doctest::test_suite("regression"))
{
    const unsigned int uiSize = 13;
    void* pBlock = RedAllocFromHeap(0, 256, 0);
    REQUIRE(pBlock);
    {
        util::StackAllocator alloc(false, true, 16);
        util::Buffer pool(pBlock, 256);
        alloc.Set(pool);
        util::Buffer first(0, 32);
        alloc.Allocate(first);
        util::Buffer second(0, uiSize);
        alloc.Allocate(second);
        char* p = (char*)second.data_;
        REQUIRE(first.data_ == pBlock);
        REQUIRE(p == (char*)pBlock + 32);
        for (unsigned int i = 0; i < uiSize; ++i)
        {
            p[i] = (char)(i + 1);
        }
        for (unsigned int i = 0; i < uiSize; ++i)
        {
            CHECK(p[i] == (char)(i + 1));
        }
#if defined(__SANITIZE_ADDRESS__)
        CHECK(__asan_address_is_poisoned(p + uiSize));
#endif
        alloc.Deallocate(second);
        CHECK(!second.data_);
#if defined(__SANITIZE_ADDRESS__)
        CHECK(__asan_address_is_poisoned(p));
        CHECK(!__asan_address_is_poisoned(first.data_));
#endif
        alloc.Deallocate(first);
        CHECK(alloc.pos_ == pBlock);
    }
#if defined(__SANITIZE_ADDRESS__)
    CHECK(!__asan_address_is_poisoned((char*)pBlock + 255));
#endif
    RedFreeToHeap(0, pBlock, 0);
}

TEST_CASE("sub allocator in bounds cycle is clean" * doctest::test_suite("regression"))
{
    const unsigned int uiSize = 13;
    util::StaticSubAlloc<PoolElem> alloc;
    util::Buffer first(0, uiSize);
    alloc.Allocate(first);
    util::Buffer second(0, sizeof(PoolElem));
    alloc.Allocate(second);
    char* p = (char*)first.data_;
    char* q = (char*)second.data_;
    REQUIRE(p);
    REQUIRE(q);
    CHECK(p != q);
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        p[i] = (char)(i + 1);
    }
    for (unsigned int i = 0; i < sizeof(PoolElem); ++i)
    {
        q[i] = (char)i;
    }
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        CHECK(p[i] == (char)(i + 1));
    }
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p + uiSize));
    CHECK(!__asan_address_is_poisoned(q + sizeof(PoolElem) - 1));
#endif
    alloc.Deallocate(first);
    alloc.Deallocate(second);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p));
    CHECK(__asan_address_is_poisoned(q));
#endif
    util::Buffer again(0, sizeof(PoolElem));
    alloc.Allocate(again);
    CHECK(again.data_ == second.data_);
    alloc.Deallocate(again);
}

TEST_CASE("list allocator in bounds cycle is clean" * doctest::test_suite("regression"))
{
    const unsigned int uiSize = 13;
    util::ListAllocator* pAlloc = util::ListAllocator::Create(0x2000, 0x10000);
    REQUIRE(pAlloc);
    util::Buffer first(0, uiSize);
    pAlloc->Allocate(first);
    util::Buffer second(0, 40);
    pAlloc->Allocate(second);
    char* p = (char*)first.data_;
    char* q = (char*)second.data_;
    REQUIRE(p);
    REQUIRE(q);
    CHECK(first.size_ == 32);
    CHECK(second.size_ == 64);
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        p[i] = (char)(i + 1);
    }
    for (unsigned int i = 0; i < 40; ++i)
    {
        q[i] = (char)i;
    }
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        CHECK(p[i] == (char)(i + 1));
    }
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p + uiSize));
    CHECK(__asan_address_is_poisoned(q - 1));
    CHECK(__asan_address_is_poisoned(q + 40));
#endif
    pAlloc->Deallocate(first);
    pAlloc->Deallocate(second);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p));
    CHECK(__asan_address_is_poisoned(q));
#endif
    util::Buffer again(0, uiSize);
    pAlloc->Allocate(again);
    CHECK(again.data_ == p);
    pAlloc->Deallocate(again);
    util::Buffer mid(0, 0x1000);
    pAlloc->Allocate(mid);
    REQUIRE(mid.data_);
    util::Buffer big(0, 0x17f5);
    pAlloc->Allocate(big);
    char* r = (char*)big.data_;
    REQUIRE(r);
    CHECK(big.size_ >= 0x1800);
    r[0] = 1;
    r[0x17f4] = 2;
    CHECK(r[0x17f4] == 2);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(r + 0x17f5));
#endif
    pAlloc->Deallocate(big);
    pAlloc->Deallocate(mid);
    delete pAlloc;
}

TEST_CASE("snd free list in bounds cycle is clean" * doctest::test_suite("regression"))
{
    const unsigned int uiSize = 16;
    Snd::FreeList list(uiSize, 1, 8, 1);
    char* p = (char*)list.Alloc();
    char* q = (char*)list.Alloc();
    REQUIRE(p);
    REQUIRE(q);
    CHECK(p != q);
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        p[i] = (char)(i + 1);
        q[i] = (char)i;
    }
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        CHECK(p[i] == (char)(i + 1));
    }
#if defined(__SANITIZE_ADDRESS__)
    CHECK(!__asan_address_is_poisoned(q + uiSize - 1));
    CHECK(__asan_address_is_poisoned(q + uiSize));
#endif
    list.Free(p);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p));
    CHECK(!__asan_address_is_poisoned(q));
#endif
    char* again = (char*)list.Alloc();
    REQUIRE(again);
    for (unsigned int i = 0; i < uiSize; ++i)
    {
        again[i] = 0;
    }
    list.Free(again);
    list.Free(q);
    CHECK(list.mBlockList.IsEmpty());
}

TEST_CASE("glyph memory in bounds cycle is clean" * doctest::test_suite("regression"))
{
    const unsigned int uiBlockSize = sizeof(RedGlyphMemoryManager::VertexBlock);
    RedGlyphMemoryManager* pManager = new RedGlyphMemoryManager(24);
    char* p = (char*)(void*)pManager->Alloc(1);
    REQUIRE(p);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p + uiBlockSize));
#endif
    RedGlyphMemoryManager::Iterator it = pManager->Alloc(9);
    char* q = (char*)(void*)it;
    REQUIRE(q);
    CHECK(p != q);
    CHECK(it.GetBlockSize() == 48);
    unsigned int uiVertices = 0;
    for (RedGlyphMemoryManager::Iterator walk(q); walk; ++walk)
    {
        walk->x = (float)uiVertices;
        ++uiVertices;
    }
    CHECK(uiVertices == 54);
    for (unsigned int i = 0; i < sizeof(RedGlyphMemoryManager::VertexBlock::Vertices); ++i)
    {
        p[i] = (char)i;
    }
    CHECK((void*)pManager->Alloc(1) == (void*)0);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(!__asan_address_is_poisoned(p + uiBlockSize - 1));
#endif
    pManager->Free(p);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(p));
    CHECK(__asan_address_is_poisoned(p + uiBlockSize - 1));
    CHECK(!__asan_address_is_poisoned(q));
#endif
    char* again = (char*)(void*)pManager->Alloc(1);
    CHECK(again == p);
    pManager->Free(again);
    pManager->Free(it);
#if defined(__SANITIZE_ADDRESS__)
    CHECK(__asan_address_is_poisoned(q));
#endif
#if defined(__SANITIZE_ADDRESS__)
    __asan_unpoison_memory_region(pManager->m_pPool, 3 * uiBlockSize);
#endif
    RedFreeToHeap(__RedCurrHeap, pManager->m_pPool, 0);
    delete pManager;
}
