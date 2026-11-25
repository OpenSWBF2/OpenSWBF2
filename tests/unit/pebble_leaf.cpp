#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string.h>
#include "PebbleFL/Common/PblRandom.h"
#include "PebbleFL/Common/PblUnicode.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "testutil.h"

TEST_CASE_NAMED(FPebbleLeafTest1, "Unit::PebbleLeaf::random lcg sequence for seed 1", "[SmokeFilter]")
{
    PblRandom rng(1);
    CHECK(rng.GetInt() == 0x5E883C88u);
    CHECK(rng.GetInt() == 0xB4738116u);
    CHECK(rng.GetInt() == 0x5E980CF0u);
}

TEST_CASE_NAMED(FPebbleLeafTest2, "Unit::PebbleLeaf::random same seed repeats", "[SmokeFilter]")
{
    PblRandom a(1234);
    PblRandom b(1234);
    CHECK(a.GetInt() == b.GetInt());
    CHECK(a.GetSeed() == b.GetSeed());
}

TEST_CASE_NAMED(FPebbleLeafTest3, "Unit::PebbleLeaf::random normal cache round-trips through get and set", "[SmokeFilter]")
{
    int iSavedHas = 0;
    float fSaved = 0.0f;
    PblRandom::GetNormalCache(&iSavedHas, &fSaved);
    PblRandom::SetNormalCache(1, 0.25f);
    int iHas = 0;
    float fValue = 0.0f;
    PblRandom::GetNormalCache(&iHas, &fValue);
    CHECK(iHas == 1);
    CHECK(fValue == 0.25f);
    PblRandom rng(7);
    CHECK(rng.GetNormal() == 0.25f);
    CHECK(rng.GetSeed() == 7u);
    PblRandom::GetNormalCache(&iHas, &fValue);
    CHECK(iHas == 0);
    PblRandom::SetNormalCache(iSavedHas, fSaved);
}

TEST_CASE_NAMED(FPebbleLeafTest4, "Unit::PebbleLeaf::strlcpy truncates and terminates", "[SmokeFilter]")
{
    char acBuf[8];
    memset(acBuf, 0x7F, sizeof(acBuf));
    CHECK(strlcpy(acBuf, "abcdefghij", 5) == 5);
    CHECK(strcmp(acBuf, "abcd") == 0);
}

TEST_CASE_NAMED(FPebbleLeafTest5, "Unit::PebbleLeaf::strlcat truncates and terminates", "[SmokeFilter]")
{
    char acBuf[8] = "ab";
    CHECK(strlcat(acBuf, "cdefghij", 6) == 6);
    CHECK(strcmp(acBuf, "abcde") == 0);
}

TEST_CASE_NAMED(FPebbleLeafTest6, "Unit::PebbleLeaf::hash table store find remove", "[SmokeFilter]")
{
    PblHashTable<int, 8> table;
    table.Clear();
    int iValue = 5;
    unsigned int uiKey = PblHash("thing");
    CHECK(!table.Find(uiKey));
    CHECK(HashStoreNew(table, uiKey, &iValue));
    REQUIRE(table.Find(uiKey));
    CHECK(*table.Find(uiKey) == 5);
    CHECK(HashRemoveOld(table, uiKey));
    CHECK(!table.Find(uiKey));
}
