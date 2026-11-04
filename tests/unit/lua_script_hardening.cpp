#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <io.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "testutil.h"
#include "LuaFL/include/lua.h"
#include "LuaFL/include/lauxlib.h"
#include "LuaFL/include/lualib.h"
#include "Battlefront2/Source/Team.h"

Team** Team::sTeams;

namespace
{

std::string HardenSource(const char* pcRel)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRel, sText));
    return sText;
}

std::string HardenBody(const std::string& sText, const char* pcSignature)
{
    size_t uiStart = sText.find(pcSignature);
    REQUIRE_MESSAGE(uiStart != std::string::npos, pcSignature);
    REQUIRE_MESSAGE(sText.find(pcSignature, uiStart + 1) == std::string::npos, pcSignature);
    size_t uiEnd = sText.find("\n}\n", uiStart);
    REQUIRE_MESSAGE(uiEnd != std::string::npos, pcSignature);
    return sText.substr(uiStart, uiEnd + 2 - uiStart);
}

int HardenCount(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    const size_t uiLen = strlen(pcNeedle);
    for (size_t uiPos = sText.find(pcNeedle); uiPos != std::string::npos; uiPos = sText.find(pcNeedle, uiPos + uiLen))
    {
        ++iCount;
    }
    return iCount;
}

bool HardenHas(const std::string& sText, const char* pcNeedle)
{
    return sText.find(pcNeedle) != std::string::npos;
}

void CheckIndexedOnlyByLiteral(const std::string& sText, const char* pcOpen, char cClose)
{
    const size_t uiLen = strlen(pcOpen);
    for (size_t uiPos = sText.find(pcOpen); uiPos != std::string::npos; uiPos = sText.find(pcOpen, uiPos + uiLen))
    {
        const std::string sNext = sText.substr(uiPos + uiLen, 2);
        const bool bLiteral = sNext.size() == 2 && (sNext[0] == '1' || sNext[0] == '2') && sNext[1] == cClose;
        const std::string sSite = sText.substr(uiPos, 60);
        CHECK_MESSAGE(bLiteral, sSite);
    }
}

void CheckColorAfterBound(const std::string& sBody)
{
    const size_t uiBound = sBody.find("PblInBounds(");
    REQUIRE(uiBound != std::string::npos);
    CHECK(sBody.find("mColor", uiBound) != std::string::npos);
    size_t uiColor = sBody.find("GetColor(");
    if (uiColor == std::string::npos)
    {
        uiColor = sBody.find("mColor[");
    }
    REQUIRE(uiColor != std::string::npos);
    CHECK(uiColor > uiBound);
}

std::string WriteTempScript(const char* pcText)
{
    char acDir[MAX_PATH];
    char acPath[MAX_PATH];
    REQUIRE(GetTempPathA(sizeof(acDir), acDir) > 0);
    REQUIRE(GetTempFileNameA(acDir, "lsh", 0, acPath) != 0);
    FILE* pFile = fopen(acPath, "wb");
    REQUIRE(pFile != (FILE*)0);
    fputs(pcText, pFile);
    fclose(pFile);
    return acPath;
}

class StdinFromFile
{
public:
    explicit StdinFromFile(const std::string& sPath)
    {
        fflush(stdin);
        miSaved = _dup(_fileno(stdin));
        mpFile = freopen(sPath.c_str(), "r", stdin);
    }
    ~StdinFromFile()
    {
        if (miSaved >= 0)
        {
            _dup2(miSaved, _fileno(stdin));
            _close(miSaved);
        }
        clearerr(stdin);
    }
    bool IsOpen() const { return mpFile != (FILE*)0; }

private:
    int miSaved;
    FILE* mpFile;
};

bool GlobalIsNil(lua_State* L, const char* pcName)
{
    lua_pushstring(L, pcName);
    lua_gettable(L, LUA_GLOBALSINDEX);
    const bool bNil = lua_isnil(L, -1) != 0;
    lua_pop(L, 1);
    return bNil;
}

void RunIfFunction(lua_State* L)
{
    if (lua_isfunction(L, -1))
    {
        lua_pushvalue(L, -1);
        lua_pcall(L, 0, 0, 0);
    }
}

bool TableHasFunction(lua_State* L, int iIndex)
{
    bool bFound = false;
    lua_pushnil(L);
    while (lua_next(L, iIndex) != 0)
    {
        if (lua_type(L, -1) == LUA_TFUNCTION)
        {
            bFound = true;
        }
        lua_pop(L, 1);
    }
    return bFound;
}

}

TEST_CASE("lua loadfile dofile and require cannot read files" * doctest::test_suite("regression"))
{
    const std::string sPath = WriteTempScript("Pwned = 1\n");
    lua_State* L = lua_open();
    REQUIRE(L != (lua_State*)0);

    int iTop = lua_gettop(L);
    CHECK(luaL_loadfile(L, sPath.c_str()) == LUA_ERRFILE);
    CHECK(lua_gettop(L) == iTop + 1);
    RunIfFunction(L);
    REQUIRE(lua_isstring(L, -1));
    CHECK(strstr(lua_tostring(L, -1), "disabled") != (const char*)0);
    CHECK(GlobalIsNil(L, "Pwned"));

    {
        StdinFromFile kStdin(sPath);
        REQUIRE(kStdin.IsOpen());
        iTop = lua_gettop(L);
        CHECK(luaL_loadfile(L, 0) == LUA_ERRFILE);
        CHECK(lua_gettop(L) == iTop + 1);
        RunIfFunction(L);
        REQUIRE(lua_isstring(L, -1));
        CHECK(strstr(lua_tostring(L, -1), "disabled") != (const char*)0);
    }
    CHECK(GlobalIsNil(L, "Pwned"));

    lua_close(L);
    DeleteFileA(sPath.c_str());
}

TEST_CASE("lua debug debug does not read stdin" * doctest::test_suite("regression"))
{
    const std::string sPath = WriteTempScript("Pwned = 1\ncont\n");
    lua_State* L = lua_open();
    REQUIRE(L != (lua_State*)0);
    luaopen_debug(L);
    lua_settop(L, 0);

    {
        StdinFromFile kStdin(sPath);
        REQUIRE(kStdin.IsOpen());
        lua_pushstring(L, "debug");
        lua_gettable(L, LUA_GLOBALSINDEX);
        REQUIRE(lua_istable(L, -1));
        lua_pushstring(L, "debug");
        lua_gettable(L, -2);
        REQUIRE(lua_isfunction(L, -1));
        CHECK(lua_pcall(L, 0, 0, 0) == 0);
    }
    lua_settop(L, 0);
    CHECK(GlobalIsNil(L, "Pwned"));

    lua_close(L);
    DeleteFileA(sPath.c_str());
}

TEST_CASE("lua io and os libraries expose no functions" * doctest::test_suite("regression"))
{
    lua_State* L = lua_open();
    REQUIRE(L != (lua_State*)0);
    luaopen_io(L);
    lua_settop(L, 0);

    const char* apcLibs[2] = { "io", "os" };
    for (int i = 0; i < 2; ++i)
    {
        lua_pushstring(L, apcLibs[i]);
        lua_gettable(L, LUA_GLOBALSINDEX);
        REQUIRE(lua_istable(L, -1));
        CHECK_FALSE(TableHasFunction(L, lua_gettop(L)));
        lua_settop(L, 0);
    }

    const char* apcFiles[3] = { "stdin", "stdout", "stderr" };
    for (int i = 0; i < 3; ++i)
    {
        lua_pushstring(L, "io");
        lua_gettable(L, LUA_GLOBALSINDEX);
        lua_pushstring(L, apcFiles[i]);
        lua_gettable(L, -2);
        REQUIRE(lua_isuserdata(L, -1));
        REQUIRE(lua_getmetatable(L, -1) != 0);
        CHECK_FALSE(TableHasFunction(L, lua_gettop(L)));
        lua_settop(L, 0);
    }

    lua_close(L);
}

TEST_CASE("team lookup rejects out of range indices" * doctest::test_suite("regression"))
{
    Team* apSlots[10];
    for (int i = 0; i < 10; ++i)
    {
        apSlots[i] = (Team*)(0x1000 + i * 0x40);
    }
    Team** ppSaved = Team::sTeams;
    Team::sTeams = &apSlots[1];
    for (int i = 0; i < 8; ++i)
    {
        CHECK(Team::Lookup(i) == apSlots[1 + i]);
    }
    CHECK(Team::Lookup(-1) == (Team*)0);
    CHECK(Team::Lookup(8) == (Team*)0);
    CHECK(Team::Lookup(100) == (Team*)0);
    CHECK(Team::Lookup(INT_MIN) == (Team*)0);
    CHECK(Team::Lookup(INT_MAX) == (Team*)0);
    Team::sTeams = ppSaved;
}

TEST_CASE("lua team index sites route through team lookup" * doctest::test_suite("regression"))
{
    const char* apcFiles[4] = {
        "src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp",
        "src\\Battlefront2\\Source\\LuaCallbacks.cpp",
        "src\\Battlefront2\\Source\\LuaCallbacks_Sound.cpp",
        "src\\Battlefront2\\Source\\LuaCallbacks_Stats.cpp",
    };
    for (int i = 0; i < 4; ++i)
    {
        const std::string sText = HardenSource(apcFiles[i]);
        CheckIndexedOnlyByLiteral(sText, "Team::sTeams[", ']');
        CheckIndexedOnlyByLiteral(sText, "Team::GetPtr(", ')');
    }

    const std::string sMission = HardenSource("src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp");
    const std::string sMember = HardenBody(sMission, "int GetTeamMember(lua_State*");
    CHECK(HardenHas(sMember, "Team::Lookup("));
    CHECK(HardenHas(sMember, "iMember >= 0"));
    CHECK(HardenHas(sMember, "-1.0f"));

    CHECK(HardenCount(HardenBody(sMission, "int SetTeamAsEnemy(lua_State*"), "Team::Lookup(") == 2);
    CHECK(HardenCount(HardenBody(sMission, "int SetTeamAsNeutral(lua_State*"), "Team::Lookup(") == 2);
    CHECK(HardenCount(HardenBody(sMission, "int SetTeamAsFriend(lua_State*"), "Team::Lookup(") == 2);
    CHECK(HardenHas(HardenBody(sMission, "int SetObjectTeam(lua_State*"), "Team::Lookup("));
    CHECK(HardenHas(HardenBody(sMission, "int SetUnitCount(lua_State*"), "Team::Lookup("));
    CHECK(HardenHas(HardenBody(sMission, "int GetUnitCount(lua_State*"), "Team::Lookup("));
    CHECK(HardenHas(HardenBody(sMission, "int GetCommandPostBleedValue(lua_State*"), "Team::Lookup(iTeam)"));
    CHECK(HardenHas(HardenBody(sMission, "int UnlockHeroForTeam(lua_State*"), "PblInBounds(netHeroUnlocked"));

    const std::string sBatch = HardenBody(sMission, "int BatchChangeTeams(lua_State*");
    CHECK(HardenHas(sBatch, "Team::Lookup(iFrom)"));
    CHECK(HardenHas(sBatch, "Team::Lookup(iTo)"));
    CHECK(sBatch.find("Team::Lookup(iTo)") < sBatch.find("mMaxUnitCount[iTo]"));

    const std::string sChangeTeam =
        HardenBody(HardenSource("src\\Battlefront2\\Source\\Character.cpp"), "bool Character::ChangeTeam(int iTeam)");
    CHECK(HardenHas(sChangeTeam, "Team::Lookup(iTeam)"));
    CHECK_FALSE(HardenHas(sChangeTeam, "Team::sTeams["));

    const std::string sShowPoints =
        HardenBody(HardenSource("src\\Battlefront2\\Source\\GameLoop.h"), "inline void SetShowTeamPoints(");
    CHECK(HardenHas(sShowPoints, "PblInBounds(mShowTeamPoints"));

    const std::string sBonus = HardenBody(HardenSource("src\\Battlefront2\\Source\\BonusManager.cpp"),
                                          "void BonusManager::SetTeamBonusActive(");
    CHECK(sBonus.find("iTeam < 1 || iTeam > 2") < sBonus.find("mTeamBonuses["));

    const std::string sStats = HardenSource("src\\Battlefront2\\Source\\LuaCallbacks_Stats.cpp");
    CHECK(HardenHas(HardenBody(sStats, "int ScriptCB_GetMedalStats(lua_State*"), "pMedals && i < 9"));
    CHECK(HardenHas(HardenBody(sStats, "int ScriptCB_GetPersonalStats(lua_State*"), "pStats && i < 10"));
    const std::string sTitles = HardenBody(sStats, "int ScriptCB_TeamStatsSetTitles(lua_State*");
    CHECK(sTitles.find("if (!pTeam1 || !pTeam2)") < sTitles.find("NetGame::GetScore("));
    CHECK(HardenCount(sStats, "Team::sTeams[1] && Team::sTeams[2] &&") == 2);
}

TEST_CASE("stats team colour slots are bounded" * doctest::test_suite("regression"))
{
    const std::string sStats = HardenSource("src\\Battlefront2\\Source\\LuaCallbacks_Stats.cpp");
    CheckColorAfterBound(HardenBody(sStats, "int ScriptCB_TeamStatsGetTeamColor(lua_State*"));
    CheckColorAfterBound(HardenBody(sStats, "int ScriptCB_TeamStatsSetTitles(lua_State*"));
    CheckColorAfterBound(HardenBody(sStats, "int ScriptCB_PersonalStatsSetTitles(lua_State*"));
}

TEST_CASE("voice over side indices are bounded" * doctest::test_suite("regression"))
{
    const std::string sVo = HardenSource("src\\Battlefront2\\Source\\VoiceOverManager.cpp");
    CHECK(HardenHas(HardenBody(sVo, "void VoiceOverManager::SetBleedingVoiceOver("), "PblInBounds("));
    CHECK(HardenHas(HardenBody(sVo, "void VoiceOverManager::SetLowReinforcementsVoiceOver("), "PblInBounds("));
    CHECK(HardenHas(HardenBody(sVo, "void VoiceOverManager::SetAmbientMusic("), "PblInBounds("));
    CHECK(HardenHas(HardenBody(sVo, "void VoiceOverManager::SetAttackingTeam("), "PblInBounds("));

    const std::string sUpdate = HardenBody(sVo, "bool VoiceOverManager::Update(");
    CHECK(HardenHas(sUpdate, "PblInBounds(mTeamBleeding, musicTeam)"));
    CHECK(HardenCount(sUpdate, "if (bMusicSide)") == 2);
    CHECK(HardenHas(HardenBody(sVo, "void VoiceOverManager::PlayVoiceOverBleeding("), "PblInBounds(mTeamBleeding"));
    CHECK(HardenHas(HardenBody(sVo, "void VoiceOverManager::PlayVoiceOverLowReinforcements("), "PblInBounds(mTeamBleeding"));

    const std::string sPost = HardenBody(HardenSource("src\\Battlefront2\\Source\\CommandPost.cpp"),
                                         "void CommandPost::PlayVoiceOver(");
    CHECK(HardenHas(sPost, "Team::Lookup("));
    CHECK_FALSE(HardenHas(sPost, "Team::sTeams["));
}

TEST_CASE("lua stream and ui indices are bounded" * doctest::test_suite("regression"))
{
    const std::string sSound = HardenSource("src\\Battlefront2\\Source\\LuaCallbacks_Sound.cpp");
    const std::string sHelper = HardenBody(sSound, "static Snd::Stream* GetLuaStream(int iStream)");
    CHECK(HardenHas(sHelper, "(unsigned int)(iStream - 1) < 6u"));
    const char* apcStreams[3] = {
        "int ScriptCB_CloseAudioStream(lua_State*",
        "int ScriptCB_AudioStreamComplete(lua_State*",
        "int ScriptCB_PlayAudioStream(lua_State*",
    };
    for (int i = 0; i < 3; ++i)
    {
        const std::string sBody = HardenBody(sSound, apcStreams[i]);
        CHECK(HardenHas(sBody, "GetLuaStream("));
        CHECK_FALSE(HardenHas(sBody, " - 1]"));
    }

    const std::string sMain = HardenSource("src\\Battlefront2\\Source\\LuaCallbacks.cpp");
    const std::string sPages = HardenBody(sMain, "int ScriptCB_ShowMultipageText(lua_State*");
    CHECK(HardenHas(sPages, "PblInBounds(sPageStart, iPage - 1)"));
    CHECK(HardenHas(sPages, "if (!pTextElement)"));
    CHECK(HardenHas(HardenBody(sMain, "int ScriptCB_IndexMultipageText(lua_State*"), "pTextElement && i < 300"));
    CHECK(HardenHas(HardenBody(sMain, "int ScriptCB_MetagameSetSide(lua_State*"),
                    "iIndex != (int)iIndex || !PblInBounds(GameLoop::mMetagameSide, (int)iIndex)"));

    const std::string sIFace = HardenSource("src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp");
    const char* apcBindings[2] = { "int ScriptCB_SetBinding(lua_State*", "int ScriptCB_GetActionFromIdx(lua_State*" };
    for (int i = 0; i < 2; ++i)
    {
        const std::string sBody = HardenBody(sIFace, apcBindings[i]);
        CHECK(HardenHas(sBody, "iIndex < 0"));
        CHECK(HardenCount(sBody, "PblInBounds(sKeyboardCmdsPerMode[") == 2);
    }

    const std::string sOptions = HardenSource("src\\Battlefront2\\Source\\LuaCallbacks_Options.cpp");
    CHECK(HardenHas(HardenBody(sOptions, "int ScriptCB_SetControlScale(lua_State*"),
                    "PblInBounds(pProfile->mGameOptions.mControllerScale"));
    CHECK(HardenHas(HardenBody(sOptions, "int ScriptCB_GetControlScale(lua_State*"),
                    "PblInBounds(pProfile->mGameOptions.mControllerScale"));
    CHECK(HardenHas(HardenBody(sOptions, "int ScriptCB_SetSpectatorMode(lua_State*"), "PblInBounds(g_SpectatorEnabled"));
    CHECK(HardenHas(HardenBody(sOptions, "int ScriptCB_GetSpectatorMode(lua_State*"), "PblInBounds(g_SpectatorEnabled"));
}

TEST_CASE("create timer refuses when the timer map is full" * doctest::test_suite("regression"))
{
    const std::string sMission = HardenSource("src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp");
    const std::string sBody = HardenBody(sMission, "int CreateTimer(lua_State*");
    CHECK(HardenHas(sBody, "!Timer::sMemoryPool.IsFull() && Timer::sMap.GetNumEntries() < Timer::sMap.GetMaxEntries()"));
}

TEST_CASE("delete entity refuses character owned entities" * doctest::test_suite("regression"))
{
    const std::string sMission = HardenSource("src\\Battlefront2\\Source\\LuaCallbacks_Mission.cpp");
    const std::string sBody = HardenBody(sMission, "int DeleteEntity(lua_State*");
    const size_t uiOwner = sBody.find("Character::FindOwner(pEntity)");
    REQUIRE(uiOwner != std::string::npos);
    CHECK(uiOwner < sBody.find("delete pEntity;"));
    CHECK(HardenHas(sBody, "LOG(LogLua, Warning,"));

    const std::string sOwner = HardenBody(HardenSource("src\\Battlefront2\\Source\\Character.cpp"),
                                          "Character* Character::FindOwner(const EntityEx* pEntity)");
    CHECK(HardenHas(sOwner, "mUnit"));
    CHECK(HardenHas(sOwner, "mVehicle"));
    CHECK(HardenHas(sOwner, "mRemote"));
    CHECK(HardenHas(sOwner, "mCarriedFlagPtr"));
    CHECK(HardenHas(sOwner, "i < sCharacterLimit"));
}

TEST_CASE("screen group fallback returns true when set" * doctest::test_suite("regression"))
{
    const std::string sIFace = HardenSource("src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp");
    const std::string sBody = HardenBody(sIFace, "bool Util_GetRedScreenGroupElement(");
    CHECK(HardenHas(sBody, "        pElement = pCurCreateScreenGroup;\n    }\n    return pElement != 0;\n}"));
}
