#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <stdint.h>
#include <string.h>

#include "PebbleFL/Common/PblHashTable.h"

namespace
{

uintptr_t RetailFind(const uintptr_t* pTable, int iCount, unsigned int uiHash)
{
    const int iHalf = iCount >> 1;
    const uintptr_t* pSlot = pTable + ((iHalf - 1) & uiHash);

    if (!uiHash)
    {
        return 0;
    }

    while (*pSlot != uiHash)
    {
        if (!*pSlot)
        {
            return 0;
        }

        if (pSlot <= pTable)
        {
            pSlot += iHalf;
        }
        --pSlot;
    }

    return pSlot[iHalf];
}

bool RetailStore(uintptr_t* pTable, int iCount, unsigned int uiHash, uintptr_t uValue)
{
    const int iHalf = iCount >> 1;
    uintptr_t* pSlot = pTable + ((iHalf - 1) & uiHash);

    while (*pSlot)
    {
        if (*pSlot == uiHash)
        {
            return false;
        }

        if (pSlot <= pTable)
        {
            pSlot += iHalf;
        }
        --pSlot;
    }

    *pSlot = uiHash;
    pSlot[iHalf] = uValue;
    return true;
}

bool RetailRemove(uintptr_t* pTable, int iCount, unsigned int uiHash)
{
    const int iHalf = iCount >> 1;
    uintptr_t* pSlot = pTable + ((iHalf - 1) & uiHash);

    while (*pSlot != uiHash)
    {
        if (!*pSlot)
        {
            return false;
        }

        if (pSlot <= pTable)
        {
            pSlot += iHalf;
        }
        --pSlot;
    }

    *pSlot = 0;

    for (;;)
    {
        if (pSlot <= pTable)
        {
            pSlot += iHalf;
        }
        --pSlot;

        if (!*pSlot)
        {
            return true;
        }

        const unsigned int uiMoved = (unsigned int)*pSlot;
        const uintptr_t uMovedValue = pSlot[iHalf];
        *pSlot = 0;
        RetailStore(pTable, iCount, uiMoved, uMovedValue);
    }
}

struct HashCode : PblHashTableCode
{
    using PblHashTableCode::_Find;
    using PblHashTableCode::_Store;
    using PblHashTableCode::_Remove;
};

int* FakeValue(unsigned int uiKey)
{
    return (int*)(uintptr_t)(0x10000 + uiKey * 16);
}

uintptr_t FakeSlot(unsigned int uiKey)
{
    return (uintptr_t)FakeValue(uiKey);
}

template <int SLOTS>
void CheckFullTable(const unsigned int* puiKeys, unsigned int uiAbsent)
{
    uintptr_t auiTable[2 * SLOTS];
    memset(auiTable, 0, sizeof(auiTable));
    for (int i = 0; i < SLOTS; ++i)
    {
        REQUIRE(HashCode::_Store(auiTable, 2 * SLOTS, puiKeys[i], FakeSlot(puiKeys[i])));
    }
    for (int i = 0; i < SLOTS; ++i)
    {
        CHECK(HashCode::_Find(auiTable, 2 * SLOTS, puiKeys[i]) == FakeSlot(puiKeys[i]));
    }

    uintptr_t auiBefore[2 * SLOTS];
    memcpy(auiBefore, auiTable, sizeof(auiBefore));

    CHECK(HashCode::_Find(auiTable, 2 * SLOTS, uiAbsent) == 0);
    CHECK_FALSE(HashCode::_Store(auiTable, 2 * SLOTS, uiAbsent, FakeSlot(uiAbsent)));
    CHECK_FALSE(HashCode::_Remove(auiTable, 2 * SLOTS, uiAbsent));
    CHECK(memcmp(auiBefore, auiTable, sizeof(auiBefore)) == 0);

    CHECK(HashCode::_Remove(auiTable, 2 * SLOTS, puiKeys[0]));
    CHECK(HashCode::_Find(auiTable, 2 * SLOTS, puiKeys[0]) == 0);
    for (int i = 1; i < SLOTS; ++i)
    {
        CHECK(HashCode::_Find(auiTable, 2 * SLOTS, puiKeys[i]) == FakeSlot(puiKeys[i]));
    }
    CHECK(HashCode::_Store(auiTable, 2 * SLOTS, uiAbsent, FakeSlot(uiAbsent)));
    CHECK(HashCode::_Find(auiTable, 2 * SLOTS, uiAbsent) == FakeSlot(uiAbsent));

    PblHashTable<int, SLOTS> kTable;
    for (int i = 0; i < SLOTS; ++i)
    {
        kTable.Store(puiKeys[i], FakeValue(puiKeys[i]));
    }
    CHECK(kTable.GetNumEntries() == SLOTS);
    kTable.Store(uiAbsent, FakeValue(uiAbsent));
    CHECK(kTable.GetNumEntries() == SLOTS);
    CHECK(kTable.Find(uiAbsent) == (int*)0);
    CHECK(kTable.GetMaxEntries() == SLOTS);
}

}

TEST_CASE_NAMED(FPblHashtableFullTest1, "Unit::PblHashtableFull::hash table full table probes terminate", "[EngineFilter]")
{
    const unsigned int auiSmall[4] = { 1, 5, 2, 3 };
    CheckFullTable<4>(auiSmall, 9);

    unsigned int auiLarge[64];
    for (unsigned int i = 0; i < 64; ++i)
    {
        auiLarge[i] = (i % 2) ? 0x1000 + i * 64 + 7 : i + 1;
    }
    for (int i = 0; i < 64; ++i)
    {
        for (int j = i + 1; j < 64; ++j)
        {
            REQUIRE(auiLarge[i] != auiLarge[j]);
        }
    }
    CheckFullTable<64>(auiLarge, 0x77777);
}

TEST_CASE_NAMED(FPblHashtableFullTest2, "Unit::PblHashtableFull::hash table non-full probes match retail layout", "[EngineFilter]")
{
    uintptr_t auiTable[32];
    memset(auiTable, 0, sizeof(auiTable));
    uintptr_t auiRetail[32];
    memset(auiRetail, 0, sizeof(auiRetail));

    unsigned int uiSeed = 0x2468ace1;
    int iLive = 0;
    for (int iOp = 0; iOp < 20000; ++iOp)
    {
        uiSeed = uiSeed * 1103515245u + 12345u;
        const unsigned int r = uiSeed >> 8;
        unsigned int uiKey = (r % 6) * 16 + (r % 3) + 1;
        const unsigned int uiKind = (r >> 12) % 3;
        if (uiKind != 1 && ((r >> 16) % 17) == 0)
        {
            uiKey = 0;
        }

        if (uiKind == 0)
        {
            CHECK(HashCode::_Find(auiTable, 32, uiKey) == RetailFind(auiRetail, 32, uiKey));
        }
        else if (uiKind == 1)
        {
            if (iLive >= 15)
            {
                continue;
            }
            const bool bNew = HashCode::_Store(auiTable, 32, uiKey, FakeSlot(uiKey));
            const bool bOld = RetailStore(auiRetail, 32, uiKey, FakeSlot(uiKey));
            CHECK(bNew == bOld);
            if (bNew)
            {
                ++iLive;
            }
        }
        else
        {
            const bool bNew = HashCode::_Remove(auiTable, 32, uiKey);
            const bool bOld = RetailRemove(auiRetail, 32, uiKey);
            CHECK(bNew == bOld);
            if (bNew && uiKey)
            {
                --iLive;
            }
        }
        REQUIRE(memcmp(auiTable, auiRetail, sizeof(auiRetail)) == 0);
    }
}

TEST_CASE_NAMED(FPblHashtableFullTest3, "Unit::PblHashtableFull::pbl hashtable keeps values above 4 GB", "[EngineFilter]")
{
    const uintptr_t uHigh = (uintptr_t)1 << (sizeof(void*) * 8 - 4);
    const unsigned int auiKeys[4] = { 0x11, 0x21, 0x06, 0x13 };
    uintptr_t auValues[4];
    for (int i = 0; i < 4; ++i)
    {
        auValues[i] = uHigh + 0x40 * (uintptr_t)(i + 1) + (uintptr_t)i;
    }

    PblHashTable<int, 8> kTable;
    PblHashTable<int, 8> kValues;
    for (int i = 0; i < 4; ++i)
    {
        kTable.Store(auiKeys[i], (int*)auValues[i]);
        kValues.StoreValue(auiKeys[i], auValues[i]);
    }
    REQUIRE(kTable.GetNumEntries() == 4);
    REQUIRE(kValues.GetNumEntries() == 4);

    for (int i = 0; i < 4; ++i)
    {
        CHECK((uintptr_t)kTable.Find(auiKeys[i]) == auValues[i]);
        CHECK(kTable.FindValue(auiKeys[i]) == auValues[i]);
        CHECK(kTable.HasValue((const int*)auValues[i]));
        CHECK_FALSE(kTable.HasValue((const int*)(auValues[i] - uHigh)));
        CHECK(kValues.FindValue(auiKeys[i]) == auValues[i]);
        CHECK((uintptr_t)kValues.Find(auiKeys[i]) == auValues[i]);
    }

    int iSeen = 0;
    for (PblHashTable<int, 8>::Itor it = kTable.Begin(); it != kTable.End(); ++it)
    {
        int iFound = -1;
        for (int i = 0; i < 4; ++i)
        {
            if (auiKeys[i] == it.GetKey())
            {
                iFound = i;
            }
        }
        REQUIRE(iFound >= 0);
        CHECK((uintptr_t)(int*)it == auValues[iFound]);
        CHECK((uintptr_t)it.operator->() == auValues[iFound]);
        ++iSeen;
    }
    CHECK(iSeen == 4);
}

namespace
{

template <int SLOTS>
void CheckIterationOrder(const PblHashTable<int, SLOTS>& kTable, const unsigned int* puiOrder, int iCount)
{
    int iAt = 0;
    for (typename PblHashTable<int, SLOTS>::Itor it = kTable.Begin(); it != kTable.End(); ++it)
    {
        REQUIRE(iAt < iCount);
        CHECK(it.GetKey() == puiOrder[iAt]);
        CHECK((int*)it == FakeValue(puiOrder[iAt]));
        ++iAt;
    }
    CHECK(iAt == iCount);
}

}

TEST_CASE_NAMED(FPblHashtableFullTest4, "Unit::PblHashtableFull::pbl hashtable iteration order is pinned", "[EngineFilter]")
{
    const unsigned int auiKeys[6] = { 0x11, 0x21, 0x31, 0x06, 0x0E, 0x13 };
    PblHashTable<int, 8> kTable;
    for (int i = 0; i < 6; ++i)
    {
        kTable.Store(auiKeys[i], FakeValue(auiKeys[i]));
    }
    REQUIRE(kTable.GetNumEntries() == 6);

    const unsigned int auiOrder[6] = { 0x21, 0x11, 0x13, 0x0E, 0x06, 0x31 };
    CheckIterationOrder(kTable, auiOrder, 6);

    kTable.Remove(0x11);
    REQUIRE(kTable.GetNumEntries() == 5);
    const unsigned int auiAfterRemove[5] = { 0x31, 0x21, 0x13, 0x0E, 0x06 };
    CheckIterationOrder(kTable, auiAfterRemove, 5);
}
