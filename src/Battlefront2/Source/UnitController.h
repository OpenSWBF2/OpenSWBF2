#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Controller.h"
#include "CommandFsm.h"
#include "UnitCmdInfo.h"
#include "AILowLevel.h"
#include "UnitThreatManager.h"
#include "SquadCommands.h"
#include "Thread.h"
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class UnitAgent;
class AILowLevel;
class Character;
class GameObject;
namespace AI
{
class EventInfo;
}
namespace SquadCommands
{
class SquadCommandOrder;
}

enum AILodState
{
    AILOD_WICKED_LOW = 0,
    AILOD_LOWER = 1,
    AILOD_LOW = 2,
    AILOD_NORMAL = 3,
    AILOD_HIGH = 4,
    AILOD_CNT = 5
};

// VTABLE: SWBF2 0x007af818 CommandFsm
// VTABLE: SWBF2 0x007af858 Controller
// VTABLE: SWBF2 0x007af870 PblHandled
class UnitController : public Controller, public CommandFsm, public PblHandled
{
public:

    enum
    {
        STATE_WAIT = 1,
        STATE_COMBAT = 2,
        STATE_WAIT_SECONDARY = 3,
        STATE_BOARD = 4,
        STATE_UNBOARD = 5,
        STATE_FLY = 6,
        STATE_REPAIR = 7,
        STATE_RANDOM = 8,
        STATE_CAPTURECP = 9,
        STATE_CTFOFFENSE = 10,
        STATE_DEATHMATCH = 11,
        STATE_DEFEND = 12,
        STATE_DESTROY = 13,
        STATE_SC_FOLLOW = 14,
        STATE_SC_HOLD = 15,
        STATE_FINAL = 16
    };

    UnitController(Controllable* pOwner);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void KillController();

    int SendEvent(AI::EventInfo* pInfo);
    int SendEvent(AI::EventInfo* pInfo) const;

    float GetUpdateTime() const { return mNextUpdateTime; }
    float GetUpdateRate() const;
    PblOrderedList<UnitController, float>::Node* GetManagerNode() { return &mManagerNode; }
    const PblOrderedList<UnitController, float>::Node* GetManagerNode() const { return &mManagerNode; }
    void UpdateHighLevel();
    void UpdateLowLevel(float dt);

    void AddFriendlyFire(const GameObject* pObject);

    void ResetFriendlyList();

    int GetFriendlyFireCount() const;
    const GameObject* GetFriendlyFire(int iIndex);
    bool IsInCombat() const;
    bool IsAgentDone();
    CmdInfo CopyCurCmd() const;
    const SquadCommands::SquadCommandOrder& GetLastSquadCommand();
    const GameObject* GetAttackHelperTarget() const;
    const GameObject* GetAttackHelperVisibleTarget() const;
    const PblVector3& GetAttackHelperTargetLastSeenPosition() const;
    int GetRandomSeed() const;
    void SetRandomSeed(int iSeed);
    void SetLastShotDist(float fDistSq);
    bool IsTargetLOSPossiblyBlocked();
    void EnterCombat();

    void ForceSetAttackHelperTarget(const GameObject* pTarget);

    bool IsLookAheadOnly() const;
    const PblVector3& GetOriginalAheadDir() const;

    // SYNTHETIC: SWBF2 0x00499600
    // UnitController::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    static void* operator new(unsigned int uSize) { return sMemoryPool.Allocate(uSize); }
    static MemoryPool sMemoryPool;

    static unsigned int GetRtti() { return rttiHashUnitController; }
    static PblHash rttiHashUnitController;
    static const char rttiNameUnitController[];

    AILowLevel& GetLowLevel() { return mLowLevel; }
    const AILowLevel& GetLowLevel() const { return mLowLevel; }

protected:
    UnitController();
    virtual ~UnitController();

    virtual void EnterState();
    virtual void ExitState();
    virtual void UpdateState(float dt);

    const UnitCmdInfo* GetCurCmd() const;

    void DeleteAgent();
    void UpdateInvisibility();

    void UpdateLodState();
    void UseBuffs();
    void UseAutoTurrets();

public:
    PblOrderedList<UnitController, float>::Node mManagerNode;
    float mNextUpdateTime;
    float mLastUpdateTime;
    float mUpdateRate;
    char mLowLevelUpdateSkips;
    float mLowLevelUpdateSkipDt;
    AI::UnitThreatManager mThreatManager;
    UnitAgent* mAgent;
    AILowLevel mLowLevel;
    AILodState mLodState;
    const Character* mLodHumanPlayer;
    float mLodStateUpdateTime;
    SquadCommands::SquadCommandOrder mLastSquadCommand;
    bool mJustEnteredVehicleGoToPickupAgent;
    bool mLookAheadOnly;
    PblVector3 mOriginalAheadDir;
    int mSeed;
    float mCrowdCheckTime;
    int mFriendlyCount;
    PblHandle<const GameObject> mFriendly[8];
    bool mSkip;
    int mVisionSkip;
    float mLastShotDistSq;
    PblVector3 mLastShotDistPos;
    float mLastShotDistTime;
};

UnitController* UnitControllerPtr(Controller* pController);
const UnitController* UnitControllerPtr(const Controller* pController);
UnitController* UnitControllerPtr(Controllable* pControllable);
const UnitController* UnitControllerPtr(const Controllable* pControllable);

 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x00663050
inline bool UnitController::IsRtti(unsigned int uRtti) const
{
    if (rttiHashUnitController == uRtti)
    {
        return true;
    }
    return Controller::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00663080
inline unsigned int UnitController::GetDerivedRtti() const
{
    return rttiHashUnitController;
}

// FUNCTION: SWBF2 0x00663090
inline const char* UnitController::GetDerivedRttiName() const
{
    return rttiNameUnitController;
}


