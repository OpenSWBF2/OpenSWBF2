#include "third_party/doctest/doctest.h"

#include <stdlib.h>

#include "RedEngineFL/Interface/RedGlyphMemoryManager.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Util/RedStack.h"
#include "RedEngineFL/Util/ext/Allocator.h"
#include "RedEngineFL/Util/ext/ListAllocator.h"
#include "RedEngineFL/Util/ext/SubAllocator.h"
#include "Battlefront2/Source/LuaMemory.h"
#include "Battlefront2/Source/NetPacket.h"
#include "SoundFL/source/sndfreelist.h"

#if defined(__SANITIZE_ADDRESS__)
namespace
{

struct CanaryElem
{
    char ac[16];
};

}

TEST_CASE("asan canary heap overflow is reported" * doctest::test_suite("asan_canary"))
{
    volatile char* p = (volatile char*)malloc(8);
    p[8] = 1;
    free((void*)p);
}

TEST_CASE("asan canary red heap overflow is reported" * doctest::test_suite("asan_canary"))
{
    volatile char* p = (volatile char*)RedAllocFromHeap(__RedCurrHeap, 13, 0);
    REQUIRE(p);
    p[13] = 1;
    RedFreeToHeap(__RedCurrHeap, (void*)p, 0);
}

TEST_CASE("asan canary red heap use after free is reported" * doctest::test_suite("asan_canary"))
{
    volatile char* p = (volatile char*)RedAllocFromHeap(__RedCurrHeap, 16, 0);
    REQUIRE(p);
    RedFreeToHeap(__RedCurrHeap, (void*)p, 0);
    p[0] = 1;
}

TEST_CASE("asan canary memory pool overflow is reported" * doctest::test_suite("asan_canary"))
{
    MemoryPool pool("canary", 16);
    pool.Create(4, 16, 1);
    volatile char* p = (volatile char*)pool.Allocate(13);
    REQUIRE(p);
    p[13] = 1;
    pool.Free((void*)p);
}

TEST_CASE("asan canary memory pool use after free is reported" * doctest::test_suite("asan_canary"))
{
    MemoryPool pool("canary", 16);
    pool.Create(4, 16, 1);
    volatile char* p = (volatile char*)pool.Allocate(16);
    REQUIRE(p);
    pool.Free((void*)p);
    p[0] = 1;
}

TEST_CASE("asan canary lua bin overflow is reported" * doctest::test_suite("asan_canary"))
{
    LuaMemory::CreatePools(true);
    volatile char* p = (volatile char*)Lua_Red_realloc(0, 0, 13);
    REQUIRE(p);
    REQUIRE(LuaMemory::LocatePool(13, (void*)p));
    p[13] = 1;
    Lua_Red_Free((void*)p, 13);
}

TEST_CASE("asan canary lua bin use after free is reported" * doctest::test_suite("asan_canary"))
{
    LuaMemory::CreatePools(true);
    volatile char* p = (volatile char*)Lua_Red_realloc(0, 0, 16);
    REQUIRE(p);
    REQUIRE(LuaMemory::LocatePool(16, (void*)p));
    Lua_Red_Free((void*)p, 16);
    p[0] = 1;
}

TEST_CASE("asan canary net packet overflow is reported" * doctest::test_suite("asan_canary"))
{
    NetPacket::Create();
    NetPacket* pPacket = NetPacket::Alloc(false);
    REQUIRE(pPacket);
    volatile char* p = (volatile char*)pPacket;
    p[sizeof(NetBigPacket)] = 1;
    pPacket->Free();
}

TEST_CASE("asan canary net packet use after free is reported" * doctest::test_suite("asan_canary"))
{
    NetPacket::Create();
    NetPacket* pPacket = NetPacket::Alloc(false);
    REQUIRE(pPacket);
    volatile char* p = (volatile char*)pPacket;
    pPacket->Free();
    p[0] = 1;
}

TEST_CASE("asan canary red stack overflow is reported" * doctest::test_suite("asan_canary"))
{
    RedStack stack;
    volatile char* p = (volatile char*)stack.Alloc(13);
    p[13] = 1;
}

TEST_CASE("asan canary red stack use after scope is reported" * doctest::test_suite("asan_canary"))
{
    volatile char* p = 0;
    {
        RedStack stack;
        p = (volatile char*)stack.Alloc(16);
    }
    p[0] = 1;
}

TEST_CASE("asan canary stack allocator overflow is reported" * doctest::test_suite("asan_canary"))
{
    void* pBlock = RedAllocFromHeap(__RedCurrHeap, 256, 0);
    REQUIRE(pBlock);
    {
        util::StackAllocator alloc(false, true, 16);
        util::Buffer pool(pBlock, 256);
        alloc.Set(pool);
        util::Buffer buf(0, 13);
        alloc.Allocate(buf);
        volatile char* p = (volatile char*)buf.data_;
        REQUIRE(p);
        p[13] = 1;
    }
    RedFreeToHeap(__RedCurrHeap, pBlock, 0);
}

TEST_CASE("asan canary stack allocator use after free is reported" * doctest::test_suite("asan_canary"))
{
    void* pBlock = RedAllocFromHeap(__RedCurrHeap, 256, 0);
    REQUIRE(pBlock);
    {
        util::StackAllocator alloc(false, true, 16);
        util::Buffer pool(pBlock, 256);
        alloc.Set(pool);
        util::Buffer buf(0, 16);
        alloc.Allocate(buf);
        volatile char* p = (volatile char*)buf.data_;
        REQUIRE(p);
        alloc.Deallocate(buf);
        p[0] = 1;
    }
    RedFreeToHeap(__RedCurrHeap, pBlock, 0);
}

TEST_CASE("asan canary sub allocator overflow is reported" * doctest::test_suite("asan_canary"))
{
    util::StaticSubAlloc<CanaryElem> alloc;
    util::Buffer buf(0, 13);
    alloc.Allocate(buf);
    volatile char* p = (volatile char*)buf.data_;
    REQUIRE(p);
    p[13] = 1;
    alloc.Deallocate(buf);
}

TEST_CASE("asan canary sub allocator use after free is reported" * doctest::test_suite("asan_canary"))
{
    util::StaticSubAlloc<CanaryElem> alloc;
    util::Buffer buf(0, 16);
    alloc.Allocate(buf);
    volatile char* p = (volatile char*)buf.data_;
    REQUIRE(p);
    alloc.Deallocate(buf);
    p[0] = 1;
}

TEST_CASE("asan canary list allocator overflow is reported" * doctest::test_suite("asan_canary"))
{
    util::ListAllocator* pAlloc = util::ListAllocator::Create(0x2000, 0x10000);
    util::Buffer buf(0, 13);
    pAlloc->Allocate(buf);
    volatile char* p = (volatile char*)buf.data_;
    REQUIRE(p);
    p[13] = 1;
    pAlloc->Deallocate(buf);
    delete pAlloc;
}

TEST_CASE("asan canary list allocator use after free is reported" * doctest::test_suite("asan_canary"))
{
    util::ListAllocator* pAlloc = util::ListAllocator::Create(0x2000, 0x10000);
    util::Buffer buf(0, 32);
    pAlloc->Allocate(buf);
    volatile char* p = (volatile char*)buf.data_;
    REQUIRE(p);
    pAlloc->Deallocate(buf);
    p[0] = 1;
    delete pAlloc;
}

TEST_CASE("asan canary snd free list overflow is reported" * doctest::test_suite("asan_canary"))
{
    Snd::FreeList list(16, 1, false, 8);
    volatile char* p = (volatile char*)list.Alloc();
    REQUIRE(p);
    p[16] = 1;
    list.Free((void*)p);
    list.PurgeBlocks();
}

TEST_CASE("asan canary snd free list use after free is reported" * doctest::test_suite("asan_canary"))
{
    Snd::FreeList list(16, 1, false, 8);
    volatile char* p = (volatile char*)list.Alloc();
    REQUIRE(p);
    list.Free((void*)p);
    p[0] = 1;
    list.PurgeBlocks();
}

TEST_CASE("asan canary glyph memory overflow is reported" * doctest::test_suite("asan_canary"))
{
    RedGlyphMemoryManager* pManager = new RedGlyphMemoryManager(32);
    void* pBlock = pManager->Alloc(1);
    volatile char* p = (volatile char*)pBlock;
    REQUIRE(p);
    p[sizeof(RedGlyphMemoryManager::VertexBlock)] = 1;
    pManager->Free(pBlock);
}

TEST_CASE("asan canary glyph memory use after free is reported" * doctest::test_suite("asan_canary"))
{
    RedGlyphMemoryManager* pManager = new RedGlyphMemoryManager(32);
    void* pBlock = pManager->Alloc(1);
    volatile char* p = (volatile char*)pBlock;
    REQUIRE(p);
    pManager->Free(pBlock);
    p[0] = 1;
}
#endif
