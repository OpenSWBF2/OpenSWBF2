#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string>

#include "nettest.h"
#include "testutil.h"

extern bool gGalaxyInitializedStub;

namespace
{
struct WanReadyFixture
{
    ConnectType eType;
    bool bGalaxy;

    WanReadyFixture() : eType(NetComm::GetConnectType()), bGalaxy(gGalaxyInitializedStub)
    {
    }

    ~WanReadyFixture()
    {
        NetComm::SetConnectType(eType);
        gGalaxyInitializedStub = bGalaxy;
    }
};
}

TEST_CASE_NAMED(FRegressionWanReadyTest1, "Net::RegressionWanReady::an online session is not ready without an initialized Galaxy", "[EngineFilter]")
{
    WanReadyFixture f;
    NetComm::SetConnectType(CT_WAN);
    gGalaxyInitializedStub = false;
    CHECK_FALSE(NetComm::IsWanReady());
    gGalaxyInitializedStub = true;
    CHECK(NetComm::IsWanReady());
}

TEST_CASE_NAMED(FRegressionWanReadyTest2, "Net::RegressionWanReady::a LAN session is ready without Galaxy", "[EngineFilter]")
{
    WanReadyFixture f;
    gGalaxyInitializedStub = false;
    NetComm::SetConnectType(CT_LAN);
    CHECK(NetComm::IsWanReady());
    NetComm::SetConnectType(CT_LINK);
    CHECK(NetComm::IsWanReady());
}

TEST_CASE_NAMED(FRegressionWanReadyTest3, "Net::RegressionWanReady::hosting a lobby refuses an online session without Galaxy before creating the host game (source pin)", "[EngineFilter]")
{
    std::string sText;
    if (!ReadRepoText("src\\Battlefront2\\Source\\LuaCallbacks_Net.cpp", sText))
    {
        SkipCase("LuaCallbacks_Net.cpp not found");
    }
    const size_t uiBegin = sText.find("int ScriptCB_BeginLobby(lua_State*)");
    REQUIRE(uiBegin != std::string::npos);
    const size_t uiHost = sText.find("hostGame = NetSetup::HostGame();", uiBegin);
    REQUIRE(uiHost != std::string::npos);
    const std::string sBody = sText.substr(uiBegin, uiHost - uiBegin);
    const size_t uiCheck = sBody.find("if (!NetComm::IsWanReady())");
    REQUIRE(uiCheck != std::string::npos);
    const size_t uiError = sBody.find("NetMatch::SetGameError(\"ifs.mp.galaxy.SIGNIN_SERVICE_NOT_AVAILABLE\", GuiManager::ERROR_LEAVE_SESSION);", uiCheck);
    CHECK(uiError != std::string::npos);
    CHECK(sBody.find("LOG(LogNet, Warning, \"online hosting needs a Galaxy login; hosting refused\");", uiCheck) < uiError);
    CHECK(sBody.find("return 0;", uiError) != std::string::npos);
}
