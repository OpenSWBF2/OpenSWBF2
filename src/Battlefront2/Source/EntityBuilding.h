#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "CollisionModel.h"
#include "GameObject.h"
#include "GameSound.h"
#include "PebbleFL/Common/PblList.h"
#include "PebbleFL/Common/PblVector.h"

class CollisionMesh;
class Controllable;
class EntityHologram;
class EntityHologramClass;
class ExplosionClass;
class Obstacle;
class RedPose;
class RedShadowProxy;
struct EntityDesc;

// VTABLE: SWBF2 0x00799540
class EntityBuildingClass : public GameObjectClass
{
public:
    EntityBuildingClass(unsigned int uHash);

    EntityBuildingClass(EntityBuildingClass* pBase, unsigned int uHash);
    virtual ~EntityBuildingClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    bool IsDestructableBuilding();

    static PblHash rttiHashEntityBuildingClass;
    static const char rttiNameEntityBuildingClass[];

    int mLodType;
    GameModel* mDestroyedModel;
    CollisionModel mDestroyedCollisionModel;
    GameModel* mUnbuiltModel;
    CollisionModel mUnbuiltCollisionModel;
    EntityHologramClass* m_pHologramClass;
    ExplosionClass* mExplosion;
    PblVector3 mExplosionOffset;
    float mRespawnTime;
    GameSound mBuiltSound;
    GameSound mAmbientBuildingSound;
    bool mAutoCreateBarrier;
    bool mDisableBarrierWhenDead;
    bool mColorizeByTeam;
    float mBarrierX;
    float mBarrierZ;
    float mBarrierOX;
    float mBarrierOZ;
    bool mIsDestructable;
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x007993ec Damageable
// VTABLE: SWBF2 0x00799418 EntityEx
// VTABLE: SWBF2 0x0079958c RedSceneObject
// VTABLE: SWBF2 0x007995f4 PblHandled
// VTABLE: SWBF2 0x007995fc CollisionObject
class EntityBuilding : public GameObject
{
public:
    enum STATE
    {
        STATE_ALIVE = 0,
        STATE_DEAD = 1,
        STATE_UNBUILT = 2,
        STATE_DYING = 3,
    };

    EntityBuilding(EntityBuildingClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~EntityBuilding();
    virtual void ClientKill();
    virtual bool IsUnbuilt() const;
    virtual float GetMaxSpeed() const;
    virtual float GetMaxStrafe() const;
    virtual const Controllable* GetControllable() const;
    virtual Controllable* GetControllable();
    virtual GameObjectType GetGameObjectType();
    virtual EntityType GetEntityType() const;
    virtual const PblMatrix* GetRenderMatrix() const;
    virtual const RedPose* GetRenderPose() const;

    virtual void Render(unsigned char uFlags, float fDist, unsigned int uMask);
    virtual const PblMatrix& GetBBoxMatrix() const;
    virtual const PblBox& GetBBox() const;

    virtual void SetPosition(const PblVector3& vPos);

    virtual void SetMatrix(const PblMatrix& matrix);
    virtual void RecalculateAABB();

    virtual void Kill();
    virtual void Respawn();

    virtual bool ApplyDamage(const DamageDesc& damage, const PblVector3& vDir, const PblVector3& vPos,
                             int iBodyPart, unsigned int uFlags);

    virtual void HealthChangeCallback(bool bArg, float fDelta);

    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual GameObject* AddToClient();

    void Update(float fDeltaTime);

    static PblHash rttiHashEntityBuilding;
    static const char rttiNameEntityBuilding[];

    EntityBuildingClass* mClass;
    STATE mState;
    CollisionMesh* mCollision;
    RedShadowProxy* mpShadow;
    EntityHologram* m_pHologram;
    Obstacle* mObstacle;
    GameSoundControllable mAmbientSound;
};
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x00475ef0
inline bool EntityBuildingClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityBuildingClass == uRtti)
    {
        return true;
    }
    return GameObjectClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00492390
inline unsigned int EntityBuildingClass::GetDerivedRtti() const
{
    return rttiHashEntityBuildingClass;
}

// FUNCTION: SWBF2 0x004923a0
inline const char* EntityBuildingClass::GetDerivedRttiName() const
{
    return rttiNameEntityBuildingClass;
}

// FUNCTION: SWBF2 0x00475de0
inline bool EntityBuilding::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityBuilding == uRtti)
    {
        return true;
    }
    return GameObject::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00492330
inline unsigned int EntityBuilding::GetDerivedRtti() const
{
    return rttiHashEntityBuilding;
}

// FUNCTION: SWBF2 0x00492340
inline const char* EntityBuilding::GetDerivedRttiName() const
{
    return rttiNameEntityBuilding;
}
