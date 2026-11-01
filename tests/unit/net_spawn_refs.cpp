#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>

#include "testutil.h"

namespace
{

std::string NetRefSource(const char* pcRel)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRel, sText));
    return sText;
}

std::string NetRefBody(const std::string& sText, const char* pcSignature)
{
    const std::string sOpen = std::string(pcSignature) + "\n{\n";
    size_t uiStart = sText.find(sOpen);
    REQUIRE_MESSAGE(uiStart != std::string::npos, pcSignature);
    REQUIRE_MESSAGE(sText.find(sOpen, uiStart + 1) == std::string::npos, pcSignature);
    size_t uiEnd = sText.find("\n}\n", uiStart);
    REQUIRE_MESSAGE(uiEnd != std::string::npos, pcSignature);
    return sText.substr(uiStart, uiEnd + 2 - uiStart);
}

int NetRefCount(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    const size_t uiLen = strlen(pcNeedle);
    for (size_t uiPos = sText.find(pcNeedle); uiPos != std::string::npos; uiPos = sText.find(pcNeedle, uiPos + uiLen))
    {
        ++iCount;
    }
    return iCount;
}

bool NetRefBefore(const std::string& sText, const char* pcFirst, const char* pcSecond)
{
    const size_t uiFirst = sText.find(pcFirst);
    const size_t uiSecond = sText.find(pcSecond);
    return uiFirst != std::string::npos && uiSecond != std::string::npos && uiFirst < uiSecond;
}

const char* const kNetGame = "src\\Battlefront2\\Source\\NetGame.cpp";

}

TEST_CASE("spawn select rejects unknown guest slots" * doctest::test_suite("regression"))
{
    const std::string sBody = NetRefBody(NetRefSource(kNetGame), "void ReadSpawnSelect(NetPktGroup& group, int player)");
    CHECK(NetRefCount(sBody, "LocalLimits::IsValidNetPlayer(id)") == 1);
    CHECK(NetRefCount(sBody, "group.SetBad(NET_REJECT_SPAWN_SLOT);") == 1);
    CHECK(NetRefBefore(sBody, "LocalLimits::IsValidNetPlayer(id)", "netPlayers[id]"));
    CHECK(NetRefBefore(sBody, "group.Read(hasSpawn);", "LocalLimits::IsValidNetPlayer(id)"));
}

TEST_CASE("team change event ignores null references" * doctest::test_suite("regression"))
{
    const std::string sBody = NetRefBody(NetRefSource(kNetGame), "void HandleEvent_ChangeTeam(ReliableNetEvent& event)");
    CHECK(NetRefBefore(sBody, "if (!post || !team)", "->"));
}

TEST_CASE("read create rejects unknown class and team" * doctest::test_suite("regression"))
{
    const std::string sText = NetRefSource(kNetGame);
    const std::string sCreate = NetRefBody(sText, "GameObject* ReadCreate(NetPktGroup& group, unsigned int uniqId)");
    CHECK(NetRefCount(sCreate, "group.SetBad(NET_REJECT_OBJECT_REF);\n        return 0;") == 3);
    CHECK(NetRefBefore(sCreate, "if (!entityClass)", "entityClass->IsRtti("));
    CHECK(NetRefBefore(sCreate, "if (!team)", "team->GetNumber()"));
    CHECK(NetRefBefore(sCreate, "if (!object)", "object->ReadCreate(group);"));

    const std::string sObjects = NetRefBody(sText, "void ReadObjects(NetPktGroup& group)");
    CHECK(NetRefCount(sObjects, "ReadCreate(group, index);\n                if (group.IsBad())\n                {\n                    break;") == 1);

    const std::string sObject = NetRefBody(sText, "bool ReadObject(NetPktGroup& group)");
    CHECK(NetRefCount(sObject, "object = ReadCreate(group, index);\n            if (!object)\n            {\n                return false;") == 1);
    CHECK(NetRefBefore(sObject, "const bool bObjectKnown = netObj != 0;", "netObj->usedTurn"));
    CHECK(NetRefCount(sObject, "if (!ensure(bObjectKnown))") == 1);

    const std::string sBuild = NetRefBody(sText,
        "GameObject* CreateObject(unsigned int uniqId, EntityClass* entityClass, int initialPlayerId, int playerId,\n"
        "                         PblQuaternion& rotation, PblVector3& position, Character* character, Team* team)");
    CHECK(NetRefBefore(sBuild, "if (!entityClass || !team)", "NetPredict::GetSmoothInfo(uniqId)"));
    CHECK(NetRefCount(sBuild, "Dump();\n        return 0;") == 1);
    CHECK(NetRefCount(sBuild, "if (player && controllable)") == 1);
    CHECK(NetRefCount(sBuild, "if (controllable && controllable->GetPilotType() == PILOT_SELF)") == 1);
    CHECK(NetRefCount(sBuild, "initialPlayerId != playerId && object->GetControllable()") == 1);
}

TEST_CASE("net object slots are bounded" * doctest::test_suite("regression"))
{
    const std::string sSmooth = NetRefBody(NetRefSource("src\\Battlefront2\\Source\\NetPredict.cpp"),
                                           "SmoothInfo& NetPredict::GetSmoothInfo(int iIndex)");
    CHECK(NetRefCount(sSmooth, "PblInBounds(smoothInfo, iIndex)") == 1);

    const std::string sObject = NetRefSource("src\\Battlefront2\\Source\\GameObject.cpp");
    int iMethods = 0;
    const char* pcPrefix = "GameObjectInterpolator::";
    for (size_t uiPos = sObject.find(pcPrefix); uiPos != std::string::npos; uiPos = sObject.find(pcPrefix, uiPos + 1))
    {
        const size_t uiLineStart = sObject.rfind('\n', uiPos) + 1;
        const size_t uiLineEnd = sObject.find('\n', uiPos);
        const std::string sLine = sObject.substr(uiLineStart, uiLineEnd - uiLineStart);
        if (sLine.find("int iNetUniqueId") == std::string::npos)
        {
            continue;
        }
        const size_t uiBodyEnd = sObject.find("\n}\n", uiPos);
        const std::string sBody = sObject.substr(uiPos, uiBodyEnd - uiPos);
        CHECK_MESSAGE(sBody.find("PblInBounds(count, iNetUniqueId)") != std::string::npos, sLine);
        CHECK_MESSAGE(sBody.find("iNetUniqueId < 0") == std::string::npos, sLine);
        CHECK_MESSAGE(sBody.find("iNetUniqueId >= 0") == std::string::npos, sLine);
        ++iMethods;
    }
    CHECK(iMethods == 9);
}

TEST_CASE("net message and chunk events ignore null references" * doctest::test_suite("regression"))
{
    const std::string sText = NetRefSource(kNetGame);
    const std::string sMessage = NetRefBody(sText, "void HandleEvent_Message(ReliableNetEvent& event)");
    CHECK(NetRefCount(sMessage, "case NetGame::MSG_SUICIDE:\n        if (!pCharacter1)") == 1);
    CHECK(NetRefCount(sMessage, "else if (pCharacter2)") == 1);
    CHECK(NetRefCount(sMessage, "bKill = pCharacter1 != 0;") == 1);
    CHECK(NetRefCount(sMessage, "case NetGame::MSG_DIED:\n        if (!pCharacter1)") == 1);
    CHECK(NetRefCount(sMessage, "if (pPost && pTeam)") == 4);
    CHECK(NetRefCount(sMessage, "if (!pCharacter1 || !LocalLimits::IsValidNetPlayer(iPlayer))") == 2);
    CHECK(NetRefCount(sMessage, "NetGame::GetLocalPlayer(j) && pTeam") == 1);

    const std::string sChunk = NetRefBody(sText, "void ReadEvent_CreateChunkCluster(NetPktGroup& group, ReliableNetEvent& event)");
    CHECK(NetRefBefore(sChunk, "PblInBounds(relChunkClusterData", "ReadEntityClass(group)"));
}

TEST_CASE("join guest count is bounded" * doctest::test_suite("regression"))
{
    const std::string sText = NetRefSource("src\\Battlefront2\\Source\\NetSetup.cpp");
    const std::string sUpdate = NetRefBody(sText, "bool ReadShellUpdate(NetPktGroup& group, ShellUpdate& update, ShellMove& move)");
    CHECK(NetRefBefore(sUpdate, "if (!LocalLimits::IsValidGuestCount(guestCount))", "int guests[1];"));
    CHECK(NetRefCount(sUpdate, "group.SetBad(NET_REJECT_GUEST_COUNT);") == 1);
    CHECK(NetRefCount(sText, "if (!LocalLimits::IsValidGuestCount(guestCount))") == 3);
    CHECK(NetRefCount(sText, "for (j = 0; hasRoom && j < guestCount; ++j)") == 2);
    CHECK(NetRefCount(sText, "for (j = 0; j < guestCount; ++j)") == 0);
}
