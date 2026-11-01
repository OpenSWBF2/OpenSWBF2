#if defined(_M_IX86)

#include <stddef.h>

#include "Battlefront2/Source/Weapon.h"
#include "Battlefront2/Source/EntityFlyer.h"
#include "Battlefront2/Source/EntityCarrier.h"
#include "Battlefront2/Source/AmmoCounter.h"
#include "Battlefront2/Source/WeaponAreaEffect.h"
#include "Battlefront2/Source/WeaponDetonator.h"
#include "Battlefront2/Source/WeaponDisguise.h"
#include "Battlefront2/Source/WeaponDispenser.h"
#include "Battlefront2/Source/WeaponGrenade.h"
#include "Battlefront2/Source/WeaponInvisibility.h"
#include "Battlefront2/Source/WeaponLaser.h"
#include "Battlefront2/Source/WeaponLauncher.h"
#include "Battlefront2/Source/WeaponMeleeThrow.h"
#include "Battlefront2/Source/WeaponRemote.h"
#include "Battlefront2/Source/WeaponTowCable.h"

static_assert(sizeof(Thread) == 0x18, "thread size");
static_assert(sizeof(Weapon) == 0x110, "weapon size");
static_assert(offsetof(Weapon, mClass) == 0x64, "weapon class");
static_assert(offsetof(Weapon, mTarget) == 0x104, "weapon target");
static_assert(sizeof(WeaponClass) == 0x22c, "weapon class size");
static_assert(sizeof(AmmoCounter) == 0x18, "ammo counter size");
static_assert(offsetof(AmmoCounter, m_pClass) == 0x0, "ammo counter class");
static_assert(offsetof(AmmoCounter, mDefaultMaxClips) == 0x4, "ammo counter default max clips");
static_assert(offsetof(AmmoCounter, mMaxClips) == 0x8, "ammo counter max clips");
static_assert(offsetof(AmmoCounter, mNumClips) == 0xc, "ammo counter num clips");
static_assert(offsetof(AmmoCounter, mCurClip) == 0x10, "ammo counter cur clip");
static_assert(offsetof(AmmoCounter, m_uiRefCount) == 0x14, "ammo counter ref count");
static_assert(sizeof(EntityFlyer) == 0x1d90, "flyer size");
static_assert(offsetof(EntityFlyer, mVelocity) == 0x540, "flyer velocity");
static_assert(offsetof(EntityFlyer, mState) == 0x564, "flyer state");
static_assert(offsetof(EntityFlyer, mClass) == 0x62c, "flyer class");
static_assert(offsetof(EntityFlyer, mPose) == 0x1860, "flyer pose");
static_assert(sizeof(EntityCarrier) == 0x1df0, "carrier size");
static_assert(offsetof(EntityCarrier, mCargoArray) == 0x1d90, "carrier cargo array");
static_assert(offsetof(EntityCarrier, mCargoCount) == 0x1de0, "carrier cargo count");
static_assert(offsetof(WeaponDisguise, mSavedOwnerModel) == 0x110, "weapon disguise msavedownermodel");
static_assert(offsetof(WeaponDisguise, mOtherTeam) == 0x114, "weapon disguise motherteam");
static_assert(offsetof(WeaponDisguise, m_eState) == 0x118, "weapon disguise m_estate");
static_assert(offsetof(WeaponMeleeThrow, m_pPrimaryWeapon) == 0x110, "weapon melee throw m_pprimaryweapon");
static_assert(offsetof(WeaponDispenser, mNetFiredFlag) == 0x110, "weapon dispenser mnetfiredflag");
static_assert(offsetof(WeaponDispenser, mFired) == 0x111, "weapon dispenser mfired");
static_assert(offsetof(WeaponDispenser, mStrength) == 0x114, "weapon dispenser mstrength");
static_assert(offsetof(WeaponDispenser, mItemArray) == 0x118, "weapon dispenser mitemarray");
static_assert(offsetof(WeaponDispenser, mItemCount) == 0x11c, "weapon dispenser mitemcount");
static_assert(offsetof(WeaponRemote, mItem) == 0x120, "weapon remote mitem");
static_assert(offsetof(WeaponGrenade, mNetFiredFlag) == 0x110, "weapon grenade mnetfiredflag");
static_assert(offsetof(WeaponGrenade, mFired) == 0x111, "weapon grenade mfired");
static_assert(offsetof(WeaponGrenade, m_iFirePos) == 0x112, "weapon grenade m_ifirepos");
static_assert(offsetof(WeaponGrenade, mStrength) == 0x114, "weapon grenade mstrength");
static_assert(offsetof(WeaponInvisibility, m_fInvisibilityFactor) == 0x110, "weapon invisibility m_finvisibilityfactor");
static_assert(offsetof(WeaponInvisibility, m_fInvisibilityPersistTimer) == 0x114, "weapon invisibility m_finvisibilitypersisttimer");
static_assert(offsetof(WeaponInvisibility, m_fInvisibilityFlickerTimer) == 0x118, "weapon invisibility m_finvisibilityflickertimer");
static_assert(offsetof(WeaponInvisibility, m_bInvisibilityOn) == 0x11c, "weapon invisibility m_binvisibilityon");
static_assert(offsetof(WeaponInvisibility, m_bInvisibilityActive) == 0x11d, "weapon invisibility m_binvisibilityactive");
static_assert(offsetof(WeaponInvisibility, mFired) == 0x11e, "weapon invisibility mfired");
static_assert(offsetof(WeaponInvisibility, mOwnerHealth) == 0x120, "weapon invisibility mownerhealth");
static_assert(offsetof(WeaponLaser, mSalvoTimer) == 0x110, "weapon laser msalvotimer");
static_assert(offsetof(WeaponLauncher, mCurTarget) == 0x160, "weapon launcher mcurtarget");
static_assert(offsetof(WeaponLauncher, mCurTargetBodyID) == 0x168, "weapon launcher mcurtargetbodyid");
static_assert(offsetof(WeaponLauncher, mLocked) == 0x16c, "weapon launcher mlocked");
static_assert(offsetof(WeaponLauncher, mTrackingSoundInterval) == 0x170, "weapon launcher mtrackingsoundinterval");
static_assert(offsetof(WeaponLauncher, mTrackingSoundTimer) == 0x174, "weapon launcher mtrackingsoundtimer");
static_assert(offsetof(WeaponAreaEffect, m_apTarget) == 0x110, "weapon area effect m_aptarget");
static_assert(offsetof(WeaponAreaEffect, m_afTargetRange) == 0x130, "weapon area effect m_aftargetrange");
static_assert(offsetof(WeaponAreaEffect, m_iNumTargets) == 0x140, "weapon area effect m_inumtargets");
static_assert(offsetof(WeaponAreaEffect, m_fStrength) == 0x144, "weapon area effect m_fstrength");
static_assert(offsetof(WeaponAreaEffect, m_bFired) == 0x148, "weapon area effect m_bfired");
static_assert(offsetof(WeaponGrenadeClass, mShotDelay) == 0x22c, "weapon grenade class mshotdelay");
static_assert(offsetof(WeaponGrenadeClass, mInitialSalvoDelay) == 0x230, "weapon grenade class minitialsalvodelay");
static_assert(offsetof(WeaponGrenadeClass, mShotElevate) == 0x23c, "weapon grenade class mshotelevate");
static_assert(offsetof(WeaponGrenadeClass, mMaxPressedTime) == 0x240, "weapon grenade class mmaxpressedtime");
static_assert(offsetof(WeaponLaserClass, mShotDelay) == 0x22c, "weapon laser class mshotdelay");
static_assert(offsetof(WeaponLaserClass, mSalvoTime) == 0x230, "weapon laser class msalvotime");
static_assert(offsetof(WeaponInvisibilityClass, mInitialSalvoDelay) == 0x258, "weapon invisibility class minitialsalvodelay");
static_assert(offsetof(WeaponInvisibilityClass, m_eInvisibilityTrigger) == 0x22c, "weapon invisibility class m_einvisibilitytrigger");
static_assert(offsetof(WeaponLauncherClass, mLockTime) == 0x2e0, "weapon launcher class mlocktime");
static_assert(offsetof(WeaponAreaEffectClass, m_eFireType) == 0x22c, "weapon area effect class m_efiretype");

#endif
