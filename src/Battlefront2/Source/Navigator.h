#pragma once

#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblRtti.h"

#include "Fsm.h"
#include "ConnectivityGraph.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class AILowLevel;
class AIPath;
class GameObject;
class Obstacle;
class PathFollower;
class PblMatrix;
class PblVector3;

// VTABLE: SWBF2 0x007aa790
class Navigator : public Fsm
{
public:

    enum
    {
        STATE_START = 1
    };

    // FUNCTION: SWBF2 0x005ab2d0
    // Navigator::IsRtti
    // FUNCTION: SWBF2 0x005ab2f0
    // Navigator::GetDerivedRtti
    // FUNCTION: SWBF2 0x005ab300
    // Navigator::GetDerivedRttiName
    PBL_DECLARE_ROOT_RTTI(Navigator)

    static void* operator new(unsigned int uiSize);
    // SYNTHETIC: SWBF2 0x0043ab10
    // Navigator::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }
    static MemoryPool sMemoryPool;


    virtual ~Navigator();

    virtual void ReceivePathResult(AIPath* pPath);
    virtual const PblVector3& GetGlobalDest() const;
    virtual const PblVector3& GetLocalDest() const;

    PathFollower* GetPathFollower();
    const PathFollower* GetPathFollower() const;
    bool IsStuck() const;
    bool IsFailedPathSearch() const;
    void SetFailedPathSearch();
    void Update(float dt);
    void UpdateSkip();
    void ForgetBarrier(const Obstacle* pBarrier);

    static void ForgetIgnored(const Obstacle** apIgnore, int iCnt, const Obstacle* pBarrier)
    {
        for (int i = 0; i < iCnt; ++i)
        {
            if (apIgnore[i] == pBarrier)
            {
                apIgnore[i] = 0;
            }
        }
    }

protected:
    Navigator(AILowLevel* pOwner);

    const PblMatrix& GetMatrix() const;

    GameObject* GetGameObject();
    const GameObject* GetGameObject() const;
    float GetMyRadius() const;
    ConnectivityGraph::ProhibitFlag GetMyProhibit() const;
    const Obstacle* GetMyBarrier() const;
    bool AttemptWallFollow(float fStep, bool bRight);
    bool LookForPowerUps(float fRadius);
    bool LookForCoverNodes();

    AILowLevel* mOwner;
    PathFollower* mPathFollower;
    float mWallFollowTime;
    float mPowerUpTime;
    float mCoverNodeTime;
    bool mFailedPathSearch;
};

 


