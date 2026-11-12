#pragma once

#include <stddef.h>
#include <string.h>

#include "CollisionBody.h"
#include "CollisionModel.h"
#include "CollisionObject.h"
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Util/RedSceneObject.h"
#include "Thread.h"

class ChunkCluster;
class ChunkDescriptor;
class EntityGeometryClass;
class CollisionResult;
class FLEffectObject;
class GameModel;
class Restrictor;

enum ChunkPhysicsType
{
    PHYSICS_FULL_RIGID = 0,
    PHYSICS_SIMPLE_RIGID = 1,
    PHYSICS_STATIC = 2,
    PHYSICS_LEAF = 3,
    PHYSICS_COCKPIT = 4
};

class ChunkDescriptor
{
public:

    // FUNCTION: SWBF2 0x004b9f50
    void Init()
    {
        mMatrix.Reset();

        mVelocity.z = 0.0f;
        mVelocity.y = 0.0f;
        mVelocity.x = 0.0f;
        mModel = 0;
        mPrimitive = 0;
        mMaxTerrainCollisions = 0;
        mTerrainImpactEffect = 0;
        mPhysicsType = PHYSICS_FULL_RIGID;
        mOmega.z = 0.0f;
        mOmega.y = 0.0f;
        mOmega.x = 0.0f;
        mFullCollisionModel = 0;
        mCoefficientOfRestitution = 0.0f;
        mCoefficientOfFriction = 0.0f;
        mSimpleFrictionConstant = 0.0f;
        mMinDeathCollisionSpeed = 0.0f;
        mForceZeroYAxisVelocity = false;
        memset(mTrailEffect, 0, sizeof(mTrailEffect));
        memset(mSmokeEffect, 0, sizeof(mSmokeEffect));
        memset(mTrailAttachPoints, 0, sizeof(mTrailAttachPoints));
        memset(mSmokeAttachPoints, 0, sizeof(mSmokeAttachPoints));
        mGravity = PblVector3(0.0f, -9.8f, 0.0f);
        mNoBuildingCollisions = false;
        mKeepSoldierCollisions = false;
        mLinearDamping = 0.0f;
        mAngularDamping = 0.0f;
        mMaxStaticLifetime = -1.0f;
    }

    PblMatrix mMatrix;
    PblVector3 mVelocity;
    GameModel* mModel;
    CollisionBodyPrimitive* mPrimitive;
    unsigned char mMaxTerrainCollisions;
    unsigned int mTerrainImpactEffect;

    ChunkPhysicsType mPhysicsType;
    PblVector3 mOmega;
    PblVector3 mGravity;
    CollisionModel* mFullCollisionModel;
    float mCoefficientOfRestitution;
    float mCoefficientOfFriction;
    float mSimpleFrictionConstant;
    float mMinDeathCollisionSpeed;
    bool mForceZeroYAxisVelocity;
    float mMaxStaticLifetime;
    unsigned int mTrailEffect[3];
    unsigned int mSmokeEffect[3];
    unsigned int mTrailAttachPoints[3];
    unsigned int mSmokeAttachPoints[3];
    bool mNoBuildingCollisions;
    bool mKeepSoldierCollisions;
    float mLinearDamping;
    float mAngularDamping;
};

// VTABLE: SWBF2 0x0079721c
class Chunk : public CollisionObject
{
public:
    Chunk();
    // FUNCTION: SWBF2 0x00454e30
    virtual void SetPosition(const PblVector3& vPos)
    {
        mMatrix.SetTranslation(vPos);
        mCollisionMatrix = mPrimitiveMatrix.GetInverseFast() * mMatrix;
        SetCollisionCenter(mCollisionMatrix.GetTranslation());
    }

    // FUNCTION: SWBF2 0x00454f10
    virtual const PblVector3& GetPosition() const
    {
        return mMatrix.GetTranslation();
    }

    // FUNCTION: SWBF2 0x00454f20
    virtual const PblMatrix& GetMatrix() const
    {
        return mMatrix;
    }

    // FUNCTION: SWBF2 0x00454f30
    virtual const PblVector3& GetVelocity() const
    {
        return mVelocity;
    }

    // FUNCTION: SWBF2 0x00454f40
    virtual const PblVector3& GetOmega() const
    {
        return mOmega;
    }

    virtual bool CollisionCallback(const CollisionResult* pResult, CollisionObject* pOther,
                                   Restrictor* pRestrictor);
    virtual bool IsAChunk() const;

    bool IsValid() const { return mIsValid; }
    bool IsMoving() const { return mIsMoving; }

    void ClearAll();
    void FixPosition();
    void SetData(ChunkDescriptor& desc);
    void Update(float fDeltaTime);
    void Render(unsigned char ucAlpha, float fDist, unsigned int uiFlags);

protected:
    void CreateTrailEmitters();
    void CreateSmokeEmitters();
    void DoTerrainImpactEffect(const PblVector3& vPos);
    bool DoSimpleRigidCollision(const CollisionResult* pResult, CollisionObject* pOther,
                                Restrictor* pRestrictor);
    bool DoFullRigidCollision(const CollisionResult* pResult, CollisionObject* pOther,
                              Restrictor* pRestrictor);

public:
    unsigned char mID;
    ChunkCluster* mParentCluster;
    bool mIsValid;
    bool mIsMoving;
    PblMatrix mMatrix;
    PblMatrix mCollisionMatrix;
    PblMatrix mPrimitiveMatrix;
    GameModel* mModel;
    CollisionBodyPrimitive mBodyPrimitive;
    CollisionModel mSimpleCollisionModel;
    CollisionModel* mFullCollisionModel;
    PblVector3 mVelocity;
    bool bCollisionOnUpdate;
    PblVector3 mOmega;
    PblVector3 mGravity;
    float mCoefficientOfRestitution;
    float mCoefficientOfFriction;
    float mSimpleFrictionConstant;
    float mMaxStaticLifetime;
    float mMinDeathCollisionSpeed;
    bool mForceZeroYAxisVelocity;
    PblVector3 mAngularMomentum;
    float mMass;
    float mOneOverMass;
    PblMatrix mInverseBodyInertiaTensor;
    PblMatrix mInverseWorldInertiaTensor;
    unsigned char mMaxTerrainCollisions;
    unsigned char mNumTerrainCollisions;
    unsigned int mTerrainImpactEffect;
    unsigned int mTrailEffect[3];
    unsigned int mSmokeEffect[3];
    ChunkPhysicsType mPhysicsType;
    unsigned int mTrailAttachPoints[3];
    unsigned int mSmokeAttachPoints[3];
    PblHandle<FLEffectObject> mTrailEmitters[3];
    PblHandle<FLEffectObject> mSmokeEmitters[3];
    unsigned char mForwardAxis;
    float mLifeTime;
    float mLinearDamping;
    float mAngularDamping;
    float mTotalUpdateDt;
    bool mJustRendered;
};
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x007971b4
class ChunkCluster : public RedSceneObject
{
public:
    static const int kMaxChunks = 10;
    ChunkCluster();
    virtual ~ChunkCluster();
    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);

    bool IsValid();
    bool HasStaticChunks();
    void ClearChunks();
    void RecalculateCenterAndRadius();
    void AddChunk(ChunkDescriptor& desc);
    bool Update(float fDeltaTime);

    int mID;
    Chunk mChunks[kMaxChunks];
    EntityGeometryClass* mParentClass;
};
 
 
 
 

// VTABLE: SWBF2 0x00797278
class ChunkClusterManager : public Thread
{
public:
    static const int kMaxClusters = 8;
    ChunkClusterManager();
    virtual ~ChunkClusterManager();
    virtual bool Update(float fDeltaTime);

    static bool Create();
    static bool CreateClient(bool bArg);
    static bool Destroy();
    static bool DestroyClient();
    static void EnterClient();
    static void LeaveClient();
    static ChunkCluster* GetFreeCluster(bool bArg);
    static ChunkCluster* GetClusterByID(unsigned char ucID, bool bArg);

    unsigned char mOldestChunkCluster;
    ChunkCluster mChunkClusters[kMaxClusters];

    static ChunkClusterManager* sInstance;
    static ChunkClusterManager* sHostInstance;
    static ChunkClusterManager* sClientInstance;
};
 
 
 
