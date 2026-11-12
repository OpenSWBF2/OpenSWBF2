#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "CollisionModel.h"
#include "CollisionObject.h"
#include "Chunk.h"
#include "Entity.h"
#include "PebbleFL/Common/PblRtti.h"
#include "FoleyFX.h"
#include "GameModel.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Util/RedSceneObject.h"

class AttachedEffects;
class AttachedEffectsClass;
class CollisionBodyPrimitive;
class GameModel;
class GameObject;
class RedLodData;
class RedPose;

 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x0079b634
class EntityGeometryClass : public EntityClass,
                            public FoleyFXCollideeClass,
                            public FoleyFXColliderClass
{
public:
    static const int kMaxChunks = 10;

    struct HitLocationData
    {
        int mBodyId;
        float mDamageMultiplier;
    };

    EntityGeometryClass(unsigned int uHash);

    EntityGeometryClass(EntityGeometryClass* pBase, unsigned int uHash);
    virtual ~EntityGeometryClass();

    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual const GameModel* GetModel();

    int GetNumChunks();

    virtual bool IsRtti(unsigned int uRtti) const;

    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;

    virtual void PostReadSetup();

    float GetPriorityMod() { return _fPriorityMod; }
    const PblVector3& GetTargetPointOffset() const { return mTargetPointOffset; }

protected:

    bool AddBuildingCollision(const char* pszValue);
    bool AddVehicleCollision(const char* pszValue);
    bool AddTerrainCollision(const char* pszValue);
    bool AddOrdnanceCollision(const char* pszValue);
    bool AddSoldierCollision(const char* pszValue);
    bool AddTargetableCollision(const char* pszValue);

public:

    static PblHash rttiHashEntityGeometryClass;
    static const char rttiNameEntityGeometryClass[];

    ChunkDescriptor* mChunkDescriptors[kMaxChunks];
    CollisionModel* mFullChunkCollisions[kMaxChunks];
    float mInitialChunkSpeed[kMaxChunks];
    float mChunkVelocityFactor[kMaxChunks];
    float mChunkUpFactor[kMaxChunks];
    bool m_bDynamicLighting;
    float mMaxShadowLength;
    float _fPriorityMod;
    unsigned int mChunkAttachPoints[kMaxChunks];
    GameModel* mModel;
    int mLODClass;
    CollisionModel mCollisionModel;
    float mPotentialCollisionRadius;
    HitLocationData* mHitLocations;
    int mNumHitLocations;
    AttachedEffectsClass* m_pAttachedEffectsClass;
    PblVector3 mTargetPointOffset;
};
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x0079b65c EntityEx
// VTABLE: SWBF2 0x0079b6b8 CollisionObject
// VTABLE: SWBF2 0x0079b70c RedSceneObject
class EntityGeometry : public EntityEx, public CollisionObject, public RedSceneObject
{
public:
    // FUNCTION: SWBF2 0x004440d0
    // EntityGeometry::IsRtti
    // FUNCTION: SWBF2 0x004ba080
    // EntityGeometry::GetDerivedRtti
    // FUNCTION: SWBF2 0x004ba090
    // EntityGeometry::GetDerivedRttiName
    PBL_DECLARE_DERIVED_RTTI(EntityGeometry, EntityEx)

    EntityGeometry(EntityGeometryClass* pClass, const EntityDesc& desc, CollisionObjectType eType,
                   const RedLodData* pLodData);

    virtual ~EntityGeometry();
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    // FUNCTION: SWBF2 0x00444100
    virtual const PblVector3& GetPosition() const { return mMatrix.GetTranslation(); }

    // FUNCTION: SWBF2 0x004920e0
    virtual void SetPosition(const PblVector3& vPos)
    {
        PblMatrix& rMatrix = mMatrix;
        rMatrix[3] = vPos;
        rMatrix.m[3][3] = 1.0f;

        PblVector3 vCenter;
        D3DXVec3TransformCoord((D3DXVECTOR3*)&vCenter,
                               (const D3DXVECTOR3*)&mModel->_mModel->_BoundingSphere.GetCenter(),
                               (const D3DXMATRIX*)&rMatrix);
        SetCollisionCenter(vCenter);
        RecalculateAABB();
        SetRenderPos(GetCollisionCenter());
    }

    virtual float GetSmallestCollisionDim() const;
    // FUNCTION: SWBF2 0x00444120
    virtual Entity* GetEntity() { return this; }
    // [thunk]:EntityGeometry::GetEntity`adjustor{12}'
    // SYNTHETIC: SWBF2 0x00446e2c
    // ?GetEntity@EntityGeometry@@WM@BEPBVEntity@@XZ

    // FUNCTION: SWBF2 0x00444110
    virtual const Entity* GetEntity() const { return this; }

    // FUNCTION: SWBF2 0x00497e40
    virtual GameObject* GetGameObject() { return 0; }
    // FUNCTION: SWBF2 0x00497e30
    virtual const GameObject* GetGameObject() const { return 0; }

    // FUNCTION: SWBF2 0x00444130
    virtual const CollisionObject* GetCollisionObject() const { return this; }

    // FUNCTION: SWBF2 0x00444140
    virtual CollisionObject* GetCollisionObject() { return this; }
    // FUNCTION: SWBF2 0x00444150
    virtual const GameModel* GetModel() const { return mModel; }

    // FUNCTION: SWBF2 0x00444160
    virtual void SetModel(const GameModel* pModel)
    {
        mModel = const_cast<GameModel*>(pModel);
        _pLodData = &mModel->_mLodData;
    }
    // FUNCTION: SWBF2 0x00444180
    virtual const RedPose* GetPose() const { return 0; }

    // FUNCTION: SWBF2 0x00444190
    virtual void SetupPose(RedPose& pose) const
    {
        mModel->GetRedModel(0)->SetupPose(pose);
    }
    // FUNCTION: SWBF2 0x004921c0
    virtual void SetMatrix(const PblMatrix& matrix)
    {
        mMatrix = matrix;

        PblVector3 vCenter;
        D3DXVec3TransformCoord((D3DXVECTOR3*)&vCenter,
                               (const D3DXVECTOR3*)&mModel->_mModel->_BoundingSphere.GetCenter(),
                               (const D3DXMATRIX*)&mMatrix);
        SetCollisionCenter(vCenter);
        RecalculateAABB();
        SetRenderPos(GetCollisionCenter());
    }
    // FUNCTION: SWBF2 0x00475da0
    virtual const PblMatrix& GetMatrix() const { return mMatrix; }
    virtual const PblVector3& GetVelocity() const;

    // FUNCTION: SWBF2 0x00475dc0
    virtual void SetVelocity(const PblVector3&) {}
    virtual int GetTargetBodyID(const PblVector3& vPos, const PblVector3& vDir) const;
    virtual PblVector3 GetTargetPoint(const PblVector3& vPos, const PblVector3& vDir, int iBodyId) const;
    virtual PblVector3 GetTargetPointMissVector() const;

    void CreateChunkCluster(float fScale);

    void BuildEffects();

    bool SetupShadow(const GameObject* pObject, const GameModel* pModel, unsigned char ucLod,
                     unsigned int& uFlags, unsigned int uShadowTexture);

    void SetupLightingState(unsigned int& uFlags);

    PblMatrix mMatrix;
    GameModel* mModel;
    float mSmallestCollisionDim;
    AttachedEffects* m_pAttachedEffects;


};
 
 
 
 
 

// FUNCTION: SWBF2 0x004441b0
inline bool EntityGeometryClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityGeometryClass == uRtti)
    {
        return true;
    }
    return EntityClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004ba0d0
inline unsigned int EntityGeometryClass::GetDerivedRtti() const
{
    return rttiHashEntityGeometryClass;
}

// FUNCTION: SWBF2 0x004ba0e0
inline const char* EntityGeometryClass::GetDerivedRttiName() const
{
    return rttiNameEntityGeometryClass;
}
