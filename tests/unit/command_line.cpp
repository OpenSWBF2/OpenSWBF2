#include "third_party/doctest/doctest.h"

#include <string>

#include "Battlefront2/Source/CommandLine.h"
#include "Battlefront2/Source/GameLoop.h"
#include "PebbleFL/Common/PblSimOrder.h"

namespace
{

void Parse(const char* pcLine)
{
    GameLoop::mMissionScript[0] = 0;
    gExitOnGameOver = false;
    PblSetSimOrder(PBL_SIMORDER_ADDRESS);
    ParseCommandLine(0, pcLine);
}

std::string Mission()
{
    return GameLoop::mMissionScript;
}

}

TEST_CASE("command line keeps -simorder values out of the mission name" * doctest::test_suite("regression"))
{
    Parse("geo1c_con -simorder serial");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_SERIAL);
    CHECK_FALSE(SimOrderArgRejected());

    Parse("-simorder bogus geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_ADDRESS);
    CHECK(SimOrderArgRejected());

    Parse("-simorder -exitongameover geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(gExitOnGameOver);
    CHECK(SimOrderArgRejected());

    Parse("geo1c_con -simorder");
    CHECK(Mission() == "geo1c_con");
    CHECK(SimOrderArgRejected());

    Parse("geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_ADDRESS);
    CHECK_FALSE(SimOrderArgRejected());
}

TEST_CASE("command line lets the last -simorder value win" * doctest::test_suite("regression"))
{
    Parse("-simorder bogus -simorder serial geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_SERIAL);
    CHECK_FALSE(SimOrderArgRejected());

    Parse("-simorder serial -simorder bogus geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_SERIAL);
    CHECK(SimOrderArgRejected());
}

TEST_CASE("command line keeps -perturbheap and -repeatmission out of the mission name" * doctest::test_suite("regression"))
{
    Parse("-perturbheap 1:pad,top,fill,pool geo1c_con");
    CHECK(Mission() == "geo1c_con");

    Parse("geo1c_con -perturbheap 7 -exitongameover");
    CHECK(Mission() == "geo1c_con");
    CHECK(gExitOnGameOver);

    Parse("-perturbheap -exitongameover geo1c_con");
    CHECK(Mission() == "geo1c_con");
    CHECK(gExitOnGameOver);

    Parse("-perturbheap 2:pool, top geo1c_con -simorder serial");
    CHECK(Mission() == "geo1c_con");
    CHECK(PblGetSimOrder() == PBL_SIMORDER_SERIAL);

    Parse("-repeatmission geo1c_con");
    CHECK(Mission() == "geo1c_con");

    Parse("-shellfirst geo1c_con");
    CHECK(Mission() == "geo1c_con");
}

TEST_CASE("command line reports test-only switches it ignores" * doctest::test_suite("regression"))
{
    Parse("-perturbheap 1:pad -repeatmission -shellfirst geo1c_con");
    std::string ignored;
    const char* pcName = 0;
    int iCount = 0;
    for (int i = 0; TestOnlyArgIgnored(i, &pcName); ++i)
    {
        ++iCount;
        if (pcName)
        {
            ignored += pcName;
            ignored += " ";
        }
    }
    CHECK(iCount == 3);
#ifdef OPENSWBF2_TEST
    CHECK(ignored == "");
#else
    CHECK(ignored == "perturbheap repeatmission shellfirst ");
#endif

    Parse("geo1c_con");
    ignored.clear();
    for (int i = 0; TestOnlyArgIgnored(i, &pcName); ++i)
    {
        if (pcName)
        {
            ignored += pcName;
        }
    }
    CHECK(ignored == "");
    CHECK_FALSE(TestOnlyArgIgnored(-1, &pcName));
}
