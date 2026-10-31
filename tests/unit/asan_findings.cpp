#include "third_party/doctest/doctest.h"

#include <limits.h>
#include <string.h>
#include <string>

#include "testutil.h"
#include "PebbleFL/Common/PblIndex.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "Battlefront2/Source/CollisionMesh.h"

TEST_CASE("pbl in bounds accepts only indices inside the array" * doctest::test_suite("regression"))
{
    int aiValues[3] = {};
    CHECK(PblInBounds(aiValues, 0));
    CHECK(PblInBounds(aiValues, 1));
    CHECK(PblInBounds(aiValues, 2));
    CHECK(!PblInBounds(aiValues, -1));
    CHECK(!PblInBounds(aiValues, 3));
    CHECK(!PblInBounds(aiValues, INT_MIN));
}

struct HashTableWithSentinel
{
    PblHashTable<int, 4> mTable;
    unsigned int mSentinel;
};

TEST_CASE("hash table end converts to null" * doctest::test_suite("regression"))
{
    HashTableWithSentinel kHolder;
    memset(&kHolder, 0, sizeof(kHolder));
    kHolder.mSentinel = 0xDEADBEEF;
    CHECK((int*)kHolder.mTable.End() == (int*)0);

    int iValue = 9;
    REQUIRE(kHolder.mTable.Store(PblHash("one"), &iValue));
    int iCount = 0;
    for (PblHashTable<int, 4>::Itor it = kHolder.mTable.Begin(); it != kHolder.mTable.End(); ++it)
    {
        CHECK(*it == 9);
        ++iCount;
        if (iCount > 4)
        {
            break;
        }
    }
    CHECK(iCount == 1);
}

TEST_CASE("hash table iteration visits null valued entries" * doctest::test_suite("regression"))
{
    PblHashTable<int, 4> kTable;
    kTable.Clear();
    int iValue = 9;
    REQUIRE(kTable.Store(1, 0));
    REQUIRE(kTable.Store(2, &iValue));
    int iCount = 0;
    int iFound = 0;
    for (PblHashTable<int, 4>::Itor it = kTable.Begin(); it != kTable.End(); ++it)
    {
        ++iCount;
        if ((int*)it == &iValue)
        {
            ++iFound;
        }
        if (iCount > 4)
        {
            break;
        }
    }
    CHECK(iCount == 2);
    CHECK(iFound == 1);
}

TEST_CASE("remove polygon keeps the remaining polygons" * doctest::test_suite("regression"))
{
    PolyListContainer<4> kList;
    kList.Clear();
    PblVector3 avFirst[3] = {PblVector3(1.0f, 0.0f, 0.0f), PblVector3(2.0f, 0.0f, 0.0f),
                             PblVector3(3.0f, 0.0f, 0.0f)};
    PblVector3 avSecond[2] = {PblVector3(4.0f, 0.0f, 0.0f), PblVector3(5.0f, 0.0f, 0.0f)};
    PblVector3 avThird[3] = {PblVector3(6.0f, 0.0f, 0.0f), PblVector3(7.0f, 0.0f, 0.0f),
                             PblVector3(8.0f, 0.0f, 0.0f)};
    kList.AddPolygon(3, avFirst);
    kList.AddPolygon(2, avSecond);
    kList.AddPolygon(3, avThird);
    REQUIRE(kList.mPolyCount == 3);

    kList.RemovePolygon(1);
    CHECK(kList.mPolyCount == 2);
    CHECK(kList.mVertCount == 6);

    int iNumVerts = 0;
    const PblVector3* pVerts = 0;
    kList.GetPolygon(0, iNumVerts, pVerts);
    REQUIRE(iNumVerts == 3);
    for (int i = 0; i < 3; ++i)
    {
        CHECK(pVerts[i].x == avFirst[i].x);
    }
    kList.GetPolygon(1, iNumVerts, pVerts);
    REQUIRE(iNumVerts == 3);
    for (int i = 0; i < 3; ++i)
    {
        CHECK(pVerts[i].x == avThird[i].x);
    }
}

TEST_CASE("remove polygon leaves the slot past the count alone" * doctest::test_suite("regression"))
{
    PolyListContainer<4> kList;
    memset(&kList, 0, sizeof(kList));
    kList.Clear();
    PblVector3 avVerts[4] = {PblVector3(1.0f, 0.0f, 0.0f), PblVector3(2.0f, 0.0f, 0.0f),
                             PblVector3(3.0f, 0.0f, 0.0f), PblVector3(4.0f, 0.0f, 0.0f)};
    kList.AddPolygon(1, avVerts);
    kList.AddPolygon(2, avVerts);
    kList.AddPolygon(3, avVerts);
    kList.AddPolygon(4, avVerts);
    REQUIRE(kList.mPolyCount == 4);
    REQUIRE(kList.mVertCount == 10);

    kList.RemovePolygon(0);
    CHECK(kList.mPolyCount == 3);
    CHECK(kList.mVertCount == 9);
    CHECK(kList.mNumVerticesPerPoly[0] == 2);
    CHECK(kList.mNumVerticesPerPoly[1] == 3);
    CHECK(kList.mNumVerticesPerPoly[2] == 4);
    CHECK(kList.mNumVerticesPerPoly[3] == 4);
}

TEST_CASE("hero and game rules reads stay bounded" * doctest::test_suite("regression"))
{
    struct GuardSite
    {
        const char* pcPath;
        const char* pcGuard;
    };
    const GuardSite akSites[8] = {
        { "src\\Battlefront2\\Source\\Character.cpp", "return PblInBounds(netHeroLastPlayerID, mTeamNumber) &&" },
        { "src\\Battlefront2\\Source\\Character.cpp", "if (PblInBounds(netHeroLastPlayerID, mTeamNumber) &&" },
        { "src\\Battlefront2\\Source\\Character.cpp", "PblInBounds(netHeroUnlocked, iTeam) && netHeroUnlocked[iTeam] &&" },
        { "src\\Battlefront2\\Source\\Character.cpp", "PblInBounds(netHeroKillerID, rOwner.mCharacter->mTeamNumber)" },
        { "src\\Battlefront2\\Source\\Character.cpp", "!PblInBounds(netHeroPlayerID, mTeamNumber) ||" },
        { "src\\Battlefront2\\Source\\Character.cpp", "PblInBounds(netHeroAIID, mTeamNumber) ? netHeroAIID[mTeamNumber] : -1" },
        { "src\\Battlefront2\\Platform\\Pc\\pcSpawnDisplay.cpp", "PblInBounds(netHeroUnlocked, iTeam) && netHeroUnlocked[iTeam] &&" },
        { "src\\Battlefront2\\Source\\LuaCallbacks_Options.cpp", "if (PblInBounds(gNumBots, eRules))" },
    };
    for (int i = 0; i < 8; ++i)
    {
        std::string sText;
        REQUIRE(ReadRepoText(akSites[i].pcPath, sText));
        INFO(akSites[i].pcPath);
        INFO(akSites[i].pcGuard);
        CHECK(sText.find(akSites[i].pcGuard) != std::string::npos);
    }
}
