#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Controller.h"
#include "PebbleFL/Common/PblList.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class Controllable;
class ProcessedInputs;

// VTABLE: SWBF2 0x007ad808
class PlayerController : public Controller
{
public:
    PlayerController(Controllable* pOwner);

    void LogVODeath(int iVictimTeam, int iKillerTeam);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void Update(float dt);

    float GetMouseBuildup();

    void SetMouseBuildup(float fBuildup) { mMouseTurnBuildup = fBuildup; }

    // SYNTHETIC: SWBF2 0x00499320
    // PlayerController::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    static void* operator new(unsigned int uSize) { return sMemoryPool.Allocate(uSize); }
    static MemoryPool sMemoryPool;

    static PblHash rttiHashPlayerController;
    static const char rttiNamePlayerController[];

protected:
    void DeployCargo();

    void CheckForAutoFollow(float dt);
    void CheckForVisibleCriticalPts();

public:
    PblList<PlayerController>::Node* GetManagerNode() { return &mManagerNode; }
    const PblList<PlayerController>::Node* GetManagerNode() const { return &mManagerNode; }

    PblList<PlayerController>::Node mManagerNode;
    ProcessedInputs* mJoystick;
    float mMouseTurnBuildup;
    bool mLanding;
    float mAutoAimScale;
    float mNextCritHitTime;
    float mEnemyDeaths[4];
    float mAllyDeaths[4];
    float mTeamKills[4];
    float mAutoFollowTime;
    bool mIsFireButtonDebounced;

    bool mUseButtonDebounced;
public:
    static int sEnabled;
};

PlayerController* PlayerControllerPtr(Controller* pCtrl);
const PlayerController* PlayerControllerPtr(const Controller* pCtrl);
PlayerController* PlayerControllerPtr(Controllable* pControllable);
const PlayerController* PlayerControllerPtr(const Controllable* pControllable);

 
 
 
 
 

// FUNCTION: SWBF2 0x0061a060
inline bool PlayerController::IsRtti(unsigned int uRtti) const
{
    if (rttiHashPlayerController == uRtti)
    {
        return true;
    }
    return Controller::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x0061a090
inline unsigned int PlayerController::GetDerivedRtti() const
{
    return rttiHashPlayerController;
}

// FUNCTION: SWBF2 0x0061a0a0
inline const char* PlayerController::GetDerivedRttiName() const
{
    return rttiNamePlayerController;
}


