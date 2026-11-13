#include "VOHelper.h"
#include "AIUtil.h"
#include "Character.h"
#include "Controllable.h"
#include "EntityCarrier.h"
#include "EntitySoldier.h"
#include "GameLoop.h"
#include "GameObject.h"
#include "NetGame.h"
#include "PlayerController.h"
#include "Team.h"
#include "Trackable.h"
#include "UnitController.h"
#include "PebbleFL/Common/PblHashTable.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// GLOBAL: SWBF2 0x0078bce0
static const unsigned int VOTypeNames[48] =
{
    0x00000000, 0x2588f638 /* "AcquiredTarget" */, 0xe359b342 /* "FriendlyFire" */, 0x0cb276ac,
    0xee6df4fd /* "missileLocked" */, 0x2af268b3 /* "MissileIncoming" */, 0xaaccc29a /* "PathFollowWait" */, 0x9ce7b15e,
    0xc50f4599 /* "kill" */, 0xdeb06c63 /* "HeadshotKill" */, 0xd988d8fd /* "Grenade" */, 0xfdf0ecb5 /* "RebelsShootDeadBody" */,
    0x74bb501c /* "KillingSpree4" */, 0x78bb5668 /* "KillingSpree8" */, 0x5ad79083, 0x5ed796cf,
    0x0d430618 /* "RepairStart" */, 0xce19d989 /* "RepairEnd" */, 0xcfd5c320 /* "GivePowerup" */, 0xdd1db480,
    0xc0a42c15, 0xeaf65ff1, 0x49dfeb81 /* "SC_Follow" */, 0xaef2d88a /* "SC_FollowResponse" */,
    0x421c2029 /* "SC_StopFollow" */, 0x1b8a0392 /* "SC_StopFollowResponse" */, 0x37c344c8 /* "SC_VehicleWaitUp" */, 0x2ebba9e7 /* "SC_VehicleWaitUpResponse" */,
    0xfe5092df /* "SC_GetIn" */, 0x048b38ec /* "SC_GetInResponse" */, 0xd8a23783, 0x8bb82ce4 /* "SC_GetOut" */,
    0x027d1363 /* "SC_GetOutResponse" */, 0x5e739197 /* "NotShootingCriticalHit" */, 0xde654ab1, 0x729931de /* "NearbyFriendlySlaughter" */,
    0x3f4ec8e3 /* "NearbyEnemySlaughter" */, 0x15b693e4, 0xd0e42acc, 0x8435f5fe,
    0x31b64121 /* "SpottedVO" */, 0x006b4d8e /* "NeedMedicVO" */, 0x0ff77e53 /* "NeedRepairVO" */, 0x4460cdc4 /* "NeedAmmoVO" */,
    0x1d2f03ea /* "NeedPickupVO" */, 0x03a34694 /* "NeedBackupVO" */, 0x7208dd19 /* "AttackPositionVO" */, 0x335a8bad /* "DefendPositionVO" */,
};

namespace AI
{
namespace VOHelper
{
extern VOSound* sLoadingBatch;
}
}
namespace AI
{
namespace VOHelper
{
extern int sLoadingBatchIndex;
}
}

namespace AI
{
namespace VOHelper
{

// FUNCTION: SWBF2 0x00671640
void SetModifier(VOModifier& modifier, const char* pszWord, const char*)
{
    if (!_strnicmp(pszWord, "VOUnitNum", 9))
    {
        modifier.mTargetUnitNum = atoi(pszWord + 9);
        return;
    }

    switch ((unsigned int)PblHash(pszWord))
    {
    case 0xA4213C0A: /* "Above" */ modifier.mAboveBelowCover = VO_ABC_Above; break;
    case 0x9253C0F2: /* "Below" */ modifier.mAboveBelowCover = VO_ABC_Below; break;
    case 0xBACD0480: /* "Cover" */ modifier.mAboveBelowCover = VO_ABC_Cover; break;
    case 0x78E32DE5: /* "Right" */ modifier.mRightLeftBehind = VO_RLB_Right; break;
    case 0x124AEC70: /* "Left" */ modifier.mRightLeftBehind = VO_RLB_Left; break;
    case 0xDCED8A09: /* "Behind" */ modifier.mRightLeftBehind = VO_RLB_Behind; break;
    case 0xE5DAB718: /* "ReallyNear" */ modifier.mDistance = VO_D_ReallyNear; break;
    case 0x54F3570F: /* "Near" */ modifier.mDistance = VO_D_Near; break;
    case 0x1CDAD1EB: /* "ReallyFar" */ modifier.mDistance = VO_D_ReallyFar; break;
    case 0xEAFD2812: /* "TargetHealthBelow25" */ modifier.mTargetHealth = VO_TH_Below25; break;
    case 0x1EED85CA: /* "PlayerHealthBelow25" */ modifier.mPlayerHealth = VO_PH_Below25; break;
    case 0xEEDDAE45: /* "IsFlyer" */ modifier.mTargetType = VO_TT_IsFlyer; break;
    case 0x5D847EDD: /* "IsHover" */ modifier.mTargetType = VO_TT_IsHover; break;
    case 0x91668BB7: /* "IsWalker" */ modifier.mTargetType = VO_TT_IsWalker; break;
    case 0x710F3467: /* "IsTurret" */ modifier.mTargetType = VO_TT_IsTurret; break;
    case 0xB0C0E926: /* "IsSniper" */ modifier.mTargetType = VO_TT_IsSniper; break;
    case 0xB79BD865: /* "IsRocket" */ modifier.mTargetType = VO_TT_IsRocket; break;
    case 0x6B468289: /* "IsHero" */ modifier.mTargetType = VO_TT_IsHero; break;
    case 0x76EB7F24: /* "InFlyer" */ modifier.mMeInfo = VO_ME_InFlyer; break;
    case 0x5968B570: /* "InHover" */ modifier.mMeInfo = VO_ME_InHover; break;
    case 0x1EE5DFFC: /* "InWalker" */ modifier.mMeInfo = VO_ME_InWalker; break;
    case 0x1C993CF8: /* "InTurret" */ modifier.mMeInfo = VO_ME_InTurret; break;
    case 0xE16E9E73: /* "IsZoomed" */ modifier.mMeInfo = VO_ME_IsZoomed; break;
    case 0x3B183215: /* "IsProne" */ modifier.mMeInfo = VO_ME_IsProne; break;
    case 0xABC74D1C: /* "AdrenalineLow" */ modifier.mAdrenaline = VO_ADR_Low; break;
    case 0x33C5D9AD: /* "AdrenalineMedium" */ modifier.mAdrenaline = VO_ADR_Medium; break;
    case 0x8929FEF6: /* "AdrenalineHigh" */ modifier.mAdrenaline = VO_ADR_High; break;
    }
}

// FUNCTION: SWBF2 0x00671a80
void GetModifiers(const GameObject* pTarget, const PblVector3& vPos, VOModifier& modifiers)
{
    const PblMatrix& mTarget = pTarget->GetMatrix();
    const PblVector3& vTarget = mTarget.GetTranslation();

    const float fHeight = vPos.y - vTarget.y;
    float fDist = LengthXZ(vTarget - vPos);
    fDist = Max(fDist, 0.1f);
    const float fSlope = fHeight / fDist;
    if (fHeight > 3.0f && fSlope > 0.25f)
    {
        modifiers.mAboveBelowCover = VO_ABC_Above;
    }
    else if (fHeight < -3.0f && fSlope < -0.25f)
    {
        modifiers.mAboveBelowCover = VO_ABC_Below;
    }

    const PblVector3 vFacing = AIUtil::GetSafeNormalXZ(pTarget->GetMatrix().GetZAxis());
    PblVector3 vDelta;
    vDelta.x = vPos.x - vTarget.x;
    vDelta.y = vPos.y - vTarget.y;
    vDelta.z = vPos.z - vTarget.z;
    const PblVector3 vDir = AIUtil::GetSafeNormalXZ(vDelta);
    const float fFront = vFacing.x * vDir.x + vDir.y * vFacing.y + vDir.z * vFacing.z;
    if (fFront < -0.7071f)
    {
        modifiers.mRightLeftBehind = VO_RLB_Behind;
    }
    else if (fFront < 0.7071f)
    {
        const PblVector3 vSide = AIUtil::GetSafeNormalXZ(pTarget->GetMatrix().GetXAxis());
        const float fRight = vSide.x * vDir.x + vSide.y * vDir.y + vSide.z * vDir.z;
        modifiers.mRightLeftBehind = fRight < 0.0f ? VO_RLB_Right : VO_RLB_Left;
    }

    if (fDist < 10.0f)
    {
        modifiers.mDistance = VO_D_ReallyNear;
    }
    else if (fDist < 30.0f)
    {
        modifiers.mDistance = VO_D_Near;
    }
    else if (fDist > 100.0f)
    {
        modifiers.mDistance = VO_D_ReallyFar;
    }
}

// FUNCTION: SWBF2 0x00671c80
VOModifier GetModifiers(const GameObject* pTarget, const GameObject* pOther)
{
    VOModifier modifiers = VOModifier();
    if (!pOther)
    {
        return modifiers;
    }

    GetModifiers(pTarget, pOther->GetMatrix().GetTranslation(), modifiers);

    if (pOther->mCurHealth / pOther->mMaxHealth < 0.25f)
    {
        modifiers.mTargetHealth = VO_TH_Below25;
    }
    if (pTarget->mCurHealth / pTarget->mMaxHealth < 0.25f)
    {
        modifiers.mPlayerHealth = VO_PH_Below25;
    }

    switch (AIUtil::GetEntityType(pOther->GetEntity()))
    {
    case AIUtil::ENTITY_FLYER:
        modifiers.mTargetType = VO_TT_IsFlyer;
        break;
    case AIUtil::ENTITY_TURRET:
    case AIUtil::ENTITY_BUILDING_ARMED:
    case AIUtil::ENTITY_REMOTE_TERMINAL:
        modifiers.mTargetType = VO_TT_IsTurret;
        break;
    case AIUtil::ENTITY_WALKER:
        modifiers.mTargetType = VO_TT_IsWalker;
        break;
    case AIUtil::ENTITY_HOVER:
        modifiers.mTargetType = VO_TT_IsHover;
        break;
    default:
        if (AIUtil::IsSniperGuy(pOther))
        {
            modifiers.mTargetType = VO_TT_IsSniper;
        }
        else if (AIUtil::IsRocketGuy(pOther))
        {
            modifiers.mTargetType = VO_TT_IsRocket;
        }
        else if (AIUtil::IsHeroGuy(pOther))
        {
            modifiers.mTargetType = VO_TT_IsHero;
        }
        break;
    }

    switch (AIUtil::GetEntityType(pTarget->GetEntity()))
    {
    case AIUtil::ENTITY_TURRET:
    case AIUtil::ENTITY_BUILDING_ARMED:
    case AIUtil::ENTITY_REMOTE_TERMINAL:
        modifiers.mMeInfo = VO_ME_InTurret;
        break;
    case AIUtil::ENTITY_FLYER:
        modifiers.mTargetType = VO_ME_InFlyer;
        break;
    case AIUtil::ENTITY_WALKER:
        modifiers.mTargetType = VO_ME_InWalker;
        break;
    case AIUtil::ENTITY_HOVER:
        modifiers.mTargetType = VO_ME_InHover;
        break;
    case AIUtil::ENTITY_SOLDIER:
    {
        EntitySoldier* pSoldier = (EntitySoldier*)(const_cast<Entity*>(pTarget->GetEntity()));
        if (pSoldier->GetTargetInfo()->mIsAiming)
        {
            modifiers.mMeInfo = VO_ME_IsZoomed;
        }
        else if (pSoldier->mState == SoldierState::PRONE)
        {
            modifiers.mMeInfo = VO_ME_IsProne;
        }
        break;
    }
    default:
        break;
    }

    if (pTarget->GetControllable() && pTarget->GetControllable()->mCharacter)
    {
        switch (pTarget->GetControllable()->GetCharacter()->mAdrenaline.GetCurrentLevel())
        {
        case ADRENALINE_LOW:
            modifiers.mAdrenaline = VO_ADR_Low;
            break;
        case ADRENALINE_MEDIUM:
            modifiers.mAdrenaline = VO_ADR_Medium;
            break;
        case ADRENALINE_HIGH:
            modifiers.mAdrenaline = VO_ADR_High;
            break;
        }
    }

    modifiers.mTargetUnitNum = pOther->GetGameObjectClass()->mVOUnitType;
    return modifiers;
}

// FUNCTION: SWBF2 0x00671f00
bool HasRequiredModifiers(const VOSound* pSound, const VOModifier& modifiers)
{
    const VOModifier& required = pSound->mModifiers;
    if (required.mAboveBelowCover && required.mAboveBelowCover != modifiers.mAboveBelowCover)
    {
        return false;
    }
    if (required.mRightLeftBehind && required.mRightLeftBehind != modifiers.mRightLeftBehind)
    {
        return false;
    }
    if (required.mDistance && required.mDistance != modifiers.mDistance)
    {
        return false;
    }
    if (required.mTargetType && required.mTargetType != modifiers.mTargetType)
    {
        return false;
    }
    if (required.mMeInfo && required.mMeInfo != modifiers.mMeInfo)
    {
        return false;
    }
    if (required.mTargetUnitNum && required.mTargetUnitNum != modifiers.mTargetUnitNum)
    {
        return false;
    }
    return true;
}

// FUNCTION: SWBF2 0x00671f90
int GetModifierScore(const VOSound* pSound, const VOModifier& modifiers)
{
    const VOModifier& required = pSound->mModifiers;
    int iScore = 0;
    if (required.mAboveBelowCover && required.mAboveBelowCover == modifiers.mAboveBelowCover)
    {
        ++iScore;
    }
    if (required.mRightLeftBehind && required.mRightLeftBehind == modifiers.mRightLeftBehind)
    {
        ++iScore;
    }
    if (required.mDistance && required.mDistance == modifiers.mDistance)
    {
        ++iScore;
    }
    if (required.mTargetType && required.mTargetType == modifiers.mTargetType)
    {
        ++iScore;
    }
    if (required.mMeInfo && required.mMeInfo == modifiers.mMeInfo)
    {
        ++iScore;
    }
    if (required.mTargetUnitNum && required.mTargetUnitNum == modifiers.mTargetUnitNum)
    {
        ++iScore;
    }
    return iScore;
}

// FUNCTION: SWBF2 0x00672030
int VOSoundCompareCallback(const void* pA, const void* pB)
{
    const VOSound* pSoundA = *(const VOSound* const*)pA;
    const VOSound* pSoundB = *(const VOSound* const*)pB;
    if (pSoundB->mScore > pSoundA->mScore)
    {
        return 1;
    }
    if (pSoundB->mScore < pSoundA->mScore)
    {
        return -1;
    }
    if (pB > pA)
    {
        return 1;
    }
    if (pB < pA)
    {
        return -1;
    }
    return 0;
}

}


namespace VOHelper
{

struct QueuedSound
{
    VOType mType;
    PblHandle<const GameObject> mSpeaker;
    PblHandle<const GameObject> mPlayer;
    PblHandle<const GameObject> mTarget;
    PblVector3 mPos;
    bool mUseTarget;
    float mTime;
    int mHandle;

    // SYNTHETIC: SWBF2 0x006715f0
    // AI::VOHelper::QueuedSound::~QueuedSound
};
extern QueuedSound mSndQueue[20];
extern int mNextQueueHandle;

int Queue(VOType eType, const GameObject* pSpeaker, const GameObject* pTarget,
          const GameObject* pOther, const PblVector3* pPos, float fDelay);

void GetModifiers(const GameObject* pTarget, const PblVector3& vPos, VOModifier& modifiers);

VOModifier GetModifiers(const GameObject* pTarget, const GameObject* pOther);

void _Play(VOType eType, const GameObject* pSpeaker, const GameObject* pTarget,
           const VOModifier& modifiers);

// FUNCTION: SWBF2 0x00672070
void _Play(VOType eType, const GameObject* pSpeaker, const GameObject* pTarget,
           const VOModifier& modifiers)
{
    if (!pSpeaker)
    {
        float fClosestSq = FLT_MAX;
        const Team* pTeam = Team::sTeams[pTarget->GetTeam()];
        for (int i = 0; i < pTeam->mMemberCount; ++i)
        {
            const Character* pCharacter = pTeam->GetMemberPtr(i);
            if (!pCharacter || !pCharacter->mUnit)
            {
                continue;
            }
            const GameObject* pObj = pCharacter->mUnit->GetGameObject();
            if (pObj == pTarget)
            {
                continue;
            }
            const float fRadius = pObj->GetGameObjectClass()->mVORadius;
            const PblVector3& vObj = pObj->GetPosition();
            const PblVector3& vTarget = pTarget->GetPosition();
            const float fX = vTarget.x - vObj.x;
            const float fY = vTarget.y - vObj.y;
            const float fZ = vTarget.z - vObj.z;
            const float fDistSq = fY * fY + fX * fX + fZ * fZ;
            if (fRadius * fRadius > fDistSq && fClosestSq > fDistSq)
            {
                pSpeaker = pObj;
                fClosestSq = fDistSq;
            }
        }
        if (!pSpeaker)
        {
            return;
        }
    }

    const float fRadius = pSpeaker->GetGameObjectClass()->mVORadius;
    const PblVector3& vTarget = pTarget->GetPosition();
    const PblVector3& vSpeaker = pSpeaker->GetPosition();
    const float fX = vSpeaker.x - vTarget.x;
    const float fY = vSpeaker.y - vTarget.y;
    const float fZ = vSpeaker.z - vTarget.z;
    if (fY * fY + fX * fX + fZ * fZ > fRadius * fRadius)
    {
        return;
    }

    if (pSpeaker->GetAffiliationToMe(pTarget) < 0 && AIUtil::GetRandomInt(0, 100) > 30 &&
        eType != VOT_RebelsShootDeadBody)
    {
        return;
    }

    if (!pSpeaker->GetControllable())
    {
        return;
    }
    const Controller* pCtrl = pSpeaker->GetControllable()->mCtrl;
    if (!PlayerControllerPtr(pCtrl))
    {
        const UnitController* pUnit = UnitControllerPtr(pCtrl);
        if (!pUnit)
        {
            return;
        }
        if (!pSpeaker->IsRtti(EntityFlyer::rttiHashEntityFlyer) && pUnit->mLodState < 3)
        {
            return;
        }
    }

    const GameObject* pVoice = pSpeaker;
    VOSound* pSounds[64];
    int iCount = 0;
    if (pSpeaker->GetControllable() && pSpeaker->GetControllable()->GetPilot())
    {
        pVoice = pSpeaker->GetControllable()->GetPilot()->GetGameObject();
        if (!pVoice)
        {
            pVoice = pSpeaker;
        }
    }

    for (VOSound* pSound = pVoice->GetGameObjectClass()->mVOSoundList; pSound && iCount < 64;
         pSound = pSound->mNext)
    {
        if (pSound->mType == eType && HasRequiredModifiers(pSound, modifiers))
        {
            pSound->mScore = (unsigned char)GetModifierScore(pSound, modifiers);
            pSounds[iCount++] = pSound;
        }
    }
    if (iCount > 1)
    {
        qsort(pSounds, iCount, sizeof(pSounds[0]), VOSoundCompareCallback);
    }
    if (iCount <= 0)
    {
        return;
    }

    const bool bLocal = pSpeaker == pTarget || pVoice == pTarget;
    const PblVector3& vPos = pSpeaker->GetMatrix().GetTranslation();
    if (bLocal && pSounds[0]->mSound.GetPropertiesType() == GameSound::type_soundStream)
    {

        unsigned char* pProps = (unsigned char*)pSounds[0]->mSound.GetProperties();
        if (pProps)
        {
            pProps[0x50] &= ~0x40;
        }
    }
    pSounds[0]->mSound.Play(&vPos, 0, true, !bLocal);
}

// FUNCTION: SWBF2 0x006723a0
int Play(VOType eType, const GameObject* pSpeaker, const GameObject* pTarget,
         const PblVector3& vPos, float fDelay)
{
    if ((eType == VOT_AcquiredTarget || eType == VOT_PathFollowWait) &&
        AIUtil::GetRandomInt(0, 100) > 70)
    {
        return -1;
    }

    if (fDelay > GameLoop::GetMissionTime())
    {
        return Queue(eType, pSpeaker, pTarget, 0, &vPos, fDelay);
    }

    if (!pTarget)
    {
        Character* pPlayer = NetGame::GetLocalPlayer(0);
        if (pPlayer && pPlayer->GetControllable())
        {
            pTarget = pPlayer->GetControllable()->GetGameObject();
        }
    }
    if (pTarget)
    {
        VOModifier modifiers = VOModifier();
        GetModifiers(pTarget, vPos, modifiers);
        _Play(eType, pSpeaker, pTarget, modifiers);
    }
    return -1;
}

// FUNCTION: SWBF2 0x00672480
int Play(VOType eType, const GameObject* pSpeaker, const GameObject* pTarget,
         const GameObject* pOther, float fDelay)
{
    if ((eType == VOT_AcquiredTarget || eType == VOT_PathFollowWait) &&
        AIUtil::GetRandomInt(0, 100) > 70)
    {
        return -1;
    }

    if (fDelay > GameLoop::GetMissionTime())
    {
        return Queue(eType, pSpeaker, pTarget, pOther, 0, fDelay);
    }

    if (!pTarget)
    {
        Character* pPlayer = NetGame::GetLocalPlayer(0);
        if (pPlayer && pPlayer->GetControllable())
        {
            pTarget = pPlayer->GetControllable()->GetGameObject();
        }
    }
    if (pTarget)
    {
        VOModifier modifiers = GetModifiers(pTarget, pOther);
        _Play(eType, pSpeaker, pTarget, modifiers);
    }
    return -1;
}

// FUNCTION: SWBF2 0x00672550
int Queue(VOType eType, const GameObject* pSpeaker, const GameObject* pTarget,
          const GameObject* pOther, const PblVector3* pPos, float fDelay)
{
    for (int i = 0; i < 20; ++i)
    {
        if (mSndQueue[i].mType != VOT_NONE)
        {
            continue;
        }
        mSndQueue[i].mType = eType;
        mSndQueue[i].mSpeaker = pSpeaker;
        mSndQueue[i].mPlayer = pTarget;
        mSndQueue[i].mTarget = pOther;
        if (pPos)
        {
            mSndQueue[i].mPos = *pPos;
        }
        mSndQueue[i].mUseTarget = pPos == 0;
        mSndQueue[i].mTime = fDelay;
        mSndQueue[i].mHandle = mNextQueueHandle++;
        return mSndQueue[i].mHandle;
    }
    return -1;
}

// FUNCTION: SWBF2 0x00672620
void CancelQueuedSound(int iHandle)
{
    for (int i = 0; i < 20; ++i)
    {
        if (mSndQueue[i].mHandle == iHandle)
        {
            mSndQueue[i].mType = VOT_NONE;
        }
    }
}

}
}

 
 
 

// FUNCTION: SWBF2 0x00671940
AI::VOType AI::VOHelper::TypeStrToEnum(const char* pszType)
{
    unsigned int hash = PblHash(pszType);
    for (int i = 0; i < VOT_Count; ++i)
    {
        if (hash == VOTypeNames[i])
        {
            return (VOType)i;
        }
    }
    return VOT_NONE;
}

// FUNCTION: SWBF2 0x00671970
AI::VOSound* AI::VOHelper::GetNewVOSound(const char* pszSound, const char* pszType, const AI::VOModifier& modifiers)
{
    using namespace AI::VOHelper;
    if (!sLoadingBatch)
    {
        sLoadingBatch = new AI::VOSound[16];
        sLoadingBatchIndex = 0;
    }
    AI::VOSound* pSound = &sLoadingBatch[sLoadingBatchIndex];

    int iType = TypeStrToEnum(pszType);
    pSound->mSound.SetID(pszSound, false, false);
    pSound->mType = iType;
    pSound->mModifiers = modifiers;
    pSound->mNext = 0;

    if (++sLoadingBatchIndex >= 16)
    {
        sLoadingBatch = 0;
    }
    return pSound;
}

// FUNCTION: SWBF2 0x00671a60
AI::VOSound::VOSound()
    : mModifiers()
{
}


namespace AI
{

namespace VOHelper
{

// SYNTHETIC: SWBF2 0x006715c0
// AI::VOHelper::QueuedSound::QueuedSound

// SYNTHETIC: SWBF2 0x00406f50
// AI::VOHelper::`dynamic initializer for 'mSndQueue''
// SYNTHETIC: SWBF2 0x00769780
// AI::VOHelper::`dynamic atexit destructor for 'mSndQueue''

// GLOBAL: SWBF2 0x01f9a320
// mSndQueue
QueuedSound mSndQueue[20];

// GLOBAL: SWBF2 0x01eafd30
int mNextQueueHandle;
// GLOBAL: SWBF2 0x01eafd34
VOSound* sLoadingBatch;
// GLOBAL: SWBF2 0x01eafd38
int sLoadingBatchIndex;

// FUNCTION: SWBF2 0x00671600
void Reset()
{
    sLoadingBatchIndex = 0;
    sLoadingBatch = 0;
    memset(mSndQueue, 0, sizeof(mSndQueue));
    mNextQueueHandle = 1;
}

}
}

namespace AI
{
namespace VOHelper
{

// FUNCTION: SWBF2 0x00672640
void Update()
{
    using namespace AI::VOHelper;

    const float fTime = GameLoop::GetMissionTime();
    for (int i = 0; i < 20; ++i)
    {
        QueuedSound& rSound = mSndQueue[i];
        if (rSound.mType == AI::VOT_NONE || !(fTime > rSound.mTime))
        {
            continue;
        }
        if (rSound.mUseTarget)
        {
            Play(rSound.mType, rSound.mSpeaker, rSound.mPlayer, rSound.mTarget, 0.0f);
        }
        else
        {
            Play(rSound.mType, rSound.mSpeaker, rSound.mPlayer, rSound.mPos, 0.0f);
        }
        rSound.mType = AI::VOT_NONE;
    }
}
}
}
