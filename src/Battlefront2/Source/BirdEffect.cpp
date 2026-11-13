#include <math.h>
#include <float.h>
#include <string.h>
#include <stddef.h>
#include <d3dx9math.h>

#include "BirdEffect.h"
#include "CameraManager.h"
#include "CollisionManager.h"
#include "CollisionObject.h"
#include "GameObject.h"
#include "RedEngineFL/Graphics/RedCamera.h"
#include "Battlefront2Lod.h"
#include "PebbleFL/Common/PblMath.h"
#include "PebbleFL/Common/PblQuaternion.h"
#include "PebbleFL/Common/PblRandom.h"
#include "PebbleFL/Common/PblHashTable.h"

#include "RedEngineFL/Application/RedTerrain.h"

#include "RedEngineFL/Graphics/RedOmniLight.h"
#include "LensFlare.h"

class PblMatrix;
extern PblMatrix gMatrixIdentity;

static const float EPSILON = 0.0001f;
static const float fHalfPi = PblMath::fPi * 0.5f;







// GLOBAL: SWBF2 0x01e30314
BirdParticleManager* BirdParticleManager::sInstance;

// GLOBAL: SWBF2 0x01e302ec
int BirdParticleManager::sNumBirdTypes;
// GLOBAL: SWBF2 0x01e302f0
BirdParticleType BirdParticleManager::sBirdTypes[3];

extern NoLodData noloddata;

// FUNCTION: SWBF2 0x00448980
BirdParticle::BirdParticle()
    : RedSceneObject(&noloddata)
{
    mIsValid = false;
}

// SYNTHETIC: SWBF2 0x004489b0
// BirdParticle::`scalar deleting destructor'

// FUNCTION: SWBF2 0x004489e0
BirdParticle::~BirdParticle()
{
    mBirdSound.Stop(true);
}

// FUNCTION: SWBF2 0x00448a50
void BirdParticle::StartBirdSound()
{
    mBirdSound.Play(&BirdParticleManager::sInstance->mBirdSound, 0, 0, true);
}

// FUNCTION: SWBF2 0x00448a80
void BirdParticle::RespawnGroundBird(const PblVector3& vPos, const BirdParticleType& type)
{
    mState = BIRD_WALK_CIRCLE;
    mTimeOffCamera = 0.0f;
    mTotalLifetime = 0.0f;
    mFlapTime = PblRandom::_Global.GetFloat() * 6.2831855f;
    mVelocity = PblVector3(0.0f, 0.0f, 0.0f);

    PblQuaternion q = PblQuaternion(gMatrixIdentity);
    {
        PblVector3 initomega = PblVector3(0.0f, PblRandom::_Global.GetFloat() * 6.2831855f, 0.0f);
        float initAngle = Length(initomega);
        if (initAngle > EPSILON)
        {
            q *= PblQuaternion(initomega / initAngle, initAngle);
        }
    }
    {
        PblVector3 initomega = PblVector3(fHalfPi * -0.5f, 0.0f, 0.0f);
        float initAngle = Length(initomega);
        if (initAngle > EPSILON)
        {
            q *= PblQuaternion(initomega / initAngle, initAngle);
        }
    }

    mTotalCircleTime = (PblRandom::_Global.GetFloat() + 0.7f) * 10.0f;
    mCircleRadius = PblRandom::_Global.GetFloat() * 2.0f;
    mCircleCenter = vPos;
    mCircleTime = PblRandom::_Global.GetFloat() * mTotalCircleTime;

    PblVector3 vNewPos = mCircleCenter;
    float fSin, fCos;
    PblMath::SinCos(&fSin, &fCos, mCircleTime / mTotalCircleTime * 6.2831855f);
    vNewPos.z += mCircleRadius * fCos;
    vNewPos.x += mCircleRadius * fSin;
    vNewPos.y = RedTerrain::GetHoleHeight(vNewPos) + type.mSize * 0.125f;

    mMatrix = PblMatrix(q, vNewPos);
    mInitMatrix = mMatrix;
    mIsValid = true;
    mType = type;

    SetRenderRadius(mType.mSize);
    SetRenderPos(vNewPos);
    AddToScene();
    StartBirdSound();
    mBirdSound.Update(&BirdParticleManager::sInstance->mBirdSound, &mMatrix.GetTranslation(), &mVelocity);

    float angle = fHalfPi * 0.9f;
    mWing1Matrix = gMatrixIdentity;
    q = PblQuaternion(mWing1Matrix);
    q *= PblQuaternion(PblVector3(0.0f, 0.0f, 1.0f), angle);
    q *= PblQuaternion(PblVector3(0.0f, 1.0f, 0.0f), fHalfPi * 0.5f);
    mWing1Matrix = PblMatrix(q, PblVector3(0.0f, 0.0f, 0.0f));
    angle = fHalfPi * 0.9f + PblMath::fPi;
    mWing2Matrix = gMatrixIdentity;
    q = PblQuaternion(mWing2Matrix);
    q *= PblQuaternion(PblVector3(0.0f, 0.0f, 1.0f), angle);
    q *= PblQuaternion(PblVector3(0.0f, 1.0f, 0.0f), fHalfPi * 0.5f);
    mWing2Matrix = PblMatrix(q, PblVector3(0.0f, 0.0f, 0.0f));
}

// FUNCTION: SWBF2 0x00449380
void BirdParticle::RespawnFlockBird(const PblVector3& vPos, const PblVector3& vVelocity,
                                    const BirdParticleType& type)
{
    PblVector3 vUp(0.0f, 1.0f, 0.0f);

    mState = BIRD_FLOCK;
    mTimeOffCamera = 0.0f;
    mTotalLifetime = 0.0f;
    mFlapTime = PblRandom::_Global.GetFloat() * (PblMath::fPi * 2.0f);
    mVelocity = vVelocity;

    PblVector3 vDir = Normalize(mVelocity);

    mMatrix[0] = Normalize(CrossProduct(vUp, vDir));
    mMatrix.m[0][3] = 0.0f;
    mMatrix[1] = vUp;
    mMatrix.m[1][3] = 0.0f;
    mMatrix[2] = vDir;
    mMatrix.m[2][3] = 0.0f;
    mMatrix.SetTranslation(vPos);
    mIsValid = true;
    mInitMatrix = mMatrix;
    mType = type;

    SetRenderRadius(mType.mSize);
    SetRenderPos(vPos);
    Activate();
    StartBirdSound();
    mBirdSound.Update(&BirdParticleManager::sInstance->mBirdSound, &mMatrix.GetTranslation(), &mVelocity);
}

// FUNCTION: SWBF2 0x004495b0
void BirdParticle::RespawnScatterBird(const PblVector3& vPos, const BirdParticleType& type,
                                      const PblVector3& vAway)
{
    mState = BIRD_SCATTER;
    mTimeOffCamera = 0.0f;
    mTotalLifetime = 0.0f;
    mFlapTime = PblRandom::_Global.GetFloat() * (PblMath::fPi * 2.0f);

    PblQuaternion q = PblQuaternion(gMatrixIdentity);
    {
        PblVector3 initomega = PblVector3(0.0f, PblRandom::_Global.GetFloat() * (PblMath::fPi * 2.0f), 0.0f);
        float initAngle = Length(initomega);
        if (initAngle > EPSILON)
        {
            q *= PblQuaternion(initomega / initAngle, initAngle);
        }
    }
    {
        PblVector3 initomega = PblVector3((-0.25f - PblRandom::_Global.GetFloat() * 0.25f) * fHalfPi, 0.0f, 0.0f);
        float initAngle = Length(initomega);
        if (initAngle > EPSILON)
        {
            q *= PblQuaternion(initomega / initAngle, initAngle);
        }
    }
    mMatrix = PblMatrix(q, vPos);

    mRandomness = PblRandom::_Global.GetFloat();

    if (vAway.y >= -EPSILON &&
        DotProduct(mMatrix.GetZAxis(), vAway) < 0.0f)
    {
        PblVector3 vX = mMatrix.GetXAxis();
        PblVector3 vY = mMatrix.GetYAxis();
        PblVector3 vZ = mMatrix.GetZAxis();
        vX.x = -vX.x;
        vX.z = -vX.z;
        vY.x = -vY.x;
        vY.z = -vY.z;
        vZ.x = -vZ.x;
        vZ.z = -vZ.z;
        mMatrix.SetXAxis(vX);
        mMatrix.SetYAxis(vY);
        mMatrix.SetZAxis(vZ);
    }

    mInitMatrix = mMatrix;
    float fSpeed = mRandomness * 8.0f + 8.0f;
    mVelocity = fSpeed * mInitMatrix.GetZAxis();
    mIsValid = true;
    mType = type;

    SetRenderRadius(mType.mSize);
    SetRenderPos(vPos);
    AddToScene();
    StartBirdSound();
    mBirdSound.Update(&BirdParticleManager::sInstance->mBirdSound, &mMatrix.GetTranslation(), &mVelocity);
}

// FUNCTION: SWBF2 0x00449bf0
void BirdParticle::UpdateFlying(float fDeltaTime)
{
    PblVector3 vPos = mInitMatrix.GetTranslation();
    float fFlap = (mRandomness * 8.0f + 8.0f) * mFlapTime;

    float angle = PblMath::Sin(fFlap) * (fHalfPi * 0.5f);
    mWing1Matrix = gMatrixIdentity;
    PblQuaternion initRotation(mWing1Matrix);
    initRotation *= PblQuaternion(PblVector3(0.0f, 0.0f, 1.0f), angle);
    initRotation *= PblQuaternion(PblVector3(0.0f, 1.0f, 0.0f), fHalfPi * 0.5f);
    mWing1Matrix = PblMatrix(initRotation, PblVector3(0.0f, 0.0f, 0.0f));
    float fWing2 = PblMath::Sin(fFlap - 0.5654867f) * (fHalfPi * 0.7f);
    angle = fWing2;
    mWing2Matrix = gMatrixIdentity;
    initRotation = PblQuaternion(mWing2Matrix);
    initRotation *= PblQuaternion(PblVector3(0.0f, 0.0f, 1.0f), angle);
    initRotation *= PblQuaternion(PblVector3(0.0f, 1.0f, 0.0f), fHalfPi * 0.5f);
    mWing2Matrix = PblMatrix(initRotation, PblVector3(0.0f, 0.0f, 0.0f));

    float fBob = PblMath::Sin(fWing2) * 0.1f;
    const PblVector3& vUp = mInitMatrix.GetYAxis();
    vPos -= fBob * vUp;
    vPos += mVelocity * mTotalLifetime;
    mMatrix.SetTranslation(vPos);
    SetRenderPos(vPos);

    mTotalLifetime += fDeltaTime;
    mFlapTime += fDeltaTime;
    if (mFlapTime > PblMath::fTwoPi)
    {
        mFlapTime -= PblMath::fTwoPi;
    }

    mBirdSound.Update(&BirdParticleManager::sInstance->mBirdSound, &mMatrix.GetTranslation(), &mVelocity);
}

// FUNCTION: SWBF2 0x0044a0f0
bool BirdParticle::UpdateFlockBird(float fDeltaTime)
{
    UpdateFlying(fDeltaTime);
    if (mTimeOffCamera > 5.0f)
    {
        mIsValid = false;
        mBirdSound.Stop(true);
        Deactivate();
    }
    return true;
}

// FUNCTION: SWBF2 0x0044a130
bool BirdParticle::UpdateWalkingCircleBird(float fDeltaTime)
{
    mCircleTime += fDeltaTime;
    if (mCircleTime > mTotalCircleTime)
    {
        mCircleTime -= mTotalCircleTime;
    }
    float fAngle = mCircleTime / mTotalCircleTime * (PblMath::fPi * 2.0f);

    PblVector3 vPos = mCircleCenter;
    float fSin, fCos;
    PblMath::SinCos(&fSin, &fCos, fAngle);
    vPos.x += mCircleRadius * fSin;
    vPos.z += mCircleRadius * fCos;
    vPos.y = RedTerrain::GetHoleHeight(vPos) + mType.mSize * 0.125f;

    PblQuaternion q = PblQuaternion(gMatrixIdentity);
    {
        PblVector3 initomega = PblVector3(0.0f, fAngle + fHalfPi, 0.0f);
        float initAngle = Length(initomega);
        if (initAngle > EPSILON)
        {
            q *= PblQuaternion(initomega / initAngle, initAngle);
        }
    }
    {
        PblVector3 initomega = PblVector3(fHalfPi * -0.5f, 0.0f, 0.0f);
        float initAngle = Length(initomega);
        if (initAngle > EPSILON)
        {
            q *= PblQuaternion(initomega / initAngle, initAngle);
        }
    }
    {
        PblVector3 initomega = PblVector3(0.0f, 0.0f, PblMath::Sin(mFlapTime * 10.0f) * 0.5f);
        float initAngle = Length(initomega);
        if (initAngle > EPSILON)
        {
            q *= PblQuaternion(initomega / initAngle, initAngle);
        }
    }
    mMatrix = PblMatrix(q, vPos);
    SetRenderPos(vPos);
    return true;
}

// FUNCTION: SWBF2 0x0044a580
bool BirdParticle::UpdateHoppingCircleBird()
{
    BirdParticle* pBird = (BirdParticle*)this;
    float fRise = PblMath::Sin((pBird->mFlapTime + PblMath::fPi * 0.5f) * 10.0f);
    if (fRise > 0.0f)
    {
        PblVector3 vPosition = pBird->mMatrix.GetTranslation();
        vPosition.y += fRise * 0.3f * pBird->mType.mSize;
        pBird->mMatrix.SetTranslation(vPosition);
        pBird->SetRenderPos(vPosition);
    }
    return true;
}

// FUNCTION: SWBF2 0x0044a620
bool BirdParticle::UpdateStandingBird(float fDeltaTime)
{
    if (mState == BIRD_HOP_CIRCLE || mState == BIRD_WALK_CIRCLE)
    {
        UpdateWalkingCircleBird(fDeltaTime);
        if (mState == BIRD_HOP_CIRCLE)
        {
            UpdateHoppingCircleBird();
        }

        PblVector3 vPos = mMatrix.GetTranslation();
        GameObject* pObject;
        if (CollisionManager::GetObjectsInRange(vPos, 20.0f, &pObject, 1, 0, 10) > 0)
        {
            const PblVector3& vObjectPos = pObject->GetMatrix().GetTranslation();
            PblVector3 vAway(vPos.x - vObjectPos.x, vPos.y - vObjectPos.y, vPos.z - vObjectPos.z);
            vAway.y = 0.0f;
            PblVector3 vDir;
            D3DXVec3Normalize((D3DXVECTOR3*)&vDir, (const D3DXVECTOR3*)&vAway);
            mState = BIRD_TAKEOFF;
            mTotalLifetime = 0.0f;
            mVelocity = PblVector3(vDir.x * 9.0f, vDir.y * 9.0f, vDir.z * 9.0f);
        }
    }
    else if (mState == BIRD_TAKEOFF)
    {
        PblVector3 vDir;
        D3DXVec3Normalize((D3DXVECTOR3*)&vDir, (const D3DXVECTOR3*)&mVelocity);
        mMatrix[2] = vDir;
        mMatrix.m[2][3] = 0.0f;
        mMatrix.m[1][0] = 0.0f;
        mMatrix.m[1][1] = 1.0f;
        mMatrix.m[1][2] = 0.0f;
        mMatrix.m[1][3] = 0.0f;
        PblVector3 vX = Normalize(CrossProduct(mMatrix.GetYAxis(), mMatrix.GetZAxis()));
        mMatrix[0] = vX;
        mMatrix.m[0][3] = 0.0f;

        PblQuaternion q = PblQuaternion(mMatrix);
        {
            PblVector3 initomega = PblVector3(fHalfPi * -0.5f, 0.0f, 0.0f);
            float initAngle = Length(initomega);
            if (initAngle > EPSILON)
            {
                q *= PblQuaternion(initomega / initAngle, initAngle);
            }
        }
        {
            PblVector3 initomega = PblVector3(0.0f, 0.0f, PblMath::Sin(mFlapTime * 20.0f) * 0.5f);
            float initAngle = Length(initomega);
            if (initAngle > EPSILON)
            {
                q *= PblQuaternion(initomega / initAngle, initAngle);
            }
        }
        {
            PblVector3 initomega = PblVector3(0.0f, PblMath::Sin(mFlapTime * 5.0f) * 0.3f, 0.0f);
            float initAngle = Length(initomega);
            if (initAngle > EPSILON)
            {
                q *= PblQuaternion(initomega / initAngle, initAngle);
            }
        }

        const PblVector3& vOld = mMatrix.GetTranslation();
        PblVector3 vPos(vOld.x + mVelocity.x * fDeltaTime, vOld.y + mVelocity.y * fDeltaTime,
                        vOld.z + mVelocity.z * fDeltaTime);
        vPos.y = RedTerrain::GetHoleHeight(vPos) + mType.mSize * 0.125f;
        mMatrix = PblMatrix(q, vPos);
        SetRenderPos(vPos);

        if (mTotalLifetime > 1.5f)
        {
            PblVector3 vAway;
            D3DXVec3Normalize((D3DXVECTOR3*)&vAway, (const D3DXVECTOR3*)&mVelocity);
            RespawnScatterBird(vPos, mType, vAway);
            return true;
        }
    }

    mTotalLifetime += fDeltaTime;
    mFlapTime += fDeltaTime;
    if (mFlapTime > PblMath::fTwoPi)
    {
        mFlapTime -= PblMath::fTwoPi;
    }
    mBirdSound.Update(&BirdParticleManager::sInstance->mBirdSound, &mMatrix.GetTranslation(), &mVelocity);
    if (mTimeOffCamera > 10.0f)
    {
        mIsValid = false;
        mBirdSound.Stop(true);
        Deactivate();
    }
    return true;
}

// FUNCTION: SWBF2 0x0044ad40
bool BirdParticle::UpdateScatterBird(float fDeltaTime)
{
    UpdateFlying(fDeltaTime);
    if (mTotalLifetime > 5.0f)
    {
        mIsValid = false;
        mBirdSound.Stop(true);
        Deactivate();
    }
    return true;
}

// FUNCTION: SWBF2 0x0044ad80
bool BirdParticle::Update(float fDeltaTime)
{
    if (!mIsValid)
    {
        return true;
    }
    mTimeOffCamera += fDeltaTime;
    switch (mState)
    {
    case BIRD_SCATTER:
        return UpdateScatterBird(fDeltaTime);
    case BIRD_FLOCK:
        return UpdateFlockBird(fDeltaTime);
    case BIRD_HOP_CIRCLE:
    case BIRD_WALK_CIRCLE:
    case BIRD_TAKEOFF:
        return UpdateStandingBird(fDeltaTime);
    default:
        mIsValid = false;
        mBirdSound.Stop(true);
        Deactivate();
        return true;
    }
}

// GLOBAL: SWBF2 0x007e5ef8
float BirdParticleManager::sFlockMinHeight = -1025.0f;

// FUNCTION: SWBF2 0x0044adf0
BirdParticleManager::BirdParticleManager()
{
    mNextFreeBird = mBirdParticles;
    memset(mNumScatterBirds, 0, sizeof(mNumScatterBirds));
    mScatterTimer = 0.0f;
    mFlockTimer = 0.0f;
    mStandTimer = 0.0f;
}

// SYNTHETIC: SWBF2 0x0044ae90
// BirdParticleManager::`scalar deleting destructor'

// FUNCTION: SWBF2 0x0044aec0
BirdParticleManager::~BirdParticleManager()
{
    DeactivateThread();
}

// FUNCTION: SWBF2 0x0044af30
void BirdParticleManager::SetNumBirdTypes(int iNum)
{
    if (iNum > 3)
    {
        iNum = 3;
    }
    else if (iNum < 0)
    {
        iNum = 0;
    }
    BirdParticleManager::sNumBirdTypes = iNum;
}

// FUNCTION: SWBF2 0x0044af50
void BirdParticleManager::SetBirdType(int iIndex, float fSize, const char* pTexture)
{
    if (iIndex >= 0 && iIndex < BirdParticleManager::sNumBirdTypes)
    {
        BirdParticleManager::sBirdTypes[iIndex].mSize = fSize;
        BirdParticleManager::sBirdTypes[iIndex].mTexture = PblHash(pTexture);
    }
}

// FUNCTION: SWBF2 0x0044af90
bool BirdParticleManager::Create()
{
    if (sInstance)
    {
        return false;
    }
    sInstance = new BirdParticleManager;
    sInstance->ActivateThread(0, PER_FRAME);
    memset(sBirdTypes, 0, sizeof(sBirdTypes));
    sNumBirdTypes = 0;
    sFlockMinHeight = -1025.0f;
    return true;
}

// FUNCTION: SWBF2 0x0044b050
bool BirdParticleManager::Destroy()
{
    if (sInstance)
    {
        PlatformCleanup();
        delete sInstance;
        sInstance = 0;
        return true;
    }
    return false;
}

// FUNCTION: SWBF2 0x0044b080
void BirdParticleManager::SetPotentialScatterPoint(const PblVector3& vPosition, unsigned char ucNumBirds)
{
    if (!sInstance)
    {
        return;
    }

    int iClosestCameraToNew = -1;
    int iFarthestPoint = 0;
    float fFarthestDistance = 0.0f;
    int iClosestCameraToPoint = -1;

    CameraManager* pCameraManager = CameraManager::sInstance;
    const int iNumCameras = pCameraManager->m_iNumCam;
    PblMatrix cameraMatrix[1];
    for (int i = 0; i < iNumCameras; ++i)
    {
        cameraMatrix[i] = pCameraManager->mRedCamera[i]->_Matrix;
    }

    for (int i = 0; i < 5; ++i)
    {
        const PblVector3& rPoint = sInstance->mPotentialScatterPoints[i];
        if (sInstance->mNumScatterBirds[i])
        {
            const float dx = vPosition.x - rPoint.x;
            const float dy = vPosition.y - rPoint.y;
            const float dz = vPosition.z - rPoint.z;
            if (fabsf(dx * dx + dy * dy + dz * dz) < 0.001f)
            {
                return;
            }
        }

        float fPointDistance = FLT_MAX;
        float fNewDistance = FLT_MAX;
        for (int j = 0; j < iNumCameras; ++j)
        {
            const PblMatrix& rCamera = cameraMatrix[j];
            const float px = rPoint.x - rCamera.m[3][0];
            const float py = rPoint.y - rCamera.m[3][1];
            const float pz = rPoint.z - rCamera.m[3][2];
            const float fDistance = px * px + py * py + pz * pz;
            if (fDistance < fPointDistance)
            {
                fPointDistance = fDistance;
                iClosestCameraToPoint = j;
            }
            const float nx = vPosition.x - rCamera.m[3][0];
            const float ny = vPosition.y - rCamera.m[3][1];
            const float nz = vPosition.z - rCamera.m[3][2];
            const float fDistanceNew = nx * nx + ny * ny + nz * nz;
            if (fDistanceNew < fNewDistance)
            {
                fNewDistance = fDistanceNew;
                iClosestCameraToNew = j;
            }
        }
        if (fPointDistance > fFarthestDistance)
        {
            fFarthestDistance = fPointDistance;
            iFarthestPoint = i;
        }
    }

    const PblMatrix& newCamera = cameraMatrix[iClosestCameraToNew];
    PblVector3 vNew(vPosition.x - newCamera.m[3][0],
                    vPosition.y - newCamera.m[3][1],
                    vPosition.z - newCamera.m[3][2]);
    const float fNewDistance = vNew.x * vNew.x + vNew.y * vNew.y + vNew.z * vNew.z;
    PblVector3 vNewDir;
    D3DXVec3Normalize(reinterpret_cast<D3DXVECTOR3*>(&vNewDir), reinterpret_cast<const D3DXVECTOR3*>(&vNew));

    const PblMatrix& pointCamera = cameraMatrix[iClosestCameraToPoint];
    PblVector3 vOld = sInstance->mPotentialScatterPoints[iFarthestPoint] - pointCamera.GetTranslation();
    PblVector3 vOldDir;
    D3DXVec3Normalize(reinterpret_cast<D3DXVECTOR3*>(&vOldDir), reinterpret_cast<const D3DXVECTOR3*>(&vOld));

    if (!sInstance->mNumScatterBirds[iFarthestPoint] ||
        pointCamera.m[2][0] * vOldDir.x + pointCamera.m[2][1] * vOldDir.y + pointCamera.m[2][2] * vOldDir.z > -0.2f ||
        (newCamera.m[2][0] * vNewDir.x + newCamera.m[2][1] * vNewDir.y + newCamera.m[2][2] * vNewDir.z <= -0.2f &&
         vOld.x * vOld.x + vOld.y * vOld.y + vOld.z * vOld.z > fNewDistance))
    {
        sInstance->mPotentialScatterPoints[iFarthestPoint] = vPosition;
        sInstance->mNumScatterBirds[iFarthestPoint] = ucNumBirds;
    }
}

// FUNCTION: SWBF2 0x0044b430
BirdParticle* BirdParticleManager::GetFreeBird()
{
    BirdParticleManager* pManager = BirdParticleManager::sInstance;
    if (!pManager)
    {
        return NULL;
    }
    BirdParticle* pFree = pManager->mNextFreeBird;
    if (pFree)
    {
        for (int i = 0; i < 15; ++i)
        {
            BirdParticle* pBird = &pManager->mBirdParticles[i];
            if (!pBird->mIsValid && pBird != pFree)
            {
                pManager->mNextFreeBird = pBird;
            }
        }
    }
    return pFree;
}

// FUNCTION: SWBF2 0x0044b470
bool BirdParticleManager::CreateClient()
{
    return true;
}

// FUNCTION: SWBF2 0x0044b480
void BirdParticleManager::ScatterRandomBirds(int iNum, const PblVector3& vPos)
{
    if (!BirdParticleManager::sInstance)
    {
        return;
    }
    bool bScattered = false;
    unsigned int uRandom = 0;
    for (int i = 0; i < iNum; ++i)
    {
        BirdParticle* pBird = GetFreeBird();
        if (!pBird)
        {
            continue;
        }
        uRandom = PblRandom::GetGlobalInt();
        union { unsigned int u; float f; } randomFloat;
        randomFloat.u = (uRandom >> 9) | 0x3f800000u;
        const float fRandom = randomFloat.f - 1.0f;
        if (BirdParticleManager::sNumBirdTypes)
        {
            int iType = 1;
            float fStep = 1.0f / (float)BirdParticleManager::sNumBirdTypes;
            float fThreshold = 0.0f;
            while (iType <= BirdParticleManager::sNumBirdTypes)
            {
                fThreshold += fStep;
                if (fThreshold >= fRandom)
                {
                    pBird->RespawnScatterBird(vPos, BirdParticleManager::sBirdTypes[iType - 1],
                                              PblVector3(0.0f, -1.0f, 0.0f));
                    break;
                }
                ++iType;
            }
        }
        bScattered = true;
    }
    if (bScattered)
    {
        BirdParticleManager::sInstance->mScatterSound.Play(&vPos, NULL, true, true);
    }
    BirdParticleManager::sInstance->mScatterTimer = 0.0f;
}

// FUNCTION: SWBF2 0x0044b5b0
void BirdParticleManager::FlockRandomBirds(const PblVector3& vPosition, const PblVector3& vVelocity)
{
    if (!sInstance)
    {
        return;
    }
    const float fChoice = PblRandom::_Global.GetFloat();
    if (!sNumBirdTypes)
    {
        return;
    }
    const float fStep = 1.0f / (float)sNumBirdTypes;
    float fAccumulated = 0.0f;
    int iType = -1;
    for (int i = 1; i <= sNumBirdTypes; ++i)
    {
        fAccumulated += fStep;
        if (fAccumulated >= fChoice)
        {
            iType = i - 1;
            break;
        }
    }
    if (iType == -1)
    {
        return;
    }

    const float fSize = sBirdTypes[iType].mSize;
    const PblVector3 vUp(0.0f, 1.0f, 0.0f);
    PblVector3 vForward = Normalize(vVelocity) * 1.5f;
    PblVector3 vSide = Normalize(CrossProduct(vUp, vForward)) * 0.37f;

    BirdParticle* pBird = GetFreeBird();
    if (pBird)
    {
        pBird->RespawnFlockBird(vPosition, vVelocity, sBirdTypes[iType]);
    }
    pBird = GetFreeBird();
    if (pBird)
    {
        const PblVector3 vBirdPosition = vPosition + (vSide - vForward) * (fSize * 2.0f);
        pBird->RespawnFlockBird(vBirdPosition, vVelocity, sBirdTypes[iType]);
    }
    pBird = GetFreeBird();
    if (pBird)
    {
        const PblVector3 vBirdPosition = vPosition + (-vSide - vForward) * (fSize * 2.0f);
        pBird->RespawnFlockBird(vBirdPosition, vVelocity, sBirdTypes[iType]);
    }
    pBird = GetFreeBird();
    if (pBird)
    {
        const PblVector3 vBirdPosition = vPosition + (vSide - vForward) * (fSize * 4.0f);
        pBird->RespawnFlockBird(vBirdPosition, vVelocity, sBirdTypes[iType]);
    }
    pBird = GetFreeBird();
    if (pBird)
    {
        const PblVector3 vBirdPosition = vPosition + (-vSide - vForward) * (fSize * 4.0f);
        pBird->RespawnFlockBird(vBirdPosition, vVelocity, sBirdTypes[iType]);
    }
    sInstance->mFlockTimer = 0.0f;
}

// FUNCTION: SWBF2 0x0044b980
void BirdParticleManager::SpawnGroundBirds()
{
    CameraManager* pCameraManager = CameraManager::Instance();
    const unsigned int uNumCameras = pCameraManager->GetNumCameras();
    int iCam = PblRandom::GetGlobalInt() % uNumCameras;
    PblMatrix mCamera = pCameraManager->GetCamera(iCam)->GetMatrix();
    const D3DXVECTOR3& vDir = *(const D3DXVECTOR3*)&mCamera.m[2][0];
    const PblVector3& vCamPos = *(const PblVector3*)&mCamera.m[3][0];
    const D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);

    PblVector3 vPos;
    PblVector3 vSide;
    PblVector3 vForward;
    {
        D3DXVECTOR3 vTemp;
        D3DXVec3Normalize((D3DXVECTOR3*)&vSide, D3DXVec3Cross(&vTemp, &vUp, &vDir));
        D3DXVec3Normalize((D3DXVECTOR3*)&vForward, D3DXVec3Cross(&vTemp, &vUp, (const D3DXVECTOR3*)&vSide));
    }

    const PblVector3 vCenter = vCamPos + vForward * 45.0f;

    int iNumGround = 0;
    for (int i = 0; i < 15; ++i)
    {
        const BirdParticle& rBird = sInstance->mBirdParticles[i];
        if (rBird.mIsValid && (rBird.mState == BIRD_HOP_CIRCLE || rBird.mState == BIRD_WALK_CIRCLE))
        {
            ++iNumGround;
        }
    }

    const int iNumToSpawn = 3 - iNumGround;
    for (int i = 0; i < iNumToSpawn; ++i)
    {
        float fForward = PblRandom::_Global.GetFloat();
        float fSide = PblRandom::_Global.GetFloat();
        vPos = vCenter + (vSide * fSide + vForward * fForward) * 8.0f;
        vPos.y = RedTerrain::GetHoleHeight(vPos);

        CollisionObject* pHit = 0;
        const PblVector3 vDown(0.0f, -1.0f, 0.0f);
        CollisionManager::RayHit(vPos, vDown, 1024.0f, pHit, 0, 0, 0, 0x190, false);
        if (pHit && pHit->GetCollisionType() == COLL_TERRAIN)
        {
            float fChoice = PblRandom::_Global.GetFloat();
            BirdParticle* pBird = GetFreeBird();
            if (sNumBirdTypes)
            {
                const float fStep = 1.0f / (float)sNumBirdTypes;
                float fAccumulated = 0.0f;
                for (int iType = 1; iType <= sNumBirdTypes; ++iType)
                {
                    fAccumulated += fStep;
                    if (fAccumulated >= fChoice)
                    {
                        const BirdParticleType& type = sBirdTypes[iType - 1];
                        vPos.y += type.mSize * 0.125f;
                        pBird->RespawnGroundBird(vPos, type);
                        break;
                    }
                }
            }
        }
    }
    sInstance->mStandTimer = 0.0f;
}

// FUNCTION: SWBF2 0x0044bdb0
bool BirdParticleManager::Update(float fDeltaTime)
{
    mScatterTimer += fDeltaTime;
    mFlockTimer += fDeltaTime;
    mStandTimer += fDeltaTime;

    if (sNumBirdTypes)
    {
        if (mScatterTimer > 20.0f)
        {
            int iPoint = (int)(PblRandom::_Global.GetFloat() * 5.0f);
            if (iPoint >= 5)
            {
                iPoint = 4;
            }
            ScatterRandomBirds(mNumScatterBirds[iPoint], mPotentialScatterPoints[iPoint]);
        }

        if (mFlockTimer > 25.0f)
        {
            CameraManager* pCameraManager = CameraManager::Instance();
            const unsigned int uNumCameras = pCameraManager->GetNumCameras();
            int iCam = PblRandom::GetGlobalInt() % uNumCameras;
            PblMatrix mCamera = pCameraManager->GetCamera(iCam)->GetMatrix();
            const D3DXVECTOR3& vDir = *(const D3DXVECTOR3*)&mCamera.m[2][0];
            const PblVector3& vCamPos = *(const PblVector3*)&mCamera.m[3][0];
            const D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);

            PblVector3 vSide;
            {
                D3DXVECTOR3 vTemp;
                D3DXVec3Normalize((D3DXVECTOR3*)&vSide, D3DXVec3Cross(&vTemp, &vUp, &vDir));
            }
            PblVector3 vForward;
            {
                D3DXVECTOR3 vTemp;
                D3DXVec3Normalize((D3DXVECTOR3*)&vForward, D3DXVec3Cross(&vTemp, &vUp, (const D3DXVECTOR3*)&vSide));
            }

            if (PblRandom::_Global.GetFloat() <= 0.5f)
            {
                vSide = PblVector3(-vSide.x, -vSide.y, -vSide.z);
            }

            PblVector3 vPos = vCamPos + (vForward * (PblRandom::_Global.GetFloat() + 1.5f) - vSide * 1.1f) * 20.0f;

            if (sFlockMinHeight < -1024.0f)
            {
                vPos.y = RedTerrain::GetHoleHeight(vPos);
                float fRaise = PblRandom::GetGlobalFloat() * 5.0f + 10.0f;
                vPos.x += vUp.x * fRaise;
                vPos.y += vUp.y * fRaise;
                vPos.z += vUp.z * fRaise;
            }
            else
            {
                vPos.y = PblRandom::GetGlobalFloat() * 5.0f + sFlockMinHeight;
            }

            while (CollisionManager::RayTest(vPos, vSide, 200.0f, 0, 0, 0x90))
            {
                vPos.x += vUp.x * 5.0f;
                vPos.y += vUp.y * 5.0f;
                vPos.z += vUp.z * 5.0f;
            }

            FlockRandomBirds(vPos, vSide * 5.0f);
        }

        if (mStandTimer > 35.0f)
        {
            SpawnGroundBirds();
        }

        for (int i = 0; i < 15; ++i)
        {
            mBirdParticles[i].Update(fDeltaTime);
            if (!mBirdParticles[i].mIsValid)
            {
                mNextFreeBird = &mBirdParticles[i];
            }
        }
    }
    return true;
}
