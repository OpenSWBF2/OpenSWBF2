#pragma once

#include <stddef.h>

#include "ChaseCamera.h"
#include "DeathCamera.h"
#include "Thread.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblQuaternion.h"
#include "PebbleFL/Common/PblVector.h"

class Character;
class ChaseCamera;
class DeathCamera;
class FreeCamera;
class GameCamera;
class MapCamera;
class RedCamera;
class Trackable;
namespace Snd
{
class Listener;
}

// VTABLE: SWBF2 0x007970c8
class CameraManager : public Thread
{
public:
    CameraManager();
    virtual ~CameraManager();
    virtual bool Update(float fDeltaTime);

    static void Create(int iNumCameras);
    static void CreateClient();
    void SetNumCameras(unsigned int uiNumCameras);
    void RefreshAspect();
    int GetCameraIdForView(unsigned int uiView) const;

    void EnableScene(bool bEnable);
    void SetChaseCameraRandomShot(int iCam);
    void SetChaseCameraShot(int iCam, int iShot);
    void QuickSetupCameras();
    void Chase(Trackable* pTrackable, int iCam);
    void TrackDeath(Trackable* pTrackable, int iCam, Character* pKiller);
    void ChaseMode(int iCam);
    void FreeLook(int iCam);
    void MapMode(int iCam);
    bool IsChaseMode(int iCam) const;
    bool IsFreeLook(int iCam) const;
    void ApplyShake(float fAmount, float fLength);
    void ClearShake();
    bool IsInsideAnyCamera(const PblVector3& vPos, float fRadius, float fDistance);
    float UpdateTrackDistance(int iCam, float fDistance, float fDeltaTime);

    // FUNCTION: SWBF2 0x004292f0
    static CameraManager* Instance() { return sInstance; }
    unsigned int GetNumCameras() const { return m_iNumCam; }
    RedCamera* GetCamera(int iCam) { return mRedCamera[iCam]; }

    PblVector3 GetCameraVelocity(int iCam) const;

    PblMatrix GetChaseCameraTransform(int iCam);

    Trackable* GetChaseCameraTarget(int iCam);

    Trackable* GetDeathCameraTarget(int iCam);
    Character* GetDeathCameraKiller(int iCam);

    void SetVisibleIconMask(int iView, unsigned int uMask) { m_uiVisibleIconMask[iView] = uMask; }

    void AddCameraShot(const PblQuaternion& qRot, const PblVector3& vPos)
    {
        if (mNumShots < 16)
        {
            mShotRotation[mNumShots] = qRot;
            mShotPosition[mNumShots] = vPos;
            ++mNumShots;
        }
    }
    MapCamera* GetMapCamera(int iCam) { return mMapCamera[iCam]; }
    void SetChaseCameraPosition(const PblVector3& vPos, int iCam);
    void SetChaseCameraRotation(const PblQuaternion& qRot, int iCam);

    static CameraManager* sInstance;

    float mTrackDistance[1];
    unsigned int m_iNumCam;
    GameCamera* mCurGameCamera[1];
    RedCamera* mRedCamera[1];
    ChaseCamera* mChaseCamera[1];
    FreeCamera* mFreeCamera[1];
    DeathCamera* mDeathCamera[1];
    MapCamera* mMapCamera[1];
    Snd::Listener* mListener[1];
    PblVector3 mListenerPos[1];
    unsigned int m_hView[1];
    unsigned int m_uiVisibleIconMask[1];
    unsigned int mUpdatedCameraId;
    unsigned int mNumShots;
    PblQuaternion mShotRotation[16];
    PblVector3 mShotPosition[16];
};

// FUNCTION: SWBF2 0x0044e330
inline void CameraManager::SetChaseCameraRotation(const PblQuaternion& qRot, int iCam)
{
    mChaseCamera[iCam]->SetRotation(qRot);
}

// FUNCTION: SWBF2 0x004a9db0
inline Trackable* CameraManager::GetChaseCameraTarget(int iCam)
{
    if (mCurGameCamera[iCam] == mDeathCamera[iCam])
    {
        return mDeathCamera[iCam]->mOwner;
    }
    return mChaseCamera[iCam]->mOwner;
}

// FUNCTION: SWBF2 0x0051ecb0
inline PblMatrix CameraManager::GetChaseCameraTransform(int iCam)
{
    return mChaseCamera[iCam]->GetMatrix();
}

 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x007970e0
class CameraInterpolator : public Thread
{
public:

    // SYNTHETIC: SWBF2 0x0044e4e0
    // CameraInterpolator::CameraInterpolator

    // SYNTHETIC: SWBF2 0x0044e4f0
    // CameraInterpolator::~CameraInterpolator

    virtual bool Update(float fDeltaTime);

    void CreateClient();
    void SetNext(GameCamera* pGameCamera, RedCamera* pCamera);

    int count;
    PblMatrix mat[2];
    RedCamera* mCamera;
    GameCamera* mGameCamera;
};
 
 
 
 
 
