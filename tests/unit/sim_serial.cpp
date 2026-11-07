#include "third_party/doctest/doctest.h"

#include "Battlefront2/Source/Thread.h"
#include "Battlefront2/Source/CollisionObject.h"
#include "Battlefront2/Source/EntityCarrier.h"
#include "Battlefront2/Source/WobblePhase.h"
#include "PebbleFL/Common/PblPtrOrder.h"
#include "PebbleFL/Common/PblSerial.h"
#include "PebbleFL/Common/PblSimOrder.h"
#include "RedEngineFL/Util/RedLog.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "testutil.h"

namespace
{

class SerialOrderScope
{
public:
    SerialOrderScope() { PblSetSimOrder(PBL_SIMORDER_SERIAL); }
    ~SerialOrderScope() { PblSetSimOrder(PBL_SIMORDER_ADDRESS); }
};

template <class KEY>
struct SentinelFixture
{
    typename PblOrderedList<Thread, KEY>::Node aNodes[4];
    PblOrderedList<Thread, KEY> list;
};

bool ReadSource(const char* pcPath, std::string& sOut)
{
    FILE* pFile = fopen(pcPath, "rb");
    if (!pFile)
    {
        return false;
    }
    char acBuf[4096];
    size_t uiRead;
    while ((uiRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sOut.append(acBuf, uiRead);
    }
    fclose(pFile);
    return true;
}

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

std::string StripCr(const std::string& sText)
{
    std::string sOut;
    sOut.reserve(sText.size());
    for (size_t i = 0; i < sText.size(); ++i)
    {
        if (sText[i] != '\r')
        {
            sOut += sText[i];
        }
    }
    return sOut;
}

bool ReadSourceLf(const char* pcPath, std::string& sOut)
{
    std::string sRaw;
    if (!ReadSource(pcPath, sRaw))
    {
        return false;
    }
    sOut = StripCr(sRaw);
    return true;
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
    CHECK(WobblePhase(static_cast<const GameObject*>(static_cast<EntityFlyer*>(pFollower)), pFollower) ==
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
    CHECK_FALSE(PblPtrAfter(pLow, pHigh));
    CHECK(PblPtrAfter(pHigh, pLow));
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

TEST_CASE("collision serial sites keep the wire bool writes and the address arm" * doctest::test_suite("regression"))
{
    std::string sNetState;
    std::string sManager;
    std::string sObject;
    std::string sState;
    if (!ReadSourceLf("../src/Battlefront2/Source/NetState.cpp", sNetState) ||
        !ReadSourceLf("../src/Battlefront2/Source/CollisionManager.cpp", sManager) ||
        !ReadSourceLf("../src/Battlefront2/Source/CollisionObject.cpp", sObject) ||
        !ReadSourceLf("../src/Battlefront2/Source/GameState.cpp", sState))
    {
        SkipCase("source files not found");
    }

    CHECK(CountOf(sNetState, "group.Write((bool)mLastCollidedWithNonTerrain);") == 4);
    CHECK(CountOf(sNetState, "group.Write(mLastCollidedWithNonTerrain)") == 0);
    CHECK(CountOf(sManager, "PblPtrAfter(pOther, pObject)") == 1);
    CHECK(CountOf(sManager, "CollisionSerialAfter(pOther, pObject)") == 1);
    CHECK(CountOf(sObject, "AssignCollisionSerial();") == 1);
    CHECK(sObject.find("    mProcedurallyAnimated = false;\n"
                       "    if (PblGetSimOrder() == PBL_SIMORDER_SERIAL)\n"
                       "    {\n"
                       "        AssignCollisionSerial();\n"
                       "    }\n")
          != std::string::npos);
    CHECK(sState.find("    Thread::Reset();\n    CollisionObject::ResetCollisionSerials();\n") != std::string::npos);
}

TEST_CASE("ordered list breaks equal keys fifo under serial order" * doctest::test_suite("regression"))
{
    SerialOrderScope scope;
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
    SerialOrderScope scope;
    CheckSentinelStopsAtHead<int>(0x7FFFFFFF);
    CheckSentinelStopsAtHead<float>(2147483648.0f);
}

TEST_CASE("ordered list keeps address tie order under address order" * doctest::test_suite("regression"))
{
    REQUIRE(PblGetSimOrder() == PBL_SIMORDER_ADDRESS);
    PblOrderedList<Thread, int> list;
    PblOrderedList<Thread, int>::Node aNodes[4];
    for (int i = 3; i >= 0; --i)
    {
        list.Add(&aNodes[i], NULL, 3);
    }

    PblOrderedList<Thread, int>::Node* apGot[8];
    REQUIRE(Walk(list, apGot, 8) == 4);
    for (int i = 0; i < 4; ++i)
    {
        CHECK(apGot[i] == &aNodes[i]);
    }
}

TEST_CASE("thread activation order ignores node addresses under serial order" * doctest::test_suite("regression"))
{
    SerialOrderScope scope;
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
    if (!ReadSource("../src/Battlefront2/Source/Thread.cpp", sThread))
    {
        SkipCase("source files not found");
    }

    CHECK(CountOf(sThread, "NextThreadActivationKey()") == 2);
    CHECK(CountOf(sThread, "PblPtrOrderKey(this)") == 1);
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
    REQUIRE(PblGetSimOrder() == PBL_SIMORDER_ADDRESS);
    const unsigned int auiThis[] = { 0x1000u, 0x1004u, 0x103Cu, 0x7FFFFFFCu, 0x80000010u };
    const unsigned int auiId[] = { 0x13u, 0x20u, 0x7FFFu };
    for (size_t i = 0; i < sizeof(auiThis) / sizeof(auiThis[0]); ++i)
    {
        const void* pThis = (const void*)(uintptr_t)auiThis[i];
        for (size_t j = 0; j < sizeof(auiId) / sizeof(auiId[0]); ++j)
        {
            const GameObject* pOwner = RawOwner(&sRawGameObject, auiId[j]);
            CHECK(WobblePhase(pOwner, pThis) == (float)((auiThis[i] >> 2) & 0xf));
        }
    }

    SerialOrderScope scope;
    for (size_t j = 0; j < sizeof(auiId) / sizeof(auiId[0]); ++j)
    {
        const GameObject* pOwner = RawOwner(&sRawGameObject, auiId[j]);
        for (size_t i = 0; i < sizeof(auiThis) / sizeof(auiThis[0]); ++i)
        {
            const void* pThis = (const void*)(uintptr_t)auiThis[i];
            CHECK(WobblePhase(pOwner, pThis) == (float)(auiId[j] & 0xf));
        }
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
    if (!ReadSourceLf("../src/Battlefront2/Source/EntityFlyer.cpp", sFlyer) ||
        !ReadSourceLf("../src/Battlefront2/Source/EntityPathFollower.cpp", sFollower))
    {
        SkipCase("source files not found");
    }

    const char* pcFlyerCall = "    float fTime = GameLoop::GetMissionTime() + "
                              "WobblePhase(static_cast<const GameObject*>(this), this);";
    const char* pcFollowerCall = "    float fTime = GameLoop::GetMissionTime() +\n"
                                 "                  "
                                 "WobblePhase(static_cast<const GameObject*>(static_cast<EntityFlyer*>(this)), this);";
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
    SerialOrderScope scope;
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
    if (!ReadSourceLf("../src/Battlefront2/Source/ControllerManager.cpp", sManager))
    {
        SkipCase("source files not found");
    }

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
    if (!ReadSourceLf("../src/Battlefront2/Source/ControllerManager.cpp", sManager))
    {
        SkipCase("source files not found");
    }

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
    SerialOrderScope scope;
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
