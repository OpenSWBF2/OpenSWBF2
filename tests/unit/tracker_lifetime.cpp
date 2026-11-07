#include "third_party/doctest/doctest.h"

#include <string>

#include "Battlefront2/Source/Tracker.h"
#include "Battlefront2/Source/Trackable.h"
#include "Battlefront2/Source/NetGame.h"

#include "testutil.h"

extern Tracker sHostInstances[64];
extern Tracker sClientInstances[64];
extern Tracker (*sInstances)[64];
extern AloTrackable aloTrackable;

namespace
{

struct TrackerWorldGuard
{
    TrackerWorldGuard() : mOneWorld(netIsOneWorld), mInstances(sInstances) {}

    ~TrackerWorldGuard()
    {
        for (int i = 0; i < 64; ++i)
        {
            sHostInstances[i].mTrackable = 0;
            sClientInstances[i].mTrackable = 0;
            aloTrackable.mTrackable[i][0] = (Trackable*)-1;
            aloTrackable.mTrackable[i][1] = (Trackable*)-1;
        }
        sInstances = mInstances;
        netIsOneWorld = mOneWorld;
    }

    bool mOneWorld;
    Tracker (*mInstances)[64];
};

}

TEST_CASE("destroying a trackable clears both tracker worlds and the alo slots" * doctest::test_suite("regression"))
{
    TrackerWorldGuard kGuard;
    netIsOneWorld = false;
    Trackable* pDying = new Trackable;
    Trackable* pOther = new Trackable;

    sHostInstances[3].mTrackable = pDying;
    sHostInstances[63].mTrackable = pDying;
    sHostInstances[7].mTrackable = pOther;
    sClientInstances[5].mTrackable = pDying;
    sClientInstances[62].mTrackable = pDying;
    aloTrackable.mTrackable[5][0] = pDying;
    aloTrackable.mTrackable[63][1] = pDying;
    aloTrackable.mTrackable[8][0] = pOther;

    Tracker::EnterClient();
    REQUIRE(sInstances == &sClientInstances);
    delete pDying;
    Tracker::LeaveClient();
    REQUIRE(sInstances == &sHostInstances);

    for (int i = 0; i < 64; ++i)
    {
        CHECK(sHostInstances[i].mTrackable != pDying);
        CHECK(sClientInstances[i].mTrackable != pDying);
        CHECK(aloTrackable.mTrackable[i][0] != pDying);
        CHECK(aloTrackable.mTrackable[i][1] != pDying);
    }
    CHECK(sHostInstances[3].mTrackable == (Trackable*)0);
    CHECK(sHostInstances[63].mTrackable == (Trackable*)0);
    CHECK(sClientInstances[5].mTrackable == (Trackable*)0);
    CHECK(sClientInstances[62].mTrackable == (Trackable*)0);
    CHECK(aloTrackable.mTrackable[5][0] == (Trackable*)-1);
    CHECK(aloTrackable.mTrackable[63][1] == (Trackable*)-1);
    CHECK(sHostInstances[7].mTrackable == pOther);
    CHECK(aloTrackable.mTrackable[8][0] == pOther);

    sHostInstances[7].mTrackable = 0;
    aloTrackable.mTrackable[8][0] = (Trackable*)-1;
    delete pOther;
}

TEST_CASE("trackable destructor removes it from every tracker (source pin: callers need the game runtime)" * doctest::test_suite("regression"))
{
    std::string sText;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\Trackable.cpp", sText));
    CHECK(sText.find("Trackable::~Trackable()\n{\n    Tracker::RemoveTrackable(this);\n") != std::string::npos);
    std::string sTracker;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\Tracker.cpp", sTracker));
    CHECK(sTracker.find("0xDDDDDDDD") == std::string::npos);
    CHECK(sTracker.find("for (PblList<Tracker>::Itor it(sAllInstances); it != sAllInstances.End(); ++it)") != std::string::npos);
}
