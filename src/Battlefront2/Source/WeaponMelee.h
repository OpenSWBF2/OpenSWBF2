#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "CollisionObject.h"
#include "Damageable.h"
#include "GameSound.h"
#include "LightsaberTrailEffect.h"
#include "Weapon.h"

class Combo;
class ExplosionClass;
class PblFileChunk;
class PblMatrix;
class RedModel;
class WeaponMeleeClass;
class WeaponMeleeThrow;

// VTABLE: SWBF2 0x007b1578
class WeaponMelee : public Weapon
{
public:

    enum ComboState
    {
        COMBO_IDLE = 0
    };

    enum WeaponMeleeState
    {
        MELEE_IDLE = 0,
        MELEE_ATTACK = 1,
        MELEE_ATTACK2 = 2,
        MELEE_CHARGE = 3,
        MELEE_DEFLECT = 4,
        MELEE_RECOVER = 5,
        MELEE_EMPTY = 6,
        NUM_WEAPON_MELEE_STATES = 7
    };

    struct DamageData
    {
        enum { MAX_OBJECTS_PER_ATTACK = 8 };

        bool HasHit(const CollisionObject* pObject) const
        {
            const unsigned int uKey = CollisionObject::LastCollidedKey(pObject);
            for (int k = 0; k < iNumObjects; ++k)
            {
                if (auHitKey[k] == uKey)
                {
                    return true;
                }
            }
            return false;
        }

        void NoteHit(const CollisionObject* pObject)
        {
            auHitKey[iNumObjects++] = CollisionObject::LastCollidedKey(pObject);
        }

        float fTime;
        const void* pAttack;
        int iNumObjects;
        unsigned int auHitKey[8];
        DamageData* pNext;
    };

    class InputQueue
    {
    public:
        struct Event
        {
            float fTime;
            unsigned char uiButton;
            unsigned char uiButtonAction;
            unsigned short z_uiReserved;
        };

        InputQueue() : m_uiCount(0), m_uiStart(0), m_uiButtonDown(0) {}

        int GetCount() const { return m_uiCount; }
        bool IsButtonDown(unsigned char uiButton) const { return (m_uiButtonDown & (1 << uiButton)) != 0; }
        const Event& GetEvent(int iEvent) const { return m_aEventQueue[(m_uiStart + (unsigned char)(m_uiCount - iEvent - 1)) & INPUT_QUEUE_MASK]; }
        const Event& operator[](int iEvent) const { return m_aEventQueue[(m_uiStart + (unsigned char)iEvent) & INPUT_QUEUE_MASK]; }
        void Update(unsigned char uiButtons, float fTime);
        void Clear() { m_uiCount = 0; m_uiStart = 0; }
        void ClearAll() { m_uiStart = 0; m_uiButtonDown = 0; m_uiCount = 0; }
        void Push(float fTime, unsigned char uiButton, unsigned char uiAction)
        {
            Event& event = m_aEventQueue[(m_uiStart + m_uiCount) & INPUT_QUEUE_MASK];
            event.fTime = fTime;
            event.uiButton = uiButton;
            event.uiButtonAction = uiAction;
            if (m_uiCount >= INPUT_QUEUE_LENGTH)
            {
                ++m_uiStart;
            }
            else
            {
                ++m_uiCount;
            }
        }
        void Pop(int iCount)
        {
            if (iCount >= m_uiCount)
            {
                Clear();
            }
            else if (iCount > 0)
            {
                m_uiCount -= iCount;
                m_uiStart = (m_uiStart + iCount) & INPUT_QUEUE_MASK;
            }
        }

    private:
        static const int INPUT_QUEUE_LENGTH_LOG2 = 3;
        static const int INPUT_QUEUE_LENGTH = 1 << INPUT_QUEUE_LENGTH_LOG2;
        static const unsigned char INPUT_QUEUE_MASK = INPUT_QUEUE_LENGTH - 1;
        Event m_aEventQueue[INPUT_QUEUE_LENGTH];
        unsigned char m_uiCount;
        unsigned char m_uiStart;
        unsigned char m_uiButtonDown;
        unsigned char z_uiReserved;
    };

    class TrailData;

    WeaponMelee(const WeaponMeleeClass* pClass, const WeaponDesc& desc);
    virtual ~WeaponMelee();
    virtual bool Update(float fDeltaTime);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual bool Deflect(const Ordnance* pOrdnance, const PblVector3& pos,
                         const PblVector3& dir);
    virtual bool ShouldShowReticule() const;
    virtual bool IsMelee() const;
    virtual void NotifySoldierState(int iState, float fTime, bool b0, bool b1,
                                    const PblVector3& vec);
    virtual bool SoldierCanOperate() const;
    virtual bool OverrideSoldierVelocity(float& f0, float& f1, float& f2,
                                         float& f3) const;
    virtual bool OverrideSoldierControls();
    virtual bool OverrideSoldierEnergyRestore(float& fRestore) const;
    virtual bool OverrideAimer();
    virtual void Select(unsigned int uFlags, bool bOffhand);
    virtual void Deselect(unsigned int uFlags, bool bOffhand);
    virtual bool IsBusy() const;
    virtual bool WillBecomeBusy() const;
    virtual void Write(NetPktGroup& group);
    virtual void Read(NetPktGroup& group);
    virtual void Render(const PblMatrix& mat, const RedPose* pPose,
                        const RedColor& color, unsigned int uFlags,
                        bool bArg) const;
    virtual void EnterIdle();
    virtual bool UpdateIdle(float fDeltaTime);
    virtual void ExitIdle();
    virtual void EnterFire();
    virtual bool UpdateFire(float fDeltaTime);
    virtual void ExitFire();
    virtual void EnterRecover();
    virtual void ExitRecover();
    virtual bool UpdateRecover(float fDeltaTime);
    virtual void EnterDeflect();
    virtual void ExitDeflect();
    virtual bool UpdateDeflect(float fDeltaTime);

    WeaponMeleeState GetMeleeState() const { return (WeaponMeleeState)mState; }
    unsigned char GetWeaponMeleeAnimation() const;
    unsigned char GetNextWeaponMeleeAnimation() const;
    unsigned char GetOffhandWeaponMeleeAnimation(int eState) const;
    unsigned char GetWeaponMeleeThrowAnimation() const;
    bool IsAnimationVelocityAligned() const;
    bool IsFullBodyAnimation() const;
    void UpdateEffects(float fDeltaTime, const PblVector3& vPos, const PblVector3& vVel,
                       const GameSound* pSound);

    static DamageData* AllocDamageData();
    static void FreeDamageData(DamageData* pData);

    void TurnOn(float fDuration);
    void TurnOff(float fDuration);
    void TurnOnTrail(bool bOn) { m_uiTrailOnMask = bOn ? 0xf : 0; }

    float GetLightSaberLengthFactor() const;
    bool IsOn() const { return m_bLightsaberOn || m_fTurnOnTimer < m_fTurnOnDuration; }
    bool IsOn(unsigned int uiBlade) const;
    float GetLightSaberLengthFactor(unsigned int uiBlade) const;
    void RenderLightsaberTrail(unsigned int uiEdge, const PblMatrix& mMatrix, bool bTrailOn,
                               unsigned int uiFlags) const;
    bool TestInputQueueForButtons(unsigned int iPressedMask, unsigned int iReleasedMask) const;
    bool TestConditionExceptInputQueue(const void* pConditionPtr, float fPreviousTime,
                                       float fCurrentTime) const;
    bool TestCondition(const void* pConditionPtr, float fPreviousTime, float fCurrentTime,
                       float& fMatchTime);

protected:
    void ClearInput();
    Trigger* GetButton(unsigned char uiButton);
    void ReadButtons();
    bool AlignVectorToThrust(PblVector3& v, bool bInitial) const;
    void ReleaseDamageData();
    void EnterState(ComboState eState);
    bool UpdateState(float fDeltaTime);

    void SetNetworkState(ComboState eState, bool bAnimatedMove);
    void ExitState();

    friend class WeaponMeleeClass;

    static void _InitDamageDataPool();
    static void _InitTrailDataPool();

    static TrailData* _AllocTrailData();
    static void _FreeTrailData(TrailData* pData);

public:
    static void PostLoad();
    static PblHash rttiHashWeaponMelee;

    static int _ClearInputQueueToTime(InputQueue& queue, float fTime);
    static bool _TestInputQueueForButtonEvent(const InputQueue& queue, float fTime,
                unsigned int iButton, unsigned int iAction, float fBegin, float fEnd, float& fMatch);
    static const char rttiNameWeaponMelee[];

    static TrailData* s_aTrailDataPool;
    static TrailData* s_pTrailDataFreeList;
    static DamageData* s_aDamageDataPool;
    static DamageData* s_pDamageDataFreeList;

    WeaponMeleeThrow* m_pMeleeThrow;
    const Combo* m_pCombo;
    float m_fSoldierStateTimer;
    float m_fInputThrustZ;
    float m_fInputThrustX;
    PblVector3 m_vVelocityLocal;
    float m_fInputTimer;
    InputQueue m_InputQueue;
    unsigned char m_uiButtonDown;
    unsigned char m_bAnimatedMove : 1;
    unsigned char m_bLightsaberOn : 1;
    unsigned char m_bLightsaberSoundOn : 1;
    unsigned char m_bSoldierInputLock : 1;
    unsigned char m_bUnreachableState_WeaponUpdate : 1;
    unsigned char m_bUnreachableState_SoldierUpdate : 1;
    unsigned char m_bGravityDone : 1;
    unsigned char m_bAI_Deflect : 1;
    unsigned char m_bRestartAnimation : 1;
    unsigned char m_bInHeroFlee : 1;
    unsigned char m_uiAnimIndex;
    unsigned char m_uiTrailOnMask : 4;
    unsigned char m_uiLightsaberOnMask : 4;
    unsigned char m_uiLightsaberWasOnMask : 4;
    unsigned char m_uiLightsaberSoundOnMask : 4;
    ComboState m_eComboState;
    ComboState m_eComboStatePrev;
    float m_fComboStateTimer;
    float m_fComboStateDurationPrev;
    float m_fAlignZ;
    float m_fAlignX;
    float m_fDeflectTimer;
    float m_fVelocityOverrideFactor;
    float m_fTurnOnTimer;
    float m_fTurnOnDuration;
    DamageData* m_pDamageData;
    mutable TrailData* m_pTrailData;
    GameSoundControllable m_OnOffSound;
    GameSound m_OnOffSoundProps;
};

// FUNCTION: SWBF2 0x00687bf0
inline bool WeaponMelee::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponMelee == uRtti)
    {
        return true;
    }
    return Weapon::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00687c20
inline unsigned int WeaponMelee::GetDerivedRtti() const
{
    return rttiHashWeaponMelee;
}

// FUNCTION: SWBF2 0x00687c30
inline const char* WeaponMelee::GetDerivedRttiName() const
{
    return rttiNameWeaponMelee;
}

// VTABLE: SWBF2 0x007b1534
class WeaponMeleeClass : public WeaponClass
{
public:
    WeaponMeleeClass(unsigned int uHash);
    virtual ~WeaponMeleeClass();

    static void Init();

    WeaponMeleeClass(WeaponMeleeClass* aParent, unsigned int aId);

    static void PostLoad();

    static void ReadCombo(PblFileChunk& Chunk);

    struct DamageEdge
    {
        unsigned int uiAttachToWeapon;
        unsigned int uiEdgeHP;
        PblVector3 vOffsetPos;
        PblVector3 vOffsetDir;
        float fLength;
        float fWidth;
        unsigned int uiTextureHash;
        unsigned int uiGlowTextureHash;
        RedColor colorTrail;
    };

    void GetDamageEdgeRay(unsigned int uiEdge, unsigned int& uiAttach,
                          PblVector3& vBegin, PblVector3& vEnd) const;

    virtual WeaponClass* Derive(unsigned int uHash);
    virtual Weapon* Build(const WeaponDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    bool IsLightSaber() const;
    void PlayTurnOnSound() const;
    void PlayTurnOffSound() const;

    void Render(float fTurnOn, const PblMatrix& mMatrix, const class RedPose* pPose, const RedColor& color,
                unsigned int uiFlags, bool bArg) const;

    static PblHash rttiHashWeaponMeleeClass;
    static const char rttiNameWeaponMeleeClass[];

    Combo* m_pComboODF;
    const Combo* m_pCombo;
    ExplosionClass* mImpactExplosion;
    unsigned int m_uiHitEffects[5];
    short m_uiNumHitEffects;
    GameSound m_HitSound;
    unsigned int m_uiDeflectEffect;
    GameSound m_DeflectSound;
    GameSound m_OnSound;
    GameSound m_TurnOnSound;
    GameSound m_TurnOffSound;
    float m_fTurnOnDuration;
    DamageDesc mDamage;
    unsigned int m_uiNumDamageEdges;
    DamageEdge* m_aDamageEdge;
    const RedModel* m_apWeaponModel[3];
    unsigned int m_auiHardpoint[4];
    unsigned int m_uiNumWeapons;

    int m_eDeflectMatrixMap;
    PblMatrix* m_amDeflectMatrix;

protected:

    int _AttachDamageEdge(const RedModel* pAttachToModel, unsigned int uiAttachModelToHP,
                          unsigned int uiAttachToModelHP);

    DamageEdge* _CreateDamageEdges(unsigned int uiNumEdges);

    Combo* _CreateODFCombo(unsigned int uiComboId);
    void _InitDeflectMatrices(int eMap);
};
 
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x00687c90
inline bool WeaponMeleeClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashWeaponMeleeClass == uRtti)
    {
        return true;
    }
    return WeaponClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00687cc0
inline unsigned int WeaponMeleeClass::GetDerivedRtti() const
{
    return rttiHashWeaponMeleeClass;
}

// FUNCTION: SWBF2 0x00687cd0
inline const char* WeaponMeleeClass::GetDerivedRttiName() const
{
    return rttiNameWeaponMeleeClass;
}

inline void WeaponMelee::PostLoad()
{
    WeaponMeleeClass::PostLoad();
}
