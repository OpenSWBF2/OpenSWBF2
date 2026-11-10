#include "third_party/doctest/doctest.h"

#include "Battlefront2/Source/Thread.h"
#include "Battlefront2/Source/CollisionObject.h"
#include "Battlefront2/Source/EntityCarrier.h"
#include "Battlefront2/Source/WobblePhase.h"
#include "PebbleFL/Common/PblSerial.h"
#include "RedEngineFL/Util/RedLog.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "testutil.h"

namespace
{

template <class KEY>
struct SentinelFixture
{
    typename PblOrderedList<Thread, KEY>::Node aNodes[4];
    PblOrderedList<Thread, KEY> list;
};

int CountOf(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    std::string::size_type uiAt = sText.find(pcNeedle);
    while (uiAt != std::string::npos)
    {
        ++iCount;
        uiAt = sText.find(pcNeedle, uiAt + 1);
    }
    return iCount;
}

template <class LIST>
int Walk(const LIST& list, typename LIST::Node** apOut, int iMax)
{
    int iCount = 0;
    typename LIST::Node* pNode = list._head._pNext;
    while (pNode != &list._head && iCount < iMax)
    {
        apOut[iCount++] = pNode;
        pNode = pNode->_pNext;
    }
    if (pNode != &list._head)
    {
        return -1;
    }
    return iCount;
}

template <class KEY>
void CheckSentinelStopsAtHead(KEY sentinel)
{
    typedef PblOrderedList<Thread, KEY> List;
    SentinelFixture<KEY> fix;
    fix.list.Add(&fix.aNodes[0], NULL, (KEY)1);
    for (int i = 3; i >= 1; --i)
    {
        fix.list.Add(&fix.aNodes[i], NULL, sentinel);
    }

    typename List::Node* apGot[8];
    REQUIRE(Walk(fix.list, apGot, 8) == 4);
    CHECK(fix.list._iCount == 4);
    CHECK(apGot[0] == &fix.aNodes[0]);
    CHECK(apGot[1] == &fix.aNodes[3]);
    CHECK(apGot[2] == &fix.aNodes[2]);
    CHECK(apGot[3] == &fix.aNodes[1]);
    CHECK(apGot[3]->_pNext == &fix.list._head);
}

struct alignas(CollisionObject) CollisionObjectBuffer
{
    unsigned char acBytes[sizeof(CollisionObject)];
};

char sLastLog[1100];

bool LastLogHas(const char* pcText)
{
    return strstr(sLastLog, pcText) != NULL;
}

void __cdecl RecordLog(int iSeverity, int iCategory, char const* pcLine)
{
    strncpy(sLastLog, pcLine, sizeof(sLastLog) - 1);
    sLastLog[sizeof(sLastLog) - 1] = 0;
    TestLogObserver(iSeverity, iCategory, pcLine);
}

class TieLogCapture
{
public:
    TieLogCapture()
    {
        RedLog::ResetForTest();
        sLastLog[0] = 0;
        mOld = RedLog::gObserver;
        RedLog::gObserver = RecordLog;
    }
    ~TieLogCapture()
    {
        RedLog::gObserver = mOld;
        RedLog::ResetForTest();
    }

private:
    RedLog::Observer mOld;
};

template <class T>
struct alignas(T) RawObject
{
    unsigned char acBytes[sizeof(T)];
};

RawObject<GameObject> sRawGameObject;
RawObject<EntityFlyer> sRawFlyer;
RawObject<EntityCarrier> sRawCarrier;

const GameObject* RawOwner(void* pRaw, unsigned int uiId)
{
    memcpy((char*)pRaw + 0x204, &uiId, sizeof(uiId));
    return reinterpret_cast<const GameObject*>(pRaw);
}

template <class T>
void CheckFollowerOwnerPhase(RawObject<T>& raw, unsigned int uiId)
{
    memset(&raw, 0, sizeof(raw));
    memcpy(raw.acBytes + 0x204, &uiId, sizeof(uiId));
    EntityFlyer* pFlyer = reinterpret_cast<T*>(&raw);
    CHECK((void*)static_cast<GameObject*>(pFlyer) == (void*)&raw);
    EntityPathFollower* pFollower = static_cast<EntityPathFollower*>(pFlyer);
    CHECK(static_cast<EntityFlyer*>(pFollower) == pFlyer);
    CHECK(WobblePhase(static_cast<const GameObject*>(static_cast<EntityFlyer*>(pFollower))) ==
          (float)(uiId & 0xf));
}

std::string LineWith(const std::string& sText, const char* pcNeedle)
{
    std::string::size_type uiAt = sText.find(pcNeedle);
    if (uiAt == std::string::npos)
    {
        return std::string();
    }
    std::string::size_type uiBegin = sText.rfind('\n', uiAt);
    uiBegin = uiBegin == std::string::npos ? 0 : uiBegin + 1;
    std::string::size_type uiEnd = sText.find('\n', uiAt);
    return sText.substr(uiBegin, uiEnd == std::string::npos ? std::string::npos : uiEnd - uiBegin);
}

}

TEST_CASE("collision serial compare ignores object addresses under serial order" * doctest::test_suite("regression"))
{
    CHECK(PblSerialAfter(2, 1));
    CHECK_FALSE(PblSerialAfter(1, 2));
    CHECK_FALSE(PblSerialAfter(1, 1));

    CollisionObjectBuffer aBuf[2];
    memset(aBuf, 0, sizeof(aBuf));
    CollisionObject* pLow = reinterpret_cast<CollisionObject*>(&aBuf[0]);
    CollisionObject* pHigh = reinterpret_cast<CollisionObject*>(&aBuf[1]);
    pLow->mCollisionSerial = 2;
    pHigh->mCollisionSerial = 1;

    CHECK(CollisionObject::CollisionSerialAfter(pLow, pHigh));
    CHECK_FALSE(CollisionObject::CollisionSerialAfter(pHigh, pLow));
    CHECK(pLow->mProcedurallyAnimated == 0);
    CHECK(pLow->mLastCollidedWithNonTerrain == 0);

#ifdef OPENSWBF2_TEST
    TieLogCapture capture;
    CollisionObject::LogSerialTies(true);
    CollisionObject::LogSerialTies(false);
    CHECK(LastLogHas("OPENSWBF2 COLLISION serialties=0"));

    CollisionObject::NoteCollisionSerialTie(pLow, pHigh);
    CollisionObject::LogSerialTies(false);
    CHECK(LastLogHas("OPENSWBF2 COLLISION serialties=0"));

    pHigh->mCollisionSerial = 2;
    CollisionObject::NoteCollisionSerialTie(pLow, pHigh);
    CollisionObject::LogSerialTies(false);
    CHECK(LastLogHas("OPENSWBF2 COLLISION serialties=1"));

    CollisionObject::LogSerialTies(true);
    CHECK(LastLogHas("OPENSWBF2 COLLISION serialties=1"));
    CollisionObject::LogSerialTies(false);
    CHECK(LastLogHas("OPENSWBF2 COLLISION serialties=0"));
#endif
}

TEST_CASE("last collided key does not follow a dead receiver to a new object at its address" * doctest::test_suite("regression"))
{
    CollisionObjectBuffer aBuf[3];
    memset(aBuf, 0, sizeof(aBuf));
    CollisionObject* pCollider = reinterpret_cast<CollisionObject*>(&aBuf[0]);
    CollisionObject* pReceiver = reinterpret_cast<CollisionObject*>(&aBuf[1]);
    CollisionObject* pOther = reinterpret_cast<CollisionObject*>(&aBuf[2]);
    pReceiver->mCollisionSerial = 7;
    pOther->mCollisionSerial = 8;

    pCollider->NoteLastCollided(pReceiver);
    CollisionObject* apLive[2] = { pOther, pReceiver };
    pCollider->PrioritizeLastCollided(apLive, 2);
    CHECK(apLive[0] == pReceiver);
    CHECK(apLive[1] == pOther);

    memset(&aBuf[1], 0, sizeof(aBuf[1]));
    pReceiver->mCollisionSerial = 9;
    CollisionObject* apReused[2] = { pOther, pReceiver };
    pCollider->PrioritizeLastCollided(apReused, 2);
    CHECK(apReused[0] == pOther);
    CHECK(apReused[1] == pReceiver);
}

TEST_CASE("last collided record keeps three recent receivers without duplicates" * doctest::test_suite("regression"))
{
    CollisionObjectBuffer aBuf[5];
    memset(aBuf, 0, sizeof(aBuf));
    CollisionObject* pCollider = reinterpret_cast<CollisionObject*>(&aBuf[0]);
    CollisionObject* apReceiver[4];
    for (int i = 0; i < 4; ++i)
    {
        apReceiver[i] = reinterpret_cast<CollisionObject*>(&aBuf[i + 1]);
        apReceiver[i]->mCollisionSerial = i + 1;
    }

    pCollider->NoteLastCollided(apReceiver[0]);
    pCollider->NoteLastCollided(apReceiver[0]);
    CHECK(pCollider->mLastCollidedObjs[0] == 1);
    CHECK(pCollider->mLastCollidedObjs[1] == 0);
    pCollider->NoteLastCollided(apReceiver[1]);
    pCollider->NoteLastCollided(apReceiver[2]);
    pCollider->NoteLastCollided(apReceiver[3]);
    CHECK(pCollider->mLastCollidedObjs[0] == 4);
    CHECK(pCollider->mLastCollidedObjs[1] == 3);
    CHECK(pCollider->mLastCollidedObjs[2] == 2);
}

TEST_CASE("last collided key treats a null list entry as an empty slot" * doctest::test_suite("regression"))
{
    CollisionObjectBuffer aBuf[2];
    memset(aBuf, 0, sizeof(aBuf));
    CollisionObject* pCollider = reinterpret_cast<CollisionObject*>(&aBuf[0]);
    CollisionObject* pObject = reinterpret_cast<CollisionObject*>(&aBuf[1]);
    pObject->mCollisionSerial = 3;
    CHECK(CollisionObject::LastCollidedKey(0) == 0);
    CollisionObject* apList[2] = { pObject, 0 };
    pCollider->PrioritizeLastCollided(apList, 2);
    CHECK(apList[0] == pObject);
    CHECK(apList[1] == static_cast<CollisionObject*>(0));
}

TEST_CASE("last collided sites all go through the key helpers (source pin)" * doctest::test_suite("regression"))
{
    std::string sModel;
    std::string sManager;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\CollisionModel.cpp", sModel));
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\CollisionManager.cpp", sManager));
    int iNotes = 0;
    const std::string sNote = "pCollider->NoteLastCollided(pReceiver);";
    for (std::string::size_type uiAt = sModel.find(sNote); uiAt != std::string::npos; uiAt = sModel.find(sNote, uiAt + 1))
    {
        ++iNotes;
    }
    CHECK(iNotes == 3);
    CHECK(sManager.find("obj->PrioritizeLastCollided(data.mArray, data.mLength);") != std::string::npos);
    CHECK(sModel.find("mLastCollidedObjs") == std::string::npos);
    CHECK(sManager.find("mLastCollidedObjs") == std::string::npos);
}

TEST_CASE("collision serial sites keep the wire bool writes" * doctest::test_suite("regression"))
{
    std::string sNetState;
    std::string sManager;
    std::string sObject;
    std::string sState;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\NetState.cpp", sNetState));
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\CollisionManager.cpp", sManager));
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\CollisionObject.cpp", sObject));
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\GameState.cpp", sState));

    CHECK(CountOf(sNetState, "group.Write((bool)mLastCollidedWithNonTerrain);") == 4);
    CHECK(CountOf(sNetState, "group.Write(mLastCollidedWithNonTerrain)") == 0);
    CHECK(CountOf(sManager, "CollisionSerialAfter(pOther, pObject)") == 1);
    CHECK(CountOf(sObject, "AssignCollisionSerial();") == 1);
    CHECK(sObject.find("    mProcedurallyAnimated = false;\n"
                       "    AssignCollisionSerial();\n")
          != std::string::npos);
    CHECK(sState.find("    Thread::Reset();\n    CollisionObject::ResetCollisionSerials();\n") != std::string::npos);
}

TEST_CASE("ordered list breaks equal keys fifo under serial order" * doctest::test_suite("regression"))
{
    PblOrderedList<Thread, int> list;
    PblOrderedList<Thread, int>::Node aNodes[5];
    const int aiKey[] = { 2, 3, 3, 1, 3 };
    for (int i = 4; i >= 0; --i)
    {
        list.Add(&aNodes[i], NULL, aiKey[i]);
    }

    PblOrderedList<Thread, int>::Node* apGot[8];
    REQUIRE(Walk(list, apGot, 8) == 5);
    CHECK(apGot[0] == &aNodes[3]);
    CHECK(apGot[1] == &aNodes[0]);
    CHECK(apGot[2] == &aNodes[4]);
    CHECK(apGot[3] == &aNodes[2]);
    CHECK(apGot[4] == &aNodes[1]);
}

TEST_CASE("ordered list sentinel key stops at the head under serial order" * doctest::test_suite("regression"))
{
    CheckSentinelStopsAtHead<int>(0x7FFFFFFF);
    CheckSentinelStopsAtHead<float>(2147483648.0f);
}

TEST_CASE("thread activation order ignores node addresses under serial order" * doctest::test_suite("regression"))
{
    PblOrderedList<Thread, int> list;
    PblOrderedList<Thread, int>::Node aNodes[2];
    list.Add(&aNodes[1], NULL, 0);
    list.Add(&aNodes[0], NULL, 1);

    PblOrderedList<Thread, int>::Node* apGot[4];
    REQUIRE(Walk(list, apGot, 4) == 2);
    CHECK(apGot[0] == &aNodes[1]);
    CHECK(apGot[1] == &aNodes[0]);
}

TEST_CASE("thread activation key is set through one counter reader and reset" * doctest::test_suite("regression"))
{
    std::string sThread;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\Thread.cpp", sThread));

    CHECK(CountOf(sThread, "NextThreadActivationKey()") == 2);
    CHECK(CountOf(sThread, "(int)this") == 0);
    CHECK(CountOf(sThread, "sThreadActivationKey") == 5);
    CHECK(sThread.find("return (int)(sThreadActivationKey < 0x7FFFFFFF ? sThreadActivationKey++ : sThreadActivationKey);") != std::string::npos);
    CHECK(sThread.find("pGroup->mThreads.Add(&mThreadListNode, this, NextThreadActivationKey());") != std::string::npos);

    std::string::size_type uiReset = sThread.find("void Thread::Reset()");
    REQUIRE(uiReset != std::string::npos);
    std::string::size_type uiEnd = sThread.find("\n}", uiReset);
    REQUIRE(uiEnd != std::string::npos);
    CHECK(sThread.substr(uiReset, uiEnd - uiReset).find("sThreadActivationKey = 0;") != std::string::npos);
}

TEST_CASE("wobble phase comes from the owner handle id under serial order" * doctest::test_suite("regression"))
{
    const unsigned int auiId[] = { 0x13u, 0x20u, 0x7FFFu };
    for (size_t j = 0; j < sizeof(auiId) / sizeof(auiId[0]); ++j)
    {
        const GameObject* pOwner = RawOwner(&sRawGameObject, auiId[j]);
        CHECK(WobblePhase(pOwner) == (float)(auiId[j] & 0xf));
    }
    CheckFollowerOwnerPhase(sRawFlyer, 0x13u);
    CheckFollowerOwnerPhase(sRawFlyer, 0x7FFFu);
    CheckFollowerOwnerPhase(sRawCarrier, 0x20u);
    CheckFollowerOwnerPhase(sRawCarrier, 0x7FFFu);
}

TEST_CASE("wobble sites route through the phase helper" * doctest::test_suite("regression"))
{
    std::string sFlyer;
    std::string sFollower;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\EntityFlyer.cpp", sFlyer));
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\EntityPathFollower.cpp", sFollower));

    const char* pcFlyerCall = "    float fTime = GameLoop::GetMissionTime() + "
                              "WobblePhase(static_cast<const GameObject*>(this));";
    const char* pcFollowerCall = "    float fTime = GameLoop::GetMissionTime() +\n"
                                 "                  "
                                 "WobblePhase(static_cast<const GameObject*>(static_cast<EntityFlyer*>(this)));";
    CHECK(CountOf(sFlyer, pcFlyerCall) == 1);
    CHECK(CountOf(sFollower, pcFollowerCall) == 1);
    CHECK(CountOf(sFlyer, "WobblePhase(") == 1);
    CHECK(CountOf(sFollower, "WobblePhase(") == 1);
    CHECK(CountOf(sFlyer, "((unsigned int)this >> 2) & 0xf") == 0);
    CHECK(CountOf(sFollower, "((unsigned int)this >> 2) & 0xf") == 0);
    CHECK(LineWith(sFlyer, "WobblePhase(").find("GetBranchParams") == std::string::npos);
    CHECK(LineWith(sFollower, "WobblePhase(").find("GetBranchParams") == std::string::npos);
    CHECK(CountOf(sFollower, "GetBranchParams()") == 8);
}

TEST_CASE("ordered list move keeps an unchanged key in place while a walk re-queues it" * doctest::test_suite("regression"))
{
    PblOrderedList<Thread, float> list;
    PblOrderedList<Thread, float>::Node aNodes[4];
    list.Add(&aNodes[0], NULL, 1.0f);
    list.Add(&aNodes[1], NULL, 5.0f);
    list.Add(&aNodes[2], NULL, 5.0f);
    list.Add(&aNodes[3], NULL, 5.0f);

    int iVisits = 0;
    PblOrderedList<Thread, float>::Node* pNode = list.GetFirst();
    while (pNode != &list._head && iVisits < 16)
    {
        PblOrderedList<Thread, float>::Node* pNext = pNode->_pNext;
        list.Move(pNode, NULL, pNode->_iOrder < 5.0f ? pNode->_iOrder : 5.0f);
        ++iVisits;
        pNode = pNext;
    }
    CHECK(iVisits == 4);

    PblOrderedList<Thread, float>::Node* apGot[8];
    REQUIRE(Walk(list, apGot, 8) == 4);
    for (int i = 0; i < 4; ++i)
    {
        CHECK(apGot[i] == &aNodes[i]);
    }
    CHECK(list.GetCount() == 4);

    list.Move(&aNodes[3], NULL, 1.0f);
    REQUIRE(Walk(list, apGot, 8) == 4);
    CHECK(apGot[0] == &aNodes[0]);
    CHECK(apGot[1] == &aNodes[3]);
    CHECK(apGot[2] == &aNodes[1]);
    CHECK(apGot[3] == &aNodes[2]);
    CHECK(aNodes[3]._iOrder == 1.0f);
    CHECK(aNodes[3]._pList == &list);
    CHECK(list.GetCount() == 4);
}

TEST_CASE("controller update asap reschedules through the ordered list move" * doctest::test_suite("regression"))
{
    std::string sManager;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\ControllerManager.cpp", sManager));

    std::string::size_type uiBegin = sManager.find("void ControllerManager::RequestControllerUpdateASAP(");
    REQUIRE(uiBegin != std::string::npos);
    std::string::size_type uiEnd = sManager.find("\n}", uiBegin);
    REQUIRE(uiEnd != std::string::npos);
    const std::string sBody = sManager.substr(uiBegin, uiEnd - uiBegin);
    CHECK(sBody.find("sAICtrls->Move(pCtrl->GetManagerNode(), pCtrl, pCtrl->mNextUpdateTime);") != std::string::npos);
    CHECK(sBody.find("sAICtrls->Remove(") == std::string::npos);
    CHECK(sBody.find("sAICtrls->Add(") == std::string::npos);
}

TEST_CASE("controller update loop requeues through the ordered list move" * doctest::test_suite("regression"))
{
    std::string sManager;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\ControllerManager.cpp", sManager));

    std::string::size_type uiBegin = sManager.find("void ControllerManager::Update(float fDeltaTime)");
    REQUIRE(uiBegin != std::string::npos);
    std::string::size_type uiEnd = sManager.find("\n}", uiBegin);
    REQUIRE(uiEnd != std::string::npos);
    const std::string sBody = sManager.substr(uiBegin, uiEnd - uiBegin);
    CHECK(CountOf(sBody, "sAICtrls->Move(pCtrl->GetManagerNode(), pCtrl, pCtrl->mNextUpdateTime);") == 1);
    CHECK(sBody.find("sAICtrls->Remove(") == std::string::npos);
    CHECK(sBody.find("sAICtrls->Add(") == std::string::npos);
}

TEST_CASE("ordered list move ignores a node that is not in the list" * doctest::test_suite("regression"))
{
    PblOrderedList<Thread, float> list;
    PblOrderedList<Thread, float>::Node aNodes[3];
    Thread* const pObject = reinterpret_cast<Thread*>(&aNodes[2]);
    list.Add(&aNodes[0], pObject, 1.0f);
    list.Add(&aNodes[1], pObject, 2.0f);
    list.Remove(&aNodes[1]);

    list.Move(&aNodes[1], pObject, 0.5f);
    list.Move(&aNodes[2], pObject, 0.5f);
    CHECK(list.GetCount() == 1);
    CHECK(aNodes[1]._pList == (PblOrderedList<Thread, float>*)NULL);
    CHECK(aNodes[1]._pObject == (Thread*)NULL);
    CHECK(aNodes[2]._pList == (PblOrderedList<Thread, float>*)NULL);
    PblOrderedList<Thread, float>::Node* apGot[4];
    REQUIRE(Walk(list, apGot, 4) == 1);
    CHECK(apGot[0] == &aNodes[0]);

    list.Move(&aNodes[0], pObject, 3.0f);
    CHECK(aNodes[0]._pObject == pObject);
    CHECK(aNodes[0]._iOrder == 3.0f);
    CHECK(list.GetCount() == 1);
}

TEST_CASE("soldier water splash and wake timers reset with the other state counters" * doctest::test_suite("regression"))
{
    std::string sSoldier;
    std::string sState;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\EntitySoldier.cpp", sSoldier));
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\GameState.cpp", sState));

    std::string::size_type uiUpdate = sSoldier.find("bool EntitySoldier::Update(float fDeltaTime)");
    REQUIRE(uiUpdate != std::string::npos);
    std::string::size_type uiUpdateEnd = sSoldier.find("\n}", uiUpdate);
    REQUIRE(uiUpdateEnd != std::string::npos);
    const std::string sUpdate = sSoldier.substr(uiUpdate, uiUpdateEnd - uiUpdate);
    CHECK(sUpdate.find("static float s_fSplashTimeout") == std::string::npos);
    CHECK(sUpdate.find("static float s_fWakeTimer") == std::string::npos);
    CHECK(CountOf(sSoldier, "static float s_fSplashTimeout;") == 1);
    CHECK(CountOf(sSoldier, "static float s_fWakeTimer;") == 1);

    std::string::size_type uiReset = sSoldier.find("void EntitySoldier::ResetWaterTimers()");
    REQUIRE(uiReset != std::string::npos);
    CHECK(uiReset < uiUpdate);
    std::string::size_type uiResetEnd = sSoldier.find("\n}", uiReset);
    REQUIRE(uiResetEnd != std::string::npos);
    const std::string sReset = sSoldier.substr(uiReset, uiResetEnd - uiReset);
    CHECK(sReset.find("s_fSplashTimeout = 0.0f;") != std::string::npos);
    CHECK(sReset.find("s_fWakeTimer = 0.0f;") != std::string::npos);

    std::string::size_type uiCleanup = sState.find("void GameState::PostStateCleanup(bool bMission)");
    REQUIRE(uiCleanup != std::string::npos);
    std::string::size_type uiCleanupEnd = sState.find("\n}", uiCleanup);
    REQUIRE(uiCleanupEnd != std::string::npos);
    const std::string sCleanup = sState.substr(uiCleanup, uiCleanupEnd - uiCleanup);
    CHECK(sCleanup.find("CollisionObject::ResetCollisionSerials();") != std::string::npos);
    CHECK(sCleanup.find("EntitySoldier::ResetWaterTimers();") != std::string::npos);
}
