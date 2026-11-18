#include "third_party/doctest/doctest.h"

#include <stdint.h>
#include <string.h>

#include "Battlefront2/Source/NetSendScopes.h"

namespace
{

GameObject* FakeObject(uintptr_t uAddress)
{
    return (GameObject*)uAddress;
}

}

TEST_CASE("send scopes merge each object once in scope order" * doctest::test_suite("regression"))
{
    GameObject* aapScopes[2][64];
    memset(aapScopes, 0, sizeof(aapScopes));
    GameObject* pA = FakeObject(0x1000);
    GameObject* pB = FakeObject(0x2000);
    GameObject* pC = FakeObject(0x3000);
    aapScopes[0][0] = pA;
    aapScopes[1][0] = pB;
    aapScopes[0][1] = pB;
    aapScopes[1][1] = pA;
    aapScopes[1][5] = pC;
    aapScopes[0][9] = pC;

    GameObject* apSends[64];
    memset(apSends, 0, sizeof(apSends));
    REQUIRE(MergeSendScopes(apSends, aapScopes) == 3);
    CHECK(apSends[0] == pA);
    CHECK(apSends[1] == pB);
    CHECK(apSends[2] == pC);
    CHECK(apSends[3] == nullptr);
}

TEST_CASE("send scopes stop at 64 objects" * doctest::test_suite("regression"))
{
    GameObject* aapScopes[2][64];
    for (int i = 0; i < 64; ++i)
    {
        aapScopes[0][i] = FakeObject(0x1000 + 0x10 * (uintptr_t)i);
        aapScopes[1][i] = FakeObject(0x9000 + 0x10 * (uintptr_t)i);
    }

    GameObject* apSends[64];
    REQUIRE(MergeSendScopes(apSends, aapScopes) == 64);
    for (int i = 0; i < 32; ++i)
    {
        CHECK(apSends[2 * i] == aapScopes[0][i]);
        CHECK(apSends[2 * i + 1] == aapScopes[1][i]);
    }
}

TEST_CASE("send scopes keep objects whose low address bits match" * doctest::test_suite("regression"))
{
    GameObject* aapScopes[2][64];
    memset(aapScopes, 0, sizeof(aapScopes));
    const uintptr_t uHigh = (uintptr_t)1 << (sizeof(void*) * 8 - 4);
    GameObject* pLow = FakeObject(0x1000);
    GameObject* pHigh = FakeObject(uHigh + 0x1000);
    aapScopes[0][0] = pLow;
    aapScopes[1][0] = pHigh;
    aapScopes[0][1] = pHigh;

    GameObject* apSends[64];
    memset(apSends, 0, sizeof(apSends));
    REQUIRE(MergeSendScopes(apSends, aapScopes) == 2);
    CHECK(apSends[0] == pLow);
    CHECK(apSends[1] == pHigh);
}
