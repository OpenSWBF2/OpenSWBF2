#include "third_party/doctest/doctest.h"

#include <string.h>
#include "PebbleFL/Common/PblRandom.h"
#include "PebbleFL/Common/PblUnicode.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "testutil.h"

TEST_CASE("random lcg sequence for seed 1" * doctest::test_suite("unit"))
{
    PblRandom rng(1);
    CHECK(rng.GetInt() == 0x5E883C88u);
    CHECK(rng.GetInt() == 0xB4738116u);
    CHECK(rng.GetInt() == 0x5E980CF0u);
}

TEST_CASE("random same seed repeats" * doctest::test_suite("unit"))
{
    PblRandom a(1234);
    PblRandom b(1234);
    CHECK(a.GetInt() == b.GetInt());
    CHECK(a.GetSeed() == b.GetSeed());
}

TEST_CASE("random normal cache round-trips through get and set" * doctest::test_suite("unit"))
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

TEST_CASE("strlcpy truncates and terminates" * doctest::test_suite("unit"))
{
    char acBuf[8];
    memset(acBuf, 0x7F, sizeof(acBuf));
    CHECK(strlcpy(acBuf, "abcdefghij", 5) == 5);
    CHECK(strcmp(acBuf, "abcd") == 0);
}

TEST_CASE("strlcat truncates and terminates" * doctest::test_suite("unit"))
{
    char acBuf[8] = "ab";
    CHECK(strlcat(acBuf, "cdefghij", 6) == 6);
    CHECK(strcmp(acBuf, "abcde") == 0);
}

TEST_CASE("hash table store find remove" * doctest::test_suite("unit"))
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
