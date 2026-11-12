#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "GameObject.h"

class ExplosionClass;

// VTABLE: SWBF2 0x00796e98
class AsteroidClass : public GameObjectClass
{
public:
    AsteroidClass(unsigned int uHash);

    AsteroidClass(AsteroidClass* pBase, unsigned int uHash);
    virtual ~AsteroidClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    void FillRegion(const char* pcRegion, int iCount, PblVector3 vMin, PblVector3 vMax);
    void FillPath(const char* pcPath, float fDensity, int iCount,
                    PblVector3 vMin, PblVector3 vMax);

    static PblHash rttiHashAsteroidClass;
    static const char rttiNameAsteroidClass[];

    struct ChildAsteroidDesc
    {
        unsigned int mClassHash;
        unsigned int mHardPointHash;
    };

    float mMinSpeed;
    float mMaxSpeed;
    float mMinOmega;
    float mMaxOmega;
    int mCollidable;

    ChildAsteroidDesc mChildren[10];
    int mNumChildren;
    ExplosionClass* mExplosionBreak;
    ExplosionClass* mExplosionDestruct;
};
  
 
 
 
 
 
 
 
 
 

#include "Thread.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

// VTABLE: SWBF2 0x00796d00 EntityEx
// VTABLE: SWBF2 0x00796cac CollisionObject
// VTABLE: SWBF2 0x00796e20 RedSceneObject
// VTABLE: SWBF2 0x00796c80 Damageable
// VTABLE: SWBF2 0x00796e88 PblHandled
// VTABLE: SWBF2 0x00796c68
class Asteroid : public GameObject, public Thread
{
public:

    static PblHash rttiHashAsteroid;
    static const char rttiNameAsteroid[];

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    static MemoryPool sMemoryPool;
    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x00444530
    // Asteroid::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    Asteroid(AsteroidClass* pClass, const EntityDesc& desc);
    virtual ~Asteroid();
    virtual void Init();
    virtual void Kill();
    virtual void ClientKill();
    virtual bool Update(float fDeltaTime);
    virtual const PblMatrix& GetMatrix() const;
    virtual const PblVector3& GetVelocity() const;
    virtual void SetPosition(const PblVector3& vPos);
    virtual void SetMatrix(const PblMatrix& matrix);

    // FUNCTION: SWBF2 0x004446b0
    virtual float GetMaxSpeed() const
    {
        return mVelocity.y * mMatrix.m[2][1] + mVelocity.x * mMatrix.m[2][0] +
               mVelocity.z * mMatrix.m[2][2];
    }
    virtual float GetMaxStrafe() const;
    virtual bool AddToTeamManager() const;

    // FUNCTION: SWBF2 0x00444700
    virtual void SetVelocity(const PblVector3& vVelocity)
    {
        mVelocity = vVelocity;
    }

    virtual void Render(unsigned char ucLod, float fDist, unsigned int uMask);
    virtual bool CollisionCallback(const CollisionResult* pResult, CollisionObject* pOther,
                                   Restrictor* pRestrictor);
    virtual GameObjectType GetGameObjectType();
    virtual void ActivatePhysics();
    virtual void DeactivatePhysics();

    // FUNCTION: SWBF2 0x00444730
    virtual EntityType GetEntityType() const
    {
        return ENT_ASTEROID;
    }
    virtual void AddSends(void (*pfnSend)(GameObject*, int), int iArg);
    virtual void WriteCreate(NetPktGroup& group);
    virtual void ReadCreate(NetPktGroup& group);
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    void Move(const PblMatrix& matrix);
    virtual const PblMatrix& GetSmoothedMatrix() const;

    PblVector3 mVelocity;
    PblVector3 mOmega;
    AsteroidClass* mClass;
};
 
 
 
 

// FUNCTION: SWBF2 0x00444790
inline bool AsteroidClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashAsteroidClass == uRtti)
    {
        return true;
    }
    return GameObjectClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004447d0
inline unsigned int AsteroidClass::GetDerivedRtti() const
{
    return rttiHashAsteroidClass;
}

// FUNCTION: SWBF2 0x004447e0
inline const char* AsteroidClass::GetDerivedRttiName() const
{
    return rttiNameAsteroidClass;
}

// FUNCTION: SWBF2 0x004444d0
inline bool Asteroid::IsRtti(unsigned int uRtti) const
{
    if (rttiHashAsteroid == uRtti)
    {
        return true;
    }
    return GameObject::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00444510
inline unsigned int Asteroid::GetDerivedRtti() const
{
    return rttiHashAsteroid;
}

// FUNCTION: SWBF2 0x00444520
inline const char* Asteroid::GetDerivedRttiName() const
{
    return "Asteroid";
}


