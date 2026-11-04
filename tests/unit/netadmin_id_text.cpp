#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>

#include "testutil.h"
#include "Battlefront2/Source/NetAdminIdText.h"
#include "Battlefront2/Source/NetComm.h"

namespace
{

void CheckIdText(unsigned __int64 uAddr, const char* pcExpected)
{
    char acBuf[32];
    memset(acBuf, 0x5A, sizeof(acBuf));
    const int iLen = FormatAdminIdText(acBuf, uAddr);
    CAPTURE(pcExpected);
    CHECK(iLen == (int)strlen(pcExpected));
    CHECK(memcmp(acBuf, pcExpected, strlen(pcExpected) + 1) == 0);
}

int CountText(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    const std::string sNeedle(pcNeedle);
    for (size_t uiAt = sText.find(sNeedle); uiAt != std::string::npos; uiAt = sText.find(sNeedle, uiAt + 1))
    {
        ++iCount;
    }
    return iCount;
}

}

TEST_CASE("admin id text keeps the retail low-dword format" * doctest::test_suite("regression"))
{
    CheckIdText(0ull, "0ll");
    CheckIdText(0x000000017F000001ull, "2130706433ll");
    CheckIdText(0xFFFFFFFF12345678ull, "305419896ll");
    CheckIdText(0xFFFFFFFFFFFFFFFFull, "4294967295ll");
}

TEST_CASE("admin id text sites use the pinned format" * doctest::test_suite("regression"))
{
    std::string sAdmin;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\NetAdmin.cpp", sAdmin));
    CHECK(CountText(sAdmin, "FormatAdminIdText(") == 3);
    CHECK(sAdmin.find("FormatAdminIdText(adminPlayerIdText, GetAdminPeer(i));") != std::string::npos);
    CHECK(sAdmin.find("FormatAdminIdText(adminBanIdText, GetAdminPeer(iTarget));") != std::string::npos);
    CHECK(sAdmin.find("FormatAdminIdText(adminBanKeyIdText, GetAdminPeer(i));") != std::string::npos);
    CHECK(sAdmin.find("\"%ull\"") == std::string::npos);

    std::string sHeader;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\NetAdminIdText.h", sHeader));
    CHECK(sHeader.find("\"%ull\", (unsigned int)") != std::string::npos);
}

TEST_CASE("net peer lookup covers dests and join slots only" * doctest::test_suite("regression"))
{
    for (int i = -2; i < 140; ++i)
    {
        CAPTURE(i);
        const NetSocketPeerRecord* pPeer = NetCommP::GetPeer(i);
        const bool bValid = (i >= 0 && i < MAX_DESTS) || (i >= 69 && i < 69 + 64);
        CHECK((pPeer != 0) == bValid);
        if (pPeer)
        {
            CHECK(&pPeer->mGalaxyId.value == NetCommP::GetAddr(i));
        }
    }
}

TEST_CASE("admin replies route to the admin peer" * doctest::test_suite("regression"))
{
    std::string sAdmin;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\NetAdmin.cpp", sAdmin));
    CHECK(CountText(sAdmin, "const int iDest = FindAdminDest(admin.GetPeer());") == 1);
    CHECK(CountText(sAdmin, "FindJoinSource(") == 1);
    CHECK(CountText(sAdmin, "NetCommP::GetAddr(") == 0);
    const size_t uPeer = sAdmin.find("static unsigned __int64 GetAdminPeer(const NetSocketPeerRecord& kPeer)");
    REQUIRE(uPeer != std::string::npos);
    const size_t uEnd = sAdmin.find("\n}", uPeer);
    REQUIRE(uEnd != std::string::npos);
    const std::string sBody = sAdmin.substr(uPeer, uEnd - uPeer);
    CHECK(sBody.find("if (!NetComm::IsLAN())") != std::string::npos);
    CHECK(sBody.find("NetSocket::GetAddr(&kPeer, iIP, iPort);") != std::string::npos);
}
