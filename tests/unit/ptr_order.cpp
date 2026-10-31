#include "third_party/doctest/doctest.h"

#include "Battlefront2/Source/Thread.h"
#include "PebbleFL/Common/PblPtrOrder.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "testutil.h"

namespace
{

void* Addr(unsigned int uiAddr)
{
    return (void*)(uintptr_t)uiAddr;
}

bool ReadSource(const char* pcPath, std::string& sOut)
{
    FILE* pFile = fopen(pcPath, "rb");
    if (!pFile)
    {
        return false;
    }
    char acBuf[4096];
    size_t uiRead;
    while ((uiRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sOut.append(acBuf, uiRead);
    }
    fclose(pFile);
    return true;
}

}

TEST_CASE("ptr order key is monotonic across 2GB" * doctest::test_suite("unit"))
{
    const unsigned int auiAddr[] = { 0x00010000u, 0x7FFFFFF0u, 0x80000010u, 0x80000014u, 0xFFFEFFFCu };
    for (int i = 1; i < 5; ++i)
    {
        CHECK(PblPtrOrderKey(Addr(auiAddr[i - 1])) < PblPtrOrderKey(Addr(auiAddr[i])));
    }
    CHECK(PblPtrOrderKey(Addr(0xFFFEFFFCu)) < 0x7FFFFFFF);
}

TEST_CASE("ordered list keeps address order across 2GB" * doctest::test_suite("unit"))
{
    const unsigned int auiAddr[] = { 0x80000014u, 0x00010000u, 0xFFFEFFFCu, 0x7FFFFFF0u, 0x80000010u };
    PblOrderedList<Thread, intptr_t> list;
    PblOrderedList<Thread, intptr_t>::Node aNodes[5];
    for (int i = 0; i < 5; ++i)
    {
        list.Add(&aNodes[i], NULL, PblPtrOrderKey(Addr(auiAddr[i])));
    }

    const unsigned int auiSorted[] = { 0x00010000u, 0x7FFFFFF0u, 0x80000010u, 0x80000014u, 0xFFFEFFFCu };
    PblOrderedList<Thread, intptr_t>::Node* pNode = list._head._pNext;
    for (int i = 0; i < 5; ++i)
    {
        CHECK(pNode->_iOrder == PblPtrOrderKey(Addr(auiSorted[i])));
        pNode = pNode->_pNext;
    }
    CHECK(pNode == &list._head);
}

TEST_CASE("collision tie-break is unsigned" * doctest::test_suite("unit"))
{
    CHECK(PblPtrAfter(Addr(0x80000010u), Addr(0x7FFFFFF0u)));
    CHECK(!PblPtrAfter(Addr(0x7FFFFFF0u), Addr(0x80000010u)));
    CHECK(!PblPtrAfter(Addr(0x80000010u), Addr(0x80000010u)));
}

TEST_CASE("pointer order sites use the unsigned helpers" * doctest::test_suite("unit"))
{
    std::string sThread;
    std::string sCollision;
    std::string sMemory;
    if (!ReadSource("../src/Battlefront2/Source/Thread.cpp", sThread) ||
        !ReadSource("../src/Battlefront2/Source/CollisionManager.cpp", sCollision) ||
        !ReadSource("../src/RedEngineFL/Memory/RedMemory.cpp", sMemory))
    {
        SkipCase("source files not found");
    }

    CHECK(sThread.find("(int)this") == std::string::npos);
    CHECK(sThread.find("PblPtrOrderKey(this)") != std::string::npos);
    CHECK(sCollision.find("(int)pOther > (int)pObject") == std::string::npos);
    CHECK(sCollision.find("PblPtrAfter(pOther, pObject)") != std::string::npos);
    CHECK(sMemory.find("0x70000000") == std::string::npos);
    CHECK(sMemory.find("if (sRecursionDepth <= 0)") != std::string::npos);
}

TEST_CASE("ptr order key is width agnostic" * doctest::test_suite("unit"))
{
    const uintptr_t kTop = (uintptr_t)1 << (sizeof(void*) * 8 - 1);
    const uintptr_t auiAddr[] = { 0x10000u, kTop - 16, kTop + 16, ~(uintptr_t)0 - 3 };
    for (int i = 1; i < 4; ++i)
    {
        CHECK(PblPtrOrderKey((void*)auiAddr[i - 1]) < PblPtrOrderKey((void*)auiAddr[i]));
    }
    CHECK(PblPtrAfter((void*)(kTop + 16), (void*)(kTop - 16)));
    CHECK(!PblPtrAfter((void*)(kTop + 16), (void*)(kTop + 16)));
}

TEST_CASE("ordered list sentinel is the key maximum" * doctest::test_suite("unit"))
{
    PblOrderedList<Thread, intptr_t> list;
    PblOrderedList<Thread, intptr_t>::Node node;
    list.Add(&node, NULL, 0);
    CHECK(list._head._iOrder == INTPTR_MAX);

    PblOrderedList<Thread, long long> wList;
    PblOrderedList<Thread, long long>::Node wNode;
    wList.Add(&wNode, NULL, 0);
    CHECK(wList._head._iOrder == LLONG_MAX);

    PblOrderedList<Thread, float> fList;
    PblOrderedList<Thread, float>::Node fNode;
    fList.Add(&fNode, NULL, 0.0f);
    unsigned int uiBits = 0;
    memcpy(&uiBits, &fList._head._iOrder, sizeof(uiBits));
    CHECK(uiBits == 0x4F000000u);
}
