#pragma once
#include "PebbleFL/Common/PblHermite.h"

#include <stddef.h>

#include "RedEngineFL/Application/Renderable.h"
#include "Zephyr/Source/ZephyrPoseDyn.h"
#include "Zephyr/Source/ZephyrPoseStatic.h"
#include "Zephyr/Source/ZephyrSkeleton.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblQuaternion.h"
#include "PebbleFL/Common/PblVector.h"

struct CamPathInfo
{
    PblVector3 Vec[6];
    PblVector3 Slope[6];
    PblVector3 Ore[6];
    PblVector3 RSlope[6];
    float Time[6];
    unsigned int numPoints;
};

class Aimer;
class Controllable;
class RedModel;
class Weapon;
class ZephyrAnim;
class ZephyrSkeletonShared;

enum WEAPONCLASS
{
    RIFLE = 0,
    BAZOOKA = 1,
    TOOL = 2,
    GRENADE = 3,
    WEAPONCLASS_NONE = 4
};

enum SOLDIERANIMATION
{
    IDLE = 0,
    RUN = 1,
    SHOOT1 = 2,
    SHOOT2 = 3,
    CHARGE = 4,
    RELOAD = 5,
    REPAIR = 6,
    JUMP = 7,
    FLAIL = 8,
    HANDSDOWN = 9,
    JUMP_LAND = 10,
    NUM_SOLDIERANIMATIONS = 11,
    DROIDEKA_IDLE = 0,
    DROIDEKA_WALK = 1,
    DROIDEKA_SHOOT = 2,
    DROIDEKA_RELOAD = 3,
    NUM_DROIDEKAANIMATIONS = 4,
    TOTAL_NUM_ANIMATIONS = 15
};

enum eViewObjectType
{
    TYPE_SOLDIER = 0,
    TYPE_COCKPIT = 1,
    TYPE_DROIDEKA = 2,
    TYPE_HOVER = 3,
    TYPE_NONE = 4
};

class FirstPersonCameraManager
{
public:
    FirstPersonCameraManager() { mCurrCamPt = -1; }

    void ViewBob(float fScale);

    void ReturnToCenter();

    void Update(PblMatrix& rMatrix);

    float RollLeft();
    float RollRight();
    float Land();
    float ProneToStand();
    float CrouchToStand();

    float SetCameraPath(PblVector3* pPos, PblVector3* pPosSlope, PblVector3* pRot,
                        PblVector3* pRotSlope, float* pTime, unsigned int uPoints);
    void SetBobbing(bool bBobbing) { mIsBobbing = bBobbing; }

    PblHermiteVector3 mHermPos[6];
    PblHermiteVector3 mHermRot[6];
    float mCamTime[6];
    float mCurrCamTime;
    int mCurrCamPt;
    PblVector3 mCameraPos;
    PblVector3 mCurRotVec;
    int mNumPoints;
    bool mIsBobbing;
    bool mIsReturning;
    PblVector3 mLinVec;
    bool mIsGoingProne;
};
 
 
 

 
 
 
 
 
 
 
 
 
 
 

namespace FirstPerson
{

    void ClearFiringState(unsigned int uPlayer);
    void RollLeft(unsigned int uPlayer);
    void RollRight(unsigned int uPlayer);
    void ProneToStand(unsigned int uPlayer);
    void CrouchToStand(unsigned int uPlayer);
    void Land(unsigned int uPlayer);

    bool IsActive(unsigned int uPlayer);

    void PreInit();

    void ClearBanks();

    void Deinit();

    bool AddBank(const char* pszName);

    const ZephyrAnim* FindAnimation(const char* pszName);

    extern int s_iAnimBankCount;

    extern char sModelName[64][256];

    void Init();

    void InitPlatform();

    void PostLoad();

    void LoadLVLFile(char const* pName);

    int AddGeometry(const char* pszName);

    void Activate(unsigned int uPlayer, bool bActive);

    void Update(unsigned int uPlayer, eViewObjectType eType, int iArg,
                Controllable* pOwner, Aimer* pAimer);

    void SetSoldierState(unsigned int uPlayer, int iState);

    void UpdateCamera(unsigned int uPlayer, PblMatrix& rMatrix);

    void UpdateAimer(unsigned int uPlayer, const PblMatrix& rMatrix);

    const RedModel* LoadDynamicModel(const char* pszName, bool bArg);
}

namespace FirstPerson
{
extern int sModelCount;
}
