#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblVector.h"

#include "PebbleFL/Common/PblList.h"
#include "PebbleFL/Common/PblSerial.h"
#ifdef OPENSWBF2_TEST
#include "RedEngineFL/Util/RedLog.h"
#endif
#include "CollisionObjectType.h"
#include "FoleyFXCollidee.h"
#include "FoleyFXCollider.h"
#include "TreeGrid.h"
class CollisionModel;
class CollisionObject;
class CollisionOptimizationInfo;
class CollisionResult;
class Entity;
class FoleyFX;
class FoleyFXCollideeClass;
class FoleyFXColliderClass;
class FoleyFXGroup;
class GameObject;
class PblMatrix;
class PblVector3;
class RedPose;
class Restrictor;
class MemoryPool;
class TreeGridObject;
class TreeGridStack;

// VTABLE: SWBF2 0x00797460
class CollisionObject : public TreeGridObject, public FoleyFXCollider, public FoleyFXCollidee
{
public:
    static const int kNumLastCollided = 3;

    virtual ~CollisionObject();
    virtual const PblVector3 & GetPosition() const = 0;
    // FUNCTION: SWBF2 0x00463a70
    virtual void SetPosition(const PblVector3 &)
    {
    }
    // FUNCTION: SWBF2 0x00454dd0
    virtual void SetMatrix(const PblMatrix &)
    {
    }
    // FUNCTION: SWBF2 0x00443fc0
    virtual bool CanProcedurallyAnimate()
    {
        return true;
    }
    virtual float GetSmallestCollisionDim() const;
    virtual bool CollisionCallback(const CollisionResult *, CollisionObject *, Restrictor *);
    virtual float RayHit(const PblVector3 &, const PblVector3 &, float, PblVector3 *, int *, bool, float);
    virtual bool RayTest(const PblVector3 &, const PblVector3 &, float);
    virtual bool CheckForCollision(CollisionObject *, Restrictor *, CollisionOptimizationInfo *);
    virtual void SetTerrainHeight(int, float);
    virtual Entity * GetEntity();
    virtual const Entity * GetEntity() const;
    virtual GameObject * GetGameObject();
    virtual const GameObject * GetGameObject() const;
    // FUNCTION: SWBF2 0x00443fe0
    virtual bool CollideTrisWithAllPrimitives() const
    {
        return true;
    }
    virtual const PblMatrix & GetMatrix() const = 0;
    virtual const RedPose * GetPose() const;
    // FUNCTION: SWBF2 0x00443ff0
    virtual bool IsAChunk() const
    {
        return false;
    }

    virtual void RecalculateAABB();

    CollisionObject(CollisionObjectType eType, const CollisionModel* pModel,
                    FoleyFXColliderClass* const pColliderClass,
                    FoleyFXCollideeClass* const pCollideeClass);

    void AddStaticToCollisionManager();

    void AddSoftToCollisionManager();

    void AddRigidToCollisionManager();

    void AddItemToCollisionManager();
    void AddAsteroidToCollisionManager();
    void AddAttachedToCollisionManager();
    void AddShieldToCollisionManager();

    void SetMass(float fMass) { mMass = fMass; }

    float GetMass() const { return mMass; }
    void RemoveFromCollisionManager();
    bool IsInCollisionList();

    void SeparateObject(const PblVector3& vNormal, float fOverlap);
    void RestrictObject(const PblVector3& vNormal, float fOverlap, Restrictor* pRestrictor);

    void SetCollisionModel(CollisionModel* pModel);

    void PlayImpactEffects(const PblVector3* pPos, const PblVector3* pVel) const;

    void SetCollisionType(CollisionObjectType eType)
    {
        SetObjectData(0, eType);
    }
    CollisionObjectType GetCollisionType() const;

    // FUNCTION: SWBF2 0x0045db10
    const CollisionModel* GetCollisionModel() const { return mCollisionModel; }

    PblList<CollisionObject>::Node mCollisionObjectNode;
    CollisionModel* mCollisionModel;
    CollisionObject* mParent;
    float mMass;
    PblBox mAABB;
    unsigned int mProcedurallyAnimated : 1;
    unsigned int mLastCollidedWithNonTerrain : 1;
    unsigned int mCollisionSerial : 30;
    PblVector3 mLastPosition;
    unsigned int mLastCollidedObjs[kNumLastCollided];

    static bool sCollidingWithMesh;
    static bool sCollideTrisWithAllPrimitives;

    static bool CollisionSerialAfter(const CollisionObject* pA, const CollisionObject* pB)
    {
        return PblSerialAfter(pA->mCollisionSerial, pB->mCollisionSerial);
    }
    static unsigned int LastCollidedKey(const CollisionObject* pObject)
    {
        if (pObject == 0)
        {
            return 0;
        }
        return pObject->mCollisionSerial;
    }

    void NoteLastCollided(const CollisionObject* pReceiver)
    {
        const unsigned int uKey = LastCollidedKey(pReceiver);
        for (int k = 0; k < kNumLastCollided; ++k)
        {
            if (mLastCollidedObjs[k] == uKey)
            {
                return;
            }
        }
        mLastCollidedObjs[2] = mLastCollidedObjs[1];
        mLastCollidedObjs[1] = mLastCollidedObjs[0];
        mLastCollidedObjs[0] = uKey;
    }

    void PrioritizeLastCollided(CollisionObject** apList, int iLength) const
    {
        for (int i = 0; i < kNumLastCollided; ++i)
        {
            if (i >= iLength)
            {
                break;
            }
            for (int j = 0; j < iLength; ++j)
            {
                if (mLastCollidedObjs[i] == LastCollidedKey(apList[j]) && j != i)
                {
                    CollisionObject* pSwap = apList[i];
                    apList[i] = apList[j];
                    apList[j] = pSwap;
                    break;
                }
            }
        }
    }

    void AssignCollisionSerial();
    static void ResetCollisionSerials();

#ifdef OPENSWBF2_TEST
    static inline unsigned int sCollisionSerialTies = 0;

    static void NoteCollisionSerialTie(const CollisionObject* pA, const CollisionObject* pB)
    {
        if (pA->mCollisionSerial == pB->mCollisionSerial)
        {
            ++sCollisionSerialTies;
        }
    }

    static void LogSerialTies(bool bReset)
    {
        LOG(LogSim, Inform, "OPENSWBF2 COLLISION serialties=%u", sCollisionSerialTies);
        if (bReset)
        {
            sCollisionSerialTies = 0;
        }
    }
#endif
};
 
 
 
 
 

// FUNCTION: SWBF2 0x0043ec10
inline CollisionObjectType CollisionObject::GetCollisionType() const
{
    const CollisionObject* pObject = (const CollisionObject*)this;
    return (CollisionObjectType)pObject->GetObjectData(0);
}
