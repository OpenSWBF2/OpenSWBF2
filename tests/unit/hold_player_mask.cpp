#include "third_party/doctest/doctest.h"

#include <string>

#include "testutil.h"
#include "Battlefront2/Source/CommandPost.h"

namespace
{

int CountText(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    const std::string sNeedle(pcNeedle);
    for (size_t uPos = sText.find(sNeedle); uPos != std::string::npos; uPos = sText.find(sNeedle, uPos + sNeedle.size()))
    {
        ++iCount;
    }
    return iCount;
}

}

TEST_CASE("command post hold mask sites keep retail only in the sim" * doctest::test_suite("regression"))
{
    std::string sText;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\CaptureDisplay.cpp", sText));
    CHECK(CountText(sText, "CommandPost::HoldMaskHasPlayer(pPost->mHoldPlayerMask, pChr->mPlayerId)") == 1);
    CHECK(CountText(sText, "HoldMaskHasPlayerRetail") == 0);
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\EntitySoldier.cpp", sText));
    CHECK(CountText(sText, "CommandPost::HoldMaskHasPlayerRetail(") == 2);
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\EntityDroideka.cpp", sText));
    CHECK(CountText(sText, "CommandPost::HoldMaskHasPlayerRetail(") == 1);
}

TEST_CASE("command post hold mask matches the retail sign-extended 32-bit shift" * doctest::test_suite("regression"))
{
    CHECK(CommandPost::HoldMaskHasPlayerRetail(0x1ull, 0));
    CHECK(CommandPost::HoldMaskHasPlayerRetail(0x40000000ull, 30));
    CHECK_FALSE(CommandPost::HoldMaskHasPlayerRetail(0x100000000ull, 32));
    CHECK(CommandPost::HoldMaskHasPlayerRetail(0x1ull, 32));
    CHECK(CommandPost::HoldMaskHasPlayerRetail(0x2ull, 33));
    CHECK(CommandPost::HoldMaskHasPlayerRetail(0x80000000ull, 63));
    CHECK(CommandPost::HoldMaskHasPlayerRetail(0x8000000000000000ull, 31));
    CHECK(CommandPost::HoldMaskHasPlayerRetail(0x100000000ull, 31));
    CHECK_FALSE(CommandPost::HoldMaskHasPlayerRetail(0x7fffffffull, 31));
    CHECK_FALSE(CommandPost::HoldMaskHasPlayerRetail(0xfffffffeull, 0));
}

TEST_CASE("command post hold mask tests the player bit" * doctest::test_suite("regression"))
{
    for (int i = 0; i < 64; ++i)
    {
        const unsigned __int64 uBit = 1ull << i;
        CHECK(CommandPost::HoldMaskHasPlayer(uBit, i));
        CHECK_FALSE(CommandPost::HoldMaskHasPlayer(~uBit, i));
    }
    CHECK_FALSE(CommandPost::HoldMaskHasPlayer(0x1ull, 32));
    CHECK_FALSE(CommandPost::HoldMaskHasPlayer(0xffffffff00000000ull, 31));
    CHECK_FALSE(CommandPost::HoldMaskHasPlayer(~0ull, -1));
    CHECK_FALSE(CommandPost::HoldMaskHasPlayer(~0ull, 64));
}
