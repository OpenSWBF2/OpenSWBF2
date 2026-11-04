#include "third_party/doctest/doctest.h"

#include <string.h>

#include "PebbleFL/Common/PblHashTable.h"

namespace
{

void* RetailFind(const unsigned int* pTable, int iCount, unsigned int uiHash)
{
    const int iHalf = iCount >> 1;
    const unsigned int* pSlot = pTable + ((iHalf - 1) & uiHash);

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

    return (void*)pSlot[iHalf];
}

bool RetailStore(unsigned int* pTable, int iCount, unsigned int uiHash, void* pValue)
{
    const int iHalf = iCount >> 1;
    unsigned int* pSlot = pTable + ((iHalf - 1) & uiHash);

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
    pSlot[iHalf] = (unsigned int)pValue;
    return true;
}

bool RetailRemove(unsigned int* pTable, int iCount, unsigned int uiHash)
{
    const int iHalf = iCount >> 1;
    unsigned int* pSlot = pTable + ((iHalf - 1) & uiHash);

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

        const unsigned int uiMoved = *pSlot;
        void* const pMovedValue = (void*)pSlot[iHalf];
        *pSlot = 0;
        RetailStore(pTable, iCount, uiMoved, pMovedValue);
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
    return (int*)(0x10000 + uiKey * 16);
}

template <int SLOTS>
void CheckFullTable(const unsigned int* puiKeys, unsigned int uiAbsent)
{
    unsigned int auiTable[2 * SLOTS];
    memset(auiTable, 0, sizeof(auiTable));
    for (int i = 0; i < SLOTS; ++i)
    {
        REQUIRE(HashCode::_Store(auiTable, 2 * SLOTS, puiKeys[i], FakeValue(puiKeys[i])));
    }
    for (int i = 0; i < SLOTS; ++i)
    {
        CHECK(HashCode::_Find(auiTable, 2 * SLOTS, puiKeys[i]) == FakeValue(puiKeys[i]));
    }

    unsigned int auiBefore[2 * SLOTS];
    memcpy(auiBefore, auiTable, sizeof(auiBefore));

    CHECK(HashCode::_Find(auiTable, 2 * SLOTS, uiAbsent) == (void*)0);
    CHECK_FALSE(HashCode::_Store(auiTable, 2 * SLOTS, uiAbsent, FakeValue(uiAbsent)));
    CHECK_FALSE(HashCode::_Remove(auiTable, 2 * SLOTS, uiAbsent));
    CHECK(memcmp(auiBefore, auiTable, sizeof(auiBefore)) == 0);

    CHECK(HashCode::_Remove(auiTable, 2 * SLOTS, puiKeys[0]));
    CHECK(HashCode::_Find(auiTable, 2 * SLOTS, puiKeys[0]) == (void*)0);
    for (int i = 1; i < SLOTS; ++i)
    {
        CHECK(HashCode::_Find(auiTable, 2 * SLOTS, puiKeys[i]) == FakeValue(puiKeys[i]));
    }
    CHECK(HashCode::_Store(auiTable, 2 * SLOTS, uiAbsent, FakeValue(uiAbsent)));
    CHECK(HashCode::_Find(auiTable, 2 * SLOTS, uiAbsent) == FakeValue(uiAbsent));

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

TEST_CASE("hash table full table probes terminate" * doctest::test_suite("regression"))
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

TEST_CASE("hash table non-full probes match retail layout" * doctest::test_suite("regression"))
{
    unsigned int auiTable[32];
    memset(auiTable, 0, sizeof(auiTable));
    unsigned int auiRetail[32];
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
            const bool bNew = HashCode::_Store(auiTable, 32, uiKey, FakeValue(uiKey));
            const bool bOld = RetailStore(auiRetail, 32, uiKey, FakeValue(uiKey));
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
