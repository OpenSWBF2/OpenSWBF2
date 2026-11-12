#include "third_party/doctest/doctest.h"

#include <algorithm>
#include <string.h>
#include <string>
#include <new>
#include <vector>

#include "Battlefront2/Source/Character.h"
#include "Battlefront2/Source/Cloth.h"
#include "Battlefront2/Source/CollisionModel.h"
#include "Battlefront2/Source/ControllerManager.h"
#include "Battlefront2/Source/EventManager_Object.h"
#include "Battlefront2/Source/EntityCarrier.h"
#include "Battlefront2/Source/EntityMine.h"
#include "Battlefront2/Source/Navigator.h"
#include "Battlefront2/Source/NavigatorGraphFollower.h"
#include "Battlefront2/Source/NavigatorPathFollower.h"
#include "Battlefront2/Source/NavigatorSlide.h"
#include "Battlefront2/Source/NetWire.h"
#include "Battlefront2/Source/PathFinder.h"
#include "Battlefront2/Source/PathFollower.h"
#include "Battlefront2/Source/PathManager.h"
#include "Battlefront2/Source/StickInfo.h"
#include "Battlefront2/Source/UnitController.h"
#include "Battlefront2/Source/WeaponMelee.h"
#include "RedEngineFL/Memory/RedMemory.h"

#include "testutil.h"

extern const Obstacle* sIgnore[2];
extern int sIgnoreCnt;
extern std::vector<const TreeGridObject*> gTreeGridStubMembers;
extern int gPrepareCollisionCalls;

namespace PathManager
{
extern PblList<PathRequest>* sRequests;
extern PathRequest* sCurRequest;
}

namespace ControllerManager
{
extern PblOrderedList<UnitController, float>* sAICtrls;
}

using PathManager::PathRequest;

namespace
{

PblMatrix IdentityAt(float fX, float fZ)
{
    PblMatrix kMat;
    memset(&kMat, 0, sizeof(kMat));
    kMat.m[0][0] = 1.0f;
    kMat.m[1][1] = 1.0f;
    kMat.m[2][2] = 1.0f;
    kMat.m[3][3] = 1.0f;
    kMat.m[3][0] = fX;
    kMat.m[3][2] = fZ;
    return kMat;
}

PblVector4 BarrierSize(float fX, float fZ)
{
    PblVector4 kSize;
    kSize.x = fX;
    kSize.y = 0.0f;
    kSize.z = fZ;
    kSize.w = 0.0f;
    return kSize;
}

int GridCount(const Obstacle* pBarrier)
{
    return (int)std::count(gTreeGridStubMembers.begin(), gTreeGridStubMembers.end(),
                           static_cast<const TreeGridObject*>(pBarrier));
}

struct PoolPerturbGuard
{
    PoolPerturbGuard()
    {
#ifdef OPENSWBF2_TEST
        for (unsigned int uSeed = 1; uSeed < 64; ++uSeed)
        {
            unsigned int uDraw = 0;
            RedPerturbSet(uSeed, kRedPerturbPool);
            if (RedPerturbDraw(kRedPerturbPool, &uDraw) && uDraw % 4 != 0)
            {
                RedPerturbSet(uSeed, kRedPerturbPool);
                return;
            }
        }
        RedPerturbSet(0, 0);
#endif
    }

    ~PoolPerturbGuard()
    {
#ifdef OPENSWBF2_TEST
        RedPerturbSet(0, 0);
#endif
    }
};

class FakeNavigator : public Navigator
{
public:
    explicit FakeNavigator(unsigned int uRtti) : Navigator(0), mRtti(uRtti)
    {
        memset(mBody, 0, sizeof(mBody));
        memset(mFollower, 0, sizeof(mFollower));
    }

    ~FakeNavigator() { mPathFollower = 0; }

    virtual bool IsRtti(unsigned int uRtti) const { return uRtti == mRtti; }

    const Obstacle** Ignore(int iCnt, const Obstacle* p0, const Obstacle* p1)
    {
        Navigator* pNav = this;
        const Obstacle** apIgnore = 0;
        if (mRtti == NavigatorGraphFollower::GetRtti())
        {
            static_cast<NavigatorGraphFollower*>(pNav)->mIgnoreCnt = iCnt;
            apIgnore = static_cast<NavigatorGraphFollower*>(pNav)->mIgnore;
        }
        else if (mRtti == NavigatorPathFollower::GetRtti())
        {
            static_cast<NavigatorPathFollower*>(pNav)->mIgnoreCnt = iCnt;
            apIgnore = static_cast<NavigatorPathFollower*>(pNav)->mIgnore;
        }
        else if (mRtti == NavigatorSlide::GetRtti())
        {
            static_cast<NavigatorSlide*>(pNav)->mIgnoreCnt = iCnt;
            apIgnore = static_cast<NavigatorSlide*>(pNav)->mIgnore;
        }
        REQUIRE(apIgnore);
        apIgnore[0] = p0;
        apIgnore[1] = p1;
        return apIgnore;
    }

    const Obstacle** FollowerIgnore(int iCnt, const Obstacle* p0, const Obstacle* p1)
    {
        PathFollower* pFollower = reinterpret_cast<PathFollower*>(mFollower);
        mPathFollower = pFollower;
        pFollower->mIgnoreCnt = iCnt;
        pFollower->mIgnore[0] = p0;
        pFollower->mIgnore[1] = p1;
        return pFollower->mIgnore;
    }

protected:
    virtual void EnterState() {}
    virtual void ExitState() {}
    virtual void UpdateState(float) {}

public:
    alignas(16) char mBody[sizeof(NavigatorSlide) + sizeof(NavigatorGraphFollower) + sizeof(NavigatorPathFollower)];
    alignas(16) char mFollower[sizeof(PathFollower)];
    unsigned int mRtti;
};

struct FakeAiController
{
    explicit FakeAiController(unsigned int uRtti) : mNav(uRtti) { memset(mBytes, 0, sizeof(mBytes)); }

    UnitController* Ctrl() { return reinterpret_cast<UnitController*>(mBytes); }

    alignas(16) char mBytes[sizeof(UnitController)];
    FakeNavigator mNav;
};

struct BarrierWorld
{
    BarrierWorld()
    {
        Obstacle::sMemoryPool.Destroy();
        PathFinder::Initialize();
        Obstacle::sMemoryPool.Create(Obstacle::sMemoryPool.GetCount(), Obstacle::sMemoryPool.GetSize(),
                                     Obstacle::sMemoryPool.GetGrow());
        REQUIRE(Obstacle::sMemoryPool.GetCount() == 512);
        PathManager::Initialize();
        ControllerManager::Init();
        mHost = ControllerManager::sAICtrls;
        ControllerManager::EnterClient();
        mClient = ControllerManager::sAICtrls;
        ControllerManager::LeaveClient();
        Clear();
    }

    ~BarrierWorld()
    {
        PathManager::sCurRequest = 0;
        ControllerManager::Reset();
        PathManager::Reset();
        PathFinder::Reset();
        Obstacle::sMemoryPool.Destroy();
        PathRequest::sMemoryPool.Destroy();
        AIPath::sMemoryPool.Destroy();
        AIPathNode::sMemoryPool.Destroy();
        Clear();
    }

    static void Clear()
    {
        gTreeGridStubMembers.clear();
        sIgnore[0] = 0;
        sIgnore[1] = 0;
        sIgnoreCnt = 0;
    }

    static PathRequest* NewRequest(const Obstacle* pBarrier)
    {
        const Obstacle* apIgnore[1] = { pBarrier };
        const PblVector3 vAt(0.0f, 0.0f, 0.0f);
        void* pBlock = PathRequest::sMemoryPool.Allocate(sizeof(PathRequest));
        REQUIRE(pBlock);
        return ::new (pBlock) PathRequest(0, vAt, vAt, 1.0f, (ConnectivityGraph::ProhibitFlag)0, 0, 0, apIgnore, 1);
    }

    PathRequest* AddRequest(const Obstacle* pBarrier)
    {
        PathRequest* pRequest = NewRequest(pBarrier);
        PathManager::sRequests->Add(&pRequest->mNode, pRequest);
        return pRequest;
    }

    PathRequest* SetCurrentRequest(const Obstacle* pBarrier)
    {
        PathManager::sCurRequest = NewRequest(pBarrier);
        return PathManager::sCurRequest;
    }

    void AddController(bool bHost, FakeAiController& rCtrl, bool bNavigator)
    {
        if (bNavigator)
        {
            rCtrl.Ctrl()->GetLowLevel().mNavigator = &rCtrl.mNav;
        }
        (bHost ? mHost : mClient)->Add(rCtrl.Ctrl()->GetManagerNode(), rCtrl.Ctrl(), 0.0f);
    }

    PblOrderedList<UnitController, float>* mHost;
    PblOrderedList<UnitController, float>* mClient;
};

class StickTestObject : public CollisionObject
{
public:
    StickTestObject() : CollisionObject(COLL_SOFT, 0, 0, 0), mGameObject(0) { memset(&mMatrix, 0, sizeof(mMatrix)); }

    const PblVector3& GetPosition() const { return mLastPosition; }
    const PblMatrix& GetMatrix() const { return mMatrix; }
    GameObject* GetGameObject() { return mGameObject; }
    const GameObject* GetGameObject() const { return mGameObject; }

    PblMatrix mMatrix;
    GameObject* mGameObject;
};

struct alignas(StickTestObject) StickTestBuffer
{
    unsigned char acBytes[sizeof(StickTestObject)];
};

struct CollisionModelBlock
{
    CollisionModelBlock() { memset(mBytes, 0, sizeof(mBytes)); }

    CollisionModel* Model() { return reinterpret_cast<CollisionModel*>(mBytes); }

    alignas(16) char mBytes[sizeof(CollisionModel)];
};

struct FakeMineEntity : Entity
{
    explicit FakeMineEntity(bool bMine) : mIsMine(bMine) {}

    virtual bool IsRtti(unsigned int uRtti) const { return mIsMine && uRtti == EntityMine::rttiHashEntityMine; }

    bool mIsMine;
};

struct FakeMineSlot
{
    FakeMineSlot() { memset(mBytes, 0, sizeof(mBytes)); }

    GameObject* Build(bool bMine)
    {
        new (mBytes) FakeMineEntity(bMine);
        return reinterpret_cast<GameObject*>(mBytes);
    }

    void SetStick(StickInfo* pStick) { static_cast<EntityMine*>(reinterpret_cast<GameObject*>(mBytes))->mStickInfo = pStick; }

    alignas(16) char mBytes[sizeof(EntityMine)];
};

int PrepareCallsFor(CollisionObject& rThis, CollisionObject& rOther)
{
    const int iBefore = gPrepareCollisionCalls;
    rThis.CheckForCollision(&rOther, 0, 0);
    return gPrepareCollisionCalls - iBefore;
}

struct ClothEnableGuard
{
    ClothEnableGuard() : mEnabled(EntityCloth::IsEnabled()) {}

    ~ClothEnableGuard() { EntityCloth::SetEnable(mEnabled); }

    bool mEnabled;
};

struct FakeEntity : Entity
{
    explicit FakeEntity(bool bGameObject) : mIsGameObject(bGameObject) {}

    virtual GameObject* GetGameObject() { return mIsGameObject ? reinterpret_cast<GameObject*>(this) : 0; }

    virtual const GameObject* GetGameObject() const
    {
        return mIsGameObject ? reinterpret_cast<const GameObject*>(this) : 0;
    }

    bool mIsGameObject;
};

struct FakeObjectSlot
{
    FakeObjectSlot() : mLive(false), mHandled(false) { memset(mBytes, 0, sizeof(mBytes)); }

    ~FakeObjectSlot() { Kill(); }

    GameObject* Object() { return reinterpret_cast<GameObject*>(mBytes); }

    PblHandled* Handled() { return static_cast<PblHandled*>(Object()); }

    GameObject* Build(bool bGameObject, bool bInLiveSet)
    {
        new (mBytes) FakeEntity(bGameObject);
        new (Handled()) PblHandled;
        mHandled = true;
        if (bInLiveSet)
        {
            EntityEx::sLive.Insert(mBytes);
            mLive = true;
        }
        return Object();
    }

    void Kill()
    {
        if (mLive)
        {
            EntityEx::sLive.Remove(mBytes);
            mLive = false;
        }
        if (mHandled)
        {
            Handled()->~PblHandled();
            mHandled = false;
        }
    }

    alignas(16) char mBytes[sizeof(GameObject)];
    bool mLive;
    bool mHandled;
};

typedef EventManager::ObjectMatches<char> CharObjectMatches;

struct CollisionObjectBlock
{
    CollisionObjectBlock() { memset(mBytes, 0, sizeof(mBytes)); }

    CollisionObject* Build(unsigned int uSerial)
    {
        memset(mBytes, 0, sizeof(mBytes));
        CollisionObject* pObject = reinterpret_cast<CollisionObject*>(mBytes);
        pObject->mCollisionSerial = uSerial;
        return pObject;
    }

    alignas(16) char mBytes[sizeof(CollisionObject)];
};

bool HasText(const std::string& sText, const char* pcNeedle)
{
    return sText.find(pcNeedle) != std::string::npos;
}

int CountOf(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    for (std::string::size_type uiAt = sText.find(pcNeedle); uiAt != std::string::npos; uiAt = sText.find(pcNeedle, uiAt + 1))
    {
        ++iCount;
    }
    return iCount;
}

}

TEST_CASE("object event filter does not match a new object at a dead object's address" * doctest::test_suite("regression"))
{
    FakeObjectSlot kSlot;
    char cParam = 0;
    GameObject* pA = kSlot.Build(true, true);
    CharObjectMatches kFilterA(pA);
    CHECK(kFilterA(*pA, cParam));

    kSlot.Kill();
    GameObject* pB = kSlot.Build(true, true);
    CHECK(pB == pA);
    CHECK(!kFilterA(*pB, cParam));

    CharObjectMatches kFilterB(pB);
    CHECK(kFilterB(*pB, cParam));
}

TEST_CASE("object event filter dedupe compares identity not address" * doctest::test_suite("regression"))
{
    FakeObjectSlot kSlot;
    FakeObjectSlot kOther;
    GameObject* pA = kSlot.Build(true, true);
    CharObjectMatches kFilterA1(pA);
    CharObjectMatches kFilterA2(pA);
    CHECK(kFilterA1 == kFilterA2);

    GameObject* pC = kOther.Build(true, true);
    CharObjectMatches kFilterC(pC);
    CHECK(!(kFilterA1 == kFilterC));

    kSlot.Kill();
    kOther.Kill();
    CHECK(!(kFilterA1 == kFilterC));

    GameObject* pB = kSlot.Build(true, true);
    CharObjectMatches kFilterB(pB);
    CHECK(!(kFilterA2 == kFilterB));
    CHECK(kFilterB == CharObjectMatches(pB));
}

TEST_CASE("object event filter from an unknown pointer matches nothing" * doctest::test_suite("regression"))
{
    FakeObjectSlot kUnlisted;
    FakeObjectSlot kNotObject;
    char cParam = 0;
    GameObject* pUnlisted = kUnlisted.Build(true, false);
    GameObject* pNotObject = kNotObject.Build(false, true);

    CharObjectMatches kNull((GameObject*)0);
    CharObjectMatches kFilterUnlisted(pUnlisted);
    CharObjectMatches kFilterNotObject(pNotObject);
    CHECK(!kFilterUnlisted(*pUnlisted, cParam));
    CHECK(!kFilterNotObject(*pNotObject, cParam));
    CHECK(kFilterUnlisted == kNull);
    CHECK(kFilterNotObject == kNull);
}

TEST_CASE("melee hit dedupe does not follow a dead victim to a new object at its address" * doctest::test_suite("regression"))
{
    CollisionObjectBlock kVictim;
    CollisionObjectBlock kOther;
    WeaponMelee::DamageData kData;
    memset(&kData, 0, sizeof(kData));

    CollisionObject* pA = kVictim.Build(11);
    CollisionObject* pOther = kOther.Build(12);
    CHECK(!kData.HasHit(pA));
    kData.NoteHit(pA);
    CHECK(kData.iNumObjects == 1);
    CHECK(kData.HasHit(pA));
    CHECK(!kData.HasHit(pOther));

    CollisionObject* pB = kVictim.Build(13);
    CHECK(pB == pA);
    CHECK(!kData.HasHit(pB));
}

TEST_CASE("melee hit sites go through the damage data helpers (source pin)" * doctest::test_suite("regression"))
{
    std::string sMelee;
    std::string sThrow;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\WeaponMelee.cpp", sMelee));
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\OrdnanceMeleeThrow.cpp", sThrow));
    CHECK(HasText(sMelee, "if (pData->HasHit(pObject))"));
    CHECK(HasText(sMelee, "pData->NoteHit(pObject);"));
    CHECK(HasText(sThrow, "if (pData->HasHit(pObject))"));
    CHECK(HasText(sThrow, "pData->NoteHit(pObject);"));
    CHECK(!HasText(sMelee, "apObject["));
    CHECK(!HasText(sThrow, "apObject["));
}

TEST_CASE("stuck mine does not ignore a new object at its dead target's address" * doctest::test_suite("regression"))
{
    FakeObjectSlot kHost;
    CollisionObjectBlock kLevel;
    CollisionObjectBlock kElse;
    GameObject* pA = kHost.Build(true, true);
    CollisionObject* pHostCollision = pA;

    StickInfo kStick;
    kStick.mCollHandle = pA;
    kStick.mCollObject = pHostCollision;
    kStick.mCollKey = CollisionObject::LastCollidedKey(pHostCollision);
    CHECK(kStick.IsStuckTo(pHostCollision));
    CHECK(!kStick.IsStuckTo(kElse.Build(1)));

    kHost.Kill();
    GameObject* pB = kHost.Build(true, true);
    CHECK(static_cast<CollisionObject*>(pB) == pHostCollision);
    CHECK(!kStick.IsStuckTo(pHostCollision));

    StickInfo kLevelStick;
    CollisionObject* pLevel = kLevel.Build(2);
    kLevelStick.mCollObject = pLevel;
    kLevelStick.mCollKey = CollisionObject::LastCollidedKey(pLevel);
    CHECK(kLevelStick.IsStuckTo(pLevel));
    CHECK(!kLevelStick.IsStuckTo(pHostCollision));
    CHECK(!kLevelStick.IsStuckTo(0));

    CollisionObject* pReused = kLevel.Build(3);
    CHECK(pReused == pLevel);
    CHECK(!kLevelStick.IsStuckTo(pReused));
}

TEST_CASE("mine stick sites go through the stick helper and record the stick key (source pin)" * doctest::test_suite("regression"))
{
    std::string sMine;
    std::string sNetState;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\EntityMine.cpp", sMine));
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\NetState.cpp", sNetState));
    CHECK(HasText(sMine, "if (mStickInfo && mStickInfo->IsStuckTo(pOther))"));
    CHECK(!HasText(sMine, "mCollObject == pOther"));
    CHECK(HasText(sMine, "mStickInfo->mCollKey = CollisionObject::LastCollidedKey(pOther);"));
    CHECK(HasText(sNetState, "mStickInfo->mCollKey = CollisionObject::LastCollidedKey(mStickInfo->mCollObject);"));
}

TEST_CASE("barrier ignore helper clears only the slots that name the barrier" * doctest::test_suite("regression"))
{
    alignas(16) char acUnit[64];
    alignas(16) char acTarget[64];
    const Obstacle* pUnitBarrier = reinterpret_cast<const Obstacle*>(acUnit);
    const Obstacle* pTargetBarrier = reinterpret_cast<const Obstacle*>(acTarget);

    const Obstacle* apIgnore[2] = { pUnitBarrier, pTargetBarrier };
    Navigator::ForgetIgnored(apIgnore, 2, 0);
    CHECK(apIgnore[0] == pUnitBarrier);
    CHECK(apIgnore[1] == pTargetBarrier);

    Navigator::ForgetIgnored(apIgnore, 2, pTargetBarrier);
    CHECK(apIgnore[0] == pUnitBarrier);
    CHECK(apIgnore[1] == static_cast<const Obstacle*>(0));

    const Obstacle* apOne[2] = { pTargetBarrier, pTargetBarrier };
    Navigator::ForgetIgnored(apOne, 1, pTargetBarrier);
    CHECK(apOne[0] == static_cast<const Obstacle*>(0));
    CHECK(apOne[1] == pTargetBarrier);
}

TEST_CASE("replacing a barrier keeps the owner's barrier in every ignore list and in the grid once" * doctest::test_suite("regression"))
{
    BarrierWorld kWorld;
    const PblVector4 kSize = BarrierSize(2.0f, 3.0f);

    Obstacle* pBarrier = 0;
    PathFinder::PlaceBarrier(pBarrier, IdentityAt(10.0f, 20.0f), kSize, 7);
    REQUIRE(pBarrier != static_cast<Obstacle*>(0));
    CHECK(GridCount(pBarrier) == 1);

    sIgnore[0] = pBarrier;
    sIgnoreCnt = 1;
    PathRequest* pRequest = kWorld.AddRequest(pBarrier);
    FakeAiController kCtrl(NavigatorGraphFollower::GetRtti());
    kWorld.AddController(true, kCtrl, true);
    const Obstacle** apNav = kCtrl.mNav.Ignore(1, pBarrier, 0);
    const Obstacle** apFollow = kCtrl.mNav.FollowerIgnore(1, pBarrier, 0);

    Obstacle* pBefore = pBarrier;
    {
        PoolPerturbGuard kPerturb;
        PathFinder::PlaceBarrier(pBarrier, IdentityAt(40.0f, 50.0f), kSize, 7);
    }
    CHECK(pBarrier == pBefore);
    CHECK(pBarrier->mPosX == 40.0f);
    CHECK(pBarrier->mPosZ == 50.0f);
    CHECK(GridCount(pBarrier) == 1);
    CHECK(gTreeGridStubMembers.size() == 1);
    CHECK(sIgnore[0] == pBarrier);
    CHECK(pRequest->mObstacles[0] == pBarrier);
    CHECK(apNav[0] == pBarrier);
    CHECK(apFollow[0] == pBarrier);

    PathFinder::DeleteGameObjectBarrier(pBarrier);
    CHECK(apNav[0] == static_cast<const Obstacle*>(0));
    CHECK(apFollow[0] == static_cast<const Obstacle*>(0));
}

TEST_CASE("deleting a barrier clears the search and every path request and AI navigator" * doctest::test_suite("regression"))
{
    BarrierWorld kWorld;
    const PblVector4 kSize = BarrierSize(1.0f, 1.0f);

    Obstacle* pBarrier = 0;
    Obstacle* pOther = 0;
    PathFinder::PlaceBarrier(pBarrier, IdentityAt(1.0f, 1.0f), kSize, 1);
    PathFinder::PlaceBarrier(pOther, IdentityAt(5.0f, 5.0f), kSize, 2);
    const Obstacle* pDead = pBarrier;

    sIgnore[0] = pOther;
    sIgnore[1] = pBarrier;
    sIgnoreCnt = 2;
    PathRequest* pQueued = kWorld.AddRequest(pBarrier);
    PathRequest* pQueuedOther = kWorld.AddRequest(pOther);
    PathRequest* pCurrent = kWorld.SetCurrentRequest(pBarrier);

    FakeAiController kGraph(NavigatorGraphFollower::GetRtti());
    FakeAiController kPath(NavigatorPathFollower::GetRtti());
    FakeAiController kSlide(NavigatorSlide::GetRtti());
    FakeAiController kPlain(Navigator::GetRtti());
    FakeAiController kNoNav(Navigator::GetRtti());
    kWorld.AddController(true, kGraph, true);
    kWorld.AddController(false, kPath, true);
    kWorld.AddController(true, kSlide, true);
    kWorld.AddController(false, kPlain, true);
    kWorld.AddController(true, kNoNav, false);
    const Obstacle** apGraph = kGraph.mNav.Ignore(2, pOther, pBarrier);
    const Obstacle** apPath = kPath.mNav.Ignore(1, pBarrier, 0);
    const Obstacle** apSlide = kSlide.mNav.Ignore(2, pBarrier, pBarrier);
    const Obstacle** apSlideFollow = kSlide.mNav.FollowerIgnore(2, pBarrier, pOther);
    const Obstacle** apPlainFollow = kPlain.mNav.FollowerIgnore(1, pBarrier, 0);

    PathFinder::DeleteGameObjectBarrier(pBarrier);
    CHECK(pBarrier == static_cast<Obstacle*>(0));
    CHECK(sIgnore[0] == pOther);
    CHECK(sIgnore[1] == static_cast<const Obstacle*>(0));
    CHECK(pQueued->mObstacles[0] == static_cast<const Obstacle*>(0));
    CHECK(pQueuedOther->mObstacles[0] == pOther);
    CHECK(pCurrent->mObstacles[0] == static_cast<const Obstacle*>(0));
    CHECK(GridCount(pDead) == 0);
    CHECK(GridCount(pOther) == 1);
    CHECK(apGraph[0] == pOther);
    CHECK(apGraph[1] == static_cast<const Obstacle*>(0));
    CHECK(apPath[0] == static_cast<const Obstacle*>(0));
    CHECK(apSlide[0] == static_cast<const Obstacle*>(0));
    CHECK(apSlide[1] == static_cast<const Obstacle*>(0));
    CHECK(apSlideFollow[0] == static_cast<const Obstacle*>(0));
    CHECK(apSlideFollow[1] == pOther);
    CHECK(apPlainFollow[0] == static_cast<const Obstacle*>(0));

    PathFinder::DeleteGameObjectBarrier(pOther);
    CHECK(apGraph[0] == static_cast<const Obstacle*>(0));
    CHECK(apSlideFollow[1] == static_cast<const Obstacle*>(0));
}

TEST_CASE("controller reset drops the host and client lists of the ended mission" * doctest::test_suite("regression"))
{
    BarrierWorld kWorld;
    FakeAiController kHost(NavigatorPathFollower::GetRtti());
    FakeAiController kClient(NavigatorSlide::GetRtti());
    kWorld.AddController(true, kHost, true);
    kWorld.AddController(false, kClient, true);
    alignas(16) char acBarrier[sizeof(Obstacle)];
    const Obstacle* pBarrier = reinterpret_cast<const Obstacle*>(acBarrier);

    const Obstacle** apHost = kHost.mNav.Ignore(1, pBarrier, 0);
    const Obstacle** apClient = kClient.mNav.Ignore(1, pBarrier, 0);
    ControllerManager::ForgetBarrier(pBarrier);
    CHECK(apHost[0] == static_cast<const Obstacle*>(0));
    CHECK(apClient[0] == static_cast<const Obstacle*>(0));

    apHost = kHost.mNav.Ignore(1, pBarrier, 0);
    apClient = kClient.mNav.Ignore(1, pBarrier, 0);
    ControllerManager::Reset();
    ControllerManager::EnterClient();
    CHECK(ControllerManager::sAICtrls == static_cast<PblOrderedList<UnitController, float>*>(0));
    ControllerManager::LeaveClient();
    CHECK(ControllerManager::sAICtrls == static_cast<PblOrderedList<UnitController, float>*>(0));
    ControllerManager::ForgetBarrier(pBarrier);
    CHECK(apHost[0] == pBarrier);
    CHECK(apClient[0] == pBarrier);
}

TEST_CASE("mine keeps its parent link only while stuck to that live parent" * doctest::test_suite("regression"))
{
    CollisionModelBlock kModel;
    StickTestObject kMine;
    StickTestObject kTarget;
    StickTestObject kElse;
    FakeObjectSlot kHost;
    FakeMineSlot kMineObject;
    kMine.mCollisionModel = kModel.Model();
    kTarget.mCollisionModel = kModel.Model();
    kElse.mCollisionModel = kModel.Model();
    kTarget.mGameObject = kHost.Build(true, true);
    kMine.mGameObject = kMineObject.Build(true);
    kMine.mParent = &kTarget;

    StickInfo kStick;
    kStick.mCollHandle = kTarget.mGameObject;
    kStick.mCollObject = &kTarget;
    kStick.mCollKey = CollisionObject::LastCollidedKey(&kTarget);
    kMineObject.SetStick(&kStick);

    CHECK(PrepareCallsFor(kMine, kTarget) == 0);
    CHECK(PrepareCallsFor(kTarget, kMine) == 0);
    CHECK(PrepareCallsFor(kMine, kElse) == 1);

    kMineObject.SetStick(0);
    CHECK(PrepareCallsFor(kMine, kTarget) == 0);
    kMineObject.SetStick(&kStick);

    kHost.Kill();
    CHECK(PrepareCallsFor(kMine, kTarget) == 1);
    CHECK(PrepareCallsFor(kTarget, kMine) == 1);

    FakeMineSlot kNotMine;
    StickTestObject kChild;
    kChild.mCollisionModel = kModel.Model();
    kChild.mGameObject = kNotMine.Build(false);
    kNotMine.SetStick(&kStick);
    kChild.mParent = &kTarget;
    CHECK(PrepareCallsFor(kChild, kTarget) == 0);
}

TEST_CASE("mine stuck to a deleted prop does not keep its link to a new object at the prop's address" * doctest::test_suite("regression"))
{
    CollisionModelBlock kModel;
    StickTestObject kMine;
    FakeMineSlot kMineObject;
    StickTestBuffer kStorage;
    kMine.mCollisionModel = kModel.Model();
    kMine.mGameObject = kMineObject.Build(true);
    StickTestObject* pProp = new (&kStorage) StickTestObject;
    pProp->mCollisionModel = kModel.Model();
    kMine.mParent = pProp;

    StickInfo kStick;
    kStick.mCollObject = pProp;
    kStick.mCollKey = CollisionObject::LastCollidedKey(pProp);
    kMineObject.SetStick(&kStick);
    CHECK(PrepareCallsFor(kMine, *pProp) == 0);

    pProp->~StickTestObject();
    StickTestObject* pReused = new (&kStorage) StickTestObject;
    REQUIRE(static_cast<void*>(pReused) == static_cast<void*>(pProp));
    pReused->mCollisionModel = kModel.Model();
    CHECK(PrepareCallsFor(kMine, *pReused) == 1);
    CHECK(PrepareCallsFor(*pReused, kMine) == 1);
    pReused->~StickTestObject();
}

TEST_CASE("a cloth that dies while cloth is disabled leaves its bank" * doctest::test_suite("regression"))
{
    ClothEnableGuard kGuard;
    ClothBank kBank(0);
    alignas(16) char acCloth[sizeof(EntityCloth)];
    alignas(16) char acOther[sizeof(EntityCloth)];
    EntityDesc kDesc;

    EntityCloth::SetEnable(false);
    EntityCloth* pCloth = ::new (acCloth) EntityCloth(0, kDesc);
    EntityCloth* pOther = ::new (acOther) EntityCloth(0, kDesc);
    pCloth->bank_ = &kBank;
    pOther->bank_ = &kBank;
    kBank.Push(pCloth, 0.0f);
    kBank.Push(pOther, 0.0f);
    REQUIRE(kBank.Find(pCloth));

    pCloth->~EntityCloth();
    CHECK_FALSE(kBank.Find(pCloth));
    CHECK(kBank.Find(pOther));
    CHECK(kBank.cloth_.GetSize() == 1);

    pOther->~EntityCloth();
    CHECK(kBank.cloth_.GetSize() == 0);
}

TEST_CASE("vehicle damage total does not follow a dead vehicle to a new object at its address" * doctest::test_suite("regression"))
{
    alignas(16) char acCharacter[sizeof(Character)];
    memset(acCharacter, 0, sizeof(acCharacter));
    Character* pCharacter = reinterpret_cast<Character*>(acCharacter);
    StickTestBuffer kStorage;
    StickTestObject* pVehicle = new (&kStorage) StickTestObject;
    const unsigned int uKey = CollisionObject::LastCollidedKey(pVehicle);
    REQUIRE(uKey != 0);
    CHECK_FALSE(pCharacter->IsDamagedVehicle(uKey));
    CHECK_FALSE(pCharacter->IsDamagedVehicle(0));

    pCharacter->SetDamagedVehicle(uKey);
    CHECK(pCharacter->IsDamagedVehicle(CollisionObject::LastCollidedKey(pVehicle)));

    pVehicle->~StickTestObject();
    StickTestObject* pReused = new (&kStorage) StickTestObject;
    REQUIRE(static_cast<void*>(pReused) == static_cast<void*>(pVehicle));
    CHECK_FALSE(pCharacter->IsDamagedVehicle(CollisionObject::LastCollidedKey(pReused)));
    pReused->~StickTestObject();
}

TEST_CASE("host world lookup rejects an index past the host world table" * doctest::test_suite("regression"))
{
    FakeObjectSlot aSlot[3];
    GameObject* apTable[5];
    for (int i = 0; i < 3; ++i)
    {
        apTable[i] = aSlot[i].Build(true, false);
    }
    apTable[3] = apTable[2];
    apTable[4] = apTable[1];

    CHECK(NetWire::MatchHostWorldIndex(apTable, 3, 1, apTable[1]) == 1);
    CHECK(NetWire::MatchHostWorldIndex(apTable, 3, 1, apTable[2]) == -1);
    CHECK(NetWire::MatchHostWorldIndex(apTable, 3, -1, apTable[1]) == -1);
    CHECK(NetWire::MatchHostWorldIndex(apTable, 3, 3, apTable[2]) == -1);
    CHECK(NetWire::MatchHostWorldIndex(apTable, 3, 4, apTable[1]) == -1);
    CHECK(NetWire::MatchHostWorldIndex(apTable, 3, 0x40, apTable[1]) == -1);
}

TEST_CASE("deleting a carrier clears its cargo parent link so a new object at the dead carrier's address is not skipped" * doctest::test_suite("regression"))
{
    FakeObjectSlot kCarrier;
    FakeObjectSlot kCargo;
    FakeObjectSlot kOtherCargo;
    FakeObjectSlot kDeadCargo;
    CollisionObjectBlock kElse;
    CollisionObject* pCarrier = kCarrier.Build(true, true);
    CollisionObject* pElse = kElse.Build(1);
    GameObject* pCargo = kCargo.Build(true, true);
    GameObject* pOtherCargo = kOtherCargo.Build(true, true);
    GameObject* pDeadCargo = kDeadCargo.Build(true, true);
    pCargo->mParent = pCarrier;
    pOtherCargo->mParent = pElse;
    pDeadCargo->mParent = pCarrier;

    EntityCarrier::CargoSlot aSlots[4];
    aSlots[0].mObject = pCargo;
    aSlots[1].mObject = pOtherCargo;
    aSlots[2].mObject = pDeadCargo;
    kDeadCargo.Kill();

    EntityCarrier::ClearCargoParents(aSlots, 4, pCarrier);
    CHECK(pCargo->mParent == static_cast<CollisionObject*>(0));
    CHECK(pOtherCargo->mParent == pElse);
    CHECK(pDeadCargo->mParent == pCarrier);

    kCarrier.Kill();
    CollisionObject* pNew = kCarrier.Build(true, true);
    CHECK(pNew == pCarrier);
    CHECK(pCargo->mParent != pNew);
}

TEST_CASE("carrier destructor clears its cargo parents (source pin)" * doctest::test_suite("regression"))
{
    std::string sCarrier;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\EntityCarrier.cpp", sCarrier));
    const std::string::size_type uiDtor = sCarrier.find("EntityCarrier::~EntityCarrier()");
    const std::string::size_type uiNext = sCarrier.find("// FUNCTION:", uiDtor);
    REQUIRE(uiDtor != std::string::npos);
    const std::string::size_type uiCall = sCarrier.find("ClearCargoParents(mCargoArray, sizeof(mCargoArray) / sizeof(mCargoArray[0]), this);", uiDtor);
    CHECK(uiCall != std::string::npos);
    CHECK(uiCall < uiNext);
}
