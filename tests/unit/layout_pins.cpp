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
#include "Battlefront2/Source/Aimer.h"
#include "Battlefront2/Source/Controllable.h"
#include "Battlefront2/Source/GameObject.h"
#include "Battlefront2/Source/EntityGeometry.h"
#include "Battlefront2/Source/Ordnance.h"
#include "Battlefront2/Source/EntityWater.h"
#include "Battlefront2/Source/CameraManager.h"
#include "Battlefront2/Source/ChaseCamera.h"
#include "Battlefront2/Source/NetSetup.h"
#include "Battlefront2/Source/NetPacket.h"
#include "Battlefront2/Source/NetComm.h"
#include "Battlefront2/Source/FLInputManager.h"
#include "Battlefront2/Source/CommandFlyer.h"
#include "Battlefront2/Source/CommandHover.h"
#include "Battlefront2/Source/CommandWalker.h"
#include "Battlefront2/Source/EntityBuildingAnimatedArmed.h"
#include "Battlefront2/Source/EntityPortableTurret.h"
#include "RedEngineFL/Graphics/PC/pcRedAutoSettings.h"
#include "Battlefront2/Source/CollisionObject.h"
#include "RedEngineFL/Application/RedTerrain.h"
#include "Battlefront2/Source/PatrolPath.h"
#include "Battlefront2/Source/WeaponRepair.h"
#include "Battlefront2/Source/VOHelper.h"
#include "Battlefront2/Source/PlayerProfile.h"
#include "Battlefront2/Platform/Pc/pcSunFlare.h"
#include "Battlefront2/Platform/Pc/pcSpawnDisplay.h"
#include "RedEngineFL/Interface/RedBorderElement.h"
#include "RedEngineFL/Graphics/RedAmbientLight.h"
#include "RedEngineFL/movie/pc/pcRedMoviePlayer.h"
#include "lglcd/lgLcd.h"
#include "Battlefront2/Source/pcNetSocket.h"
#include "Battlefront2/Source/EntityPathFollower.h"
#include "Battlefront2/Source/TriggerSoundRegion.h"
#include "SoundFL/source/sndstreammixer.h"
#include "SoundFL/source/sndstreamsplitter.h"
#include "SoundFL/source/sndgsm.h"
#include "Battlefront2/Source/GameVoiceChatEncoder.h"
#include "Battlefront2/Source/VoiceChatNode.h"
#include "Battlefront2/Source/VoiceChatTransport.h"
#include "Battlefront2/Source/NetAdminMd5.h"
#include "Battlefront2/Source/NetAdmin.h"
#include "Battlefront2/Source/ParticleEmitterObject.h"

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
static_assert(offsetof(Weapon, mAimer) == 0x70, "weapon aimer");
static_assert(sizeof(Aimer) == 0x220, "aimer size");
static_assert(offsetof(Aimer, mFirePos) == 0x88, "aimer fire pos");
static_assert(offsetof(Aimer, mWeapon) == 0x208, "aimer weapon");
static_assert(sizeof(EntityGeometry) == 0x140, "entity geometry size");
static_assert(sizeof(GameObject) == 0x240, "game object size");
static_assert(offsetof(GameObject, mEntityClass) == 0x8, "game object entity class");
static_assert(offsetof(GameObject, mHandleId) == 0x204, "game object handle id");
static_assert(offsetof(GameObject, mNetUniqueId) == 0x21c, "game object net unique id");
struct GameObjectLayoutPins : GameObject
{
    static void Check()
    {
        static_assert(offsetof(GameObjectLayoutPins, mCreatedTime) == 0x230, "game object created time");
    }
};
static_assert(offsetof(GameObject, mDamageRegionTime) == 0x1f8, "game object damage region time");
static_assert(sizeof(Damageable) == 0xc0, "damageable size");
static_assert(offsetof(Damageable, mDamageRegionTime) == 0xb8, "damageable damage region time");
static_assert(sizeof(Ordnance) == 0x98, "ordnance size");
static_assert(offsetof(Ordnance, mCollisionManagerNode) == 0x78, "ordnance collision manager node");
static_assert(sizeof(EntityWater) == 0xa4, "entity water size");
static_assert(offsetof(EntityWater, totalTime) == 0xa0, "entity water total time");
static_assert(sizeof(OrdnanceDesc) == 0x48, "ordnance desc size");
static_assert(offsetof(OrdnanceDesc, mDamageOwner) == 0x20, "ordnance desc damage owner");
static_assert(offsetof(CameraManager, m_iNumCam) == 0x1c, "camera manager num cam");
static_assert(offsetof(CameraManager, mChaseCamera) == 0x28, "camera manager chase camera");
static_assert(offsetof(ChaseCamera, mMatrix) == 0x10, "chase camera matrix");
static_assert(offsetof(NetGameDesc, mFindData) == 0x3c, "net game desc find data");
static_assert(offsetof(FindGameData, mGalaxyLobbyPasswordData) == 0xb5, "find game data lobby password");
static_assert(sizeof(FindGameData::mGalaxyLobbyPasswordData) == 64, "find game data lobby password size");
static_assert(sizeof(NetPacket) == 0x10, "net packet size");
static_assert(offsetof(NetPacket, mData) == 0xc, "net packet data");
static_assert(offsetof(NetBigPacket, mData2) == 0x10, "net big packet data");
static_assert(offsetof(NetSmallPacket, mData2) == 0x10, "net small packet data");
static_assert(sizeof(NetBigPacket) == 0x400, "net big packet size");
static_assert(sizeof(NetSmallPacket) == 0x80, "net small packet size");
static_assert(sizeof(NetComm::PktHeader) == 0x8, "net packet header size");
static_assert(offsetof(NetComm::PktHeader, group) == 0x0, "net packet header group");
static_assert(offsetof(NetComm::PktHeader, type) == 0x2, "net packet header type");
static_assert(offsetof(NetComm::PktHeader, src) == 0x3, "net packet header src");
static_assert(offsetof(NetComm::PktHeader, idx) == 0x4, "net packet header idx");
static_assert(offsetof(NetComm::PktHeader, count) == 0x5, "net packet header count");
static_assert(offsetof(NetComm::PktHeader, compressed) == 0x6, "net packet header compressed");
static_assert(offsetof(PblList<Controllable>::Node, _pNext) == 0x4, "controllable list node next");
static_assert(offsetof(PblList<Controllable>::Node, _pObject) == 0xc, "controllable list node object");
static_assert(sizeof(RawControllerInputs) == 0x28a0, "raw controller inputs size");
static_assert(offsetof(RawControllerInputs, mMode) == 0x8, "raw controller inputs mode");
static_assert(offsetof(RawControllerInputs, mRawInputFloat) == 0x14, "raw controller inputs float");
static_assert(offsetof(RawControllerInputs, mLockedRawInputBool) == 0x17d4, "raw controller inputs locked bool");
static_assert(offsetof(RawControllerInputs, mCurBindings) == 0x1acc, "raw controller inputs bindings");
static_assert(offsetof(RawControllerInputs, mLastMouseDeltaY) == 0x289c, "raw controller inputs mouse delta y");
static_assert(offsetof(RawControllerInputs, mLCDPanel) == 0x25fc, "raw controller inputs lcd panel");
static_assert(offsetof(RawControllerInputs, _redKeyboard) == 0x2608, "raw controller inputs keyboard");
static_assert(alignof(RawControllerInputs) == 8, "raw controller inputs alignment");
static_assert(offsetof(RawControllerInputs, mFlip) == 0xc, "raw controller inputs flip");
static_assert(offsetof(RawControllerInputs, mLastRawInputFloat) == 0xbf4, "raw controller inputs last float");
static_assert(offsetof(RawControllerInputs, mCurKeyboardBindings) == 0x20bc, "raw controller inputs keyboard bindings");
static_assert(offsetof(RawControllerInputs, m_pGuiInputs) == 0x25c8, "raw controller inputs gui inputs");
static_assert(offsetof(RawControllerInputs, mProcessAfterReading) == 0x25cc, "raw controller inputs process after reading");
static_assert(offsetof(RawControllerInputs, mProcessDestination) == 0x25d0, "raw controller inputs process destination");
static_assert(offsetof(RawControllerInputs, _redJoystick) == 0x25d4, "raw controller inputs joysticks");
static_assert(offsetof(RawControllerInputs, mAttachedJoysticks) == 0x25d4, "raw controller inputs attached joysticks");
static_assert(offsetof(RawControllerInputs, mMouseSensitivity) == 0x2610, "raw controller inputs mouse sensitivity");
static_assert(sizeof(RawControllerInputs::mMouseSensitivity) == 0x28, "raw controller inputs mouse sensitivity size");
static_assert(offsetof(RawControllerInputs, mJoystickSensitivityX) == 0x2638, "raw controller inputs joystick sensitivity x");
static_assert(offsetof(RawControllerInputs, mJoystickSensitivityY) == 0x263d, "raw controller inputs joystick sensitivity y");
static_assert(offsetof(RawControllerInputs, mMouseAssist) == 0x2642, "raw controller inputs mouse assist");
static_assert(offsetof(RawControllerInputs, mMouseOldX) == 0x2880, "raw controller inputs mouse old x");
static_assert(offsetof(RawControllerInputs, mMouseOldY) == 0x2888, "raw controller inputs mouse old y");
static_assert(offsetof(RawControllerInputs, mReservedAfterFlip) == 0x11, "raw controller inputs reserved after flip");
static_assert(offsetof(RawControllerInputs, mReservedBeforeGuiInputs) == 0x25c6, "raw controller inputs reserved before gui inputs");
static_assert(offsetof(RawControllerInputs, mReservedBeforeProcessDestination) == 0x25cd, "raw controller inputs reserved before process destination");
static_assert(offsetof(RawControllerInputs, mReservedBeforeMouse) == 0x2602, "raw controller inputs reserved before mouse");
static_assert(offsetof(FLInputManager, mReservedBeforeTextTimer) == 0x416, "input manager reserved before text timer");
static_assert(offsetof(FLInputManager, mReservedBeforeJoysticks) == 0x424, "input manager reserved before joysticks");
static_assert(offsetof(FLInputManager, mReservedBeforeMissingPort) == 0x2e75, "input manager reserved before missing port");
static_assert(sizeof(ActionKey) == 0x6, "action key size");
static_assert(offsetof(ActionKey, mKey[0]) == 0x0, "action key primary");
static_assert(offsetof(ActionKey, mKey[2]) == 0x4, "action key third");
static_assert(offsetof(RedLogitechLCD, mOpened) == 0x5, "logitech lcd opened");
static_assert(sizeof(FLInputManager) == 0x2f88, "input manager size");
static_assert(offsetof(FLInputManager, mJoysticks) == 0x428, "input manager joysticks");
static_assert(alignof(FLInputManager) == 8, "input manager alignment");
static_assert(offsetof(FLInputManager, m_inputs) == 0x2cc8, "input manager processed inputs");
static_assert(offsetof(FLInputManager, m_rumble) == 0x2ce0, "input manager rumble");
static_assert(offsetof(FLInputManager, m_guiInputs) == 0x2d14, "input manager gui inputs");
static_assert(offsetof(FLInputManager, mActiveControllers) == 0x2e6c, "input manager active controllers");
static_assert(offsetof(FLInputManager, m_bIsMissing) == 0x2e7c, "input manager missing flag");
static_assert(offsetof(SpawnDisplay, mVisible) == 0x1fea, "spawn display visible");
static_assert(sizeof(RedBorderElement) == 0x260, "border element size");
static_assert(sizeof(RedAmbientLight) == 0x70, "ambient light size");
static_assert(offsetof(RedLightingState, ambientLight) == 0x0, "lighting state ambient light");
static_assert(offsetof(RedLightingState, pDirectionalLights) == 0x70, "lighting state directional lights");
static_assert(sizeof(CompressedPos) == 0x8, "compressed pos size");
static_assert(sizeof(CommandFlyer) == 0x1da0, "command flyer size");
static_assert(offsetof(CommandFlyer, _tailPad) == 0x1d99, "command flyer tail pad");
static_assert(sizeof(CommandHover) == 0x1d20, "command hover size");
static_assert(offsetof(CommandHover, _tailPad) == 0x1d18, "command hover tail pad");
static_assert(sizeof(CommandWalker) == 0x2110, "command walker size");
static_assert(offsetof(CommandWalker, _tailPad) == 0x2104, "command walker tail pad");
static_assert(sizeof(EntityBuildingAnimatedArmed) == 0x1890, "animated armed building size");
static_assert(offsetof(EntityBuildingAnimatedArmed, mTailPadding) == 0x1884, "animated armed building tail pad");
static_assert(sizeof(WeaponGrenade) == 0x120, "weapon grenade size");
static_assert(offsetof(WeaponGrenade, mUnmodeledTail) == 0x118, "weapon grenade tail");
static_assert(sizeof(WeaponRepair) == 0x120, "weapon repair size");
static_assert(offsetof(WeaponRepair, mUnmodeledTail) == 0x111, "weapon repair tail");
static_assert(offsetof(EntityCarrier, _tailPad) == 0x1de4, "carrier tail pad");
static_assert(sizeof(EntityPortableTurret) == 0x6d0, "portable turret size");
static_assert(offsetof(EntityPortableTurret, mBecomeStationaryNow) == 0x6c9, "portable turret become stationary now");
static_assert(offsetof(EntityPortableTurret, _tailPad) == 0x6ca, "portable turret tail pad");
static_assert(sizeof(WeaponLauncher) == 0x180, "weapon launcher size");
static_assert(offsetof(WeaponLauncher, mLocked) == 0x16c, "weapon launcher locked");
static_assert(offsetof(WeaponLauncher, mUnmodeledTail) == 0x178, "weapon launcher tail");
static_assert(sizeof(RenderOptionsT) == 0x48, "render options size");
static_assert(offsetof(RenderOptionsT, distortion) == 0x15, "render options distortion");
static_assert(offsetof(RenderOptionsT, mReservedAfterDistortion) == 0x16, "render options reserved after distortion");
static_assert(offsetof(RenderOptionsT, mGammaBrightness) == 0x18, "render options gamma brightness");
static_assert(offsetof(RenderOptionsT, mEnableHDR) == 0x28, "render options enable hdr");
static_assert(offsetof(RenderOptionsT, mReservedAfterHDR) == 0x29, "render options reserved after hdr");
static_assert(offsetof(RenderOptionsT, defaultWidth) == 0x2c, "render options default width");
static_assert(offsetof(RenderOptionsT, bumpMapping) == 0x3c, "render options bump mapping");
static_assert(offsetof(RenderOptionsT, mReservedAfterBumpMapping) == 0x3d, "render options reserved after bump mapping");
static_assert(offsetof(RenderOptionsT, specularQuality) == 0x40, "render options specular quality");
static_assert(sizeof(BINK) == 0xd4, "bink size");
static_assert(offsetof(BINK, NumTracks) == 0xd0, "bink num tracks");
static_assert(sizeof(CNetFindGame) == 0x6c, "find game size");
static_assert(alignof(CNetFindGame) == 0x4, "find game alignment");
static_assert(offsetof(CNetFindGame, queryTimer) == 0x34, "find game query timer");
static_assert(offsetof(CNetFindGame, queryDone) == 0x38, "find game query done");
static_assert(offsetof(CNetFindGame, queryAddr) == 0x3c, "find game query addr");
static_assert(offsetof(CNetFindGame, queryAddr.mSocketAddress) == 0x44, "find game query addr socket address");
static_assert(offsetof(CNetFindGame, findSessionId) == 0x54, "find game session id");
static_assert(offsetof(CNetFindGame, mWaitingForQueries) == 0x5c, "find game waiting for queries");
static_assert(offsetof(CollisionObject, mCollisionObjectNode) == 0x38, "collision object node");
static_assert(sizeof(CollisionObject) == 0x88, "collision object size");
static_assert(sizeof(TreeGridObject) == 0x24, "tree grid object size");
static_assert(offsetof(TreeGridObject, mCollisionSphere) == 0x8, "tree grid object collision sphere");
static_assert(offsetof(TreeGridObject, m2dRadius) == 0x18, "tree grid object 2d radius");
static_assert(offsetof(TreeGridObject, mData) == 0x1c, "tree grid object data");
static_assert(offsetof(TreeGridObject, mTreeGridCellIndex) == 0x20, "tree grid object cell index");
static_assert(sizeof(RedTerrain::Hole) == 0x3c, "terrain hole size");
static_assert(offsetof(RedTerrain::Hole, iNumPlanes) == 0x24, "terrain hole num planes");
static_assert(offsetof(RedTerrain::Hole, aPlanes) == 0x28, "terrain hole planes");
static_assert(offsetof(RedTerrain::Hole, node) == 0x2c, "terrain hole node");
static_assert(offsetof(TreeGridStack, mData) == 0x4, "tree grid stack data");
static_assert(offsetof(TreeGridStack, mSphere) == 0x40, "tree grid stack sphere");
static_assert(sizeof(PblVector4) == 0x10, "vector4 size");
static_assert(offsetof(AI::PatrolPath, mReservingObject) == 0x2c, "patrol path reserving object");
static_assert(sizeof(AI::PatrolPath) == 0x38, "patrol path size");
static_assert(sizeof(PlayerPrefs::OnePrefs) == 0x2c, "one prefs size");
static_assert(offsetof(PlayerPrefs::OnePrefs, pProfile) == 0x14, "one prefs profile");
static_assert(offsetof(PlayerProfile::tProfile, mName) == 0x2, "profile name");
static_assert(offsetof(EntityGeometryClass, mChunkDescriptors) == 0x68, "entity geometry class chunk descriptors");
static_assert(sizeof(NetSocketPeerRecord) == 0x18, "net socket peer record size");
static_assert(offsetof(NetSocketPeerRecord, mGalaxyId) == 0x0, "net socket peer record galaxy id");
static_assert(offsetof(NetSocketPeerRecord, mSocketAddress) == 0x8, "net socket peer record socket address");
static_assert(sizeof(GameObjectInterpolator) == 0x2d00, "game object interpolator size");
static_assert(offsetof(GameObjectInterpolator, mat) == 0x100, "game object interpolator mat");
static_assert(offsetof(GameObjectInterpolator, mFloats) == 0x2100, "game object interpolator floats");
static_assert(offsetof(GameObjectInterpolator, vec) == 0x2700, "game object interpolator vec");
static_assert(offsetof(EntityPathFollowerClass, mPathRollValues.mRollAcceleration) == 0x34, "path follower rollacceleration");
static_assert(offsetof(EntityPathFollowerClass, mPathRollValues.mRollLookAhead) == 0x38, "path follower rolllookahead");
static_assert(offsetof(EntityPathFollowerClass, mPathRollValues.mRollTolerance) == 0x3c, "path follower rolltolerance");
static_assert(offsetof(EntityPathFollowerClass, mPathRollValues.mRollSpeed) == 0x40, "path follower rollspeed");
static_assert(offsetof(EntityPathFollowerClass, mPathRollValues.mRollAccelResp) == 0x44, "path follower rollaccelresp");
static_assert(offsetof(EntityPathFollowerClass, mPathRollValues.mRollTurnFactor) == 0x48, "path follower rollturnfactor");
static_assert(offsetof(EntityPathFollowerClass, mPathRollValues.mRollSpeedFactor) == 0x4c, "path follower rollspeedfactor");
static_assert(offsetof(EntityPathFollowerClass, mPathRollValues.mRollDampening) == 0x50, "path follower rolldampening");
static_assert(offsetof(EntityPathFollowerClass, mSpeed) == 0x5c, "path follower speed");
static_assert(sizeof(EntityPathFollowerClass) == 0x7c, "path follower class size");
static_assert(offsetof(TriggerSoundRegion::Group, mNode) == 0x48, "trigger sound group node");
static_assert(sizeof(TriggerSoundRegion::Group) == 0x50, "trigger sound group size");
static_assert(sizeof(NetAddr) == 0x14, "net addr size");
static_assert(offsetof(NetAddr, mGalaxyId) == 0x0, "net addr galaxy id");
static_assert(sizeof(galaxy::api::GalaxyID) == 0x8, "net addr galaxy id size");
static_assert(offsetof(NetAddr, hasPriv) == 0x8, "net addr has priv");
static_assert(sizeof(Snd::StreamMixer) == 0x90, "stream mixer size");
static_assert(offsetof(Snd::StreamMixer, StreamNode::mFlags) == 0x20, "stream mixer node flags");
static_assert(offsetof(Snd::StreamMixer, mConnections) == 0x24, "stream mixer sink connections");
static_assert(offsetof(Snd::StreamMixer, mActive) == 0x2c, "stream mixer active");
static_assert(offsetof(Snd::StreamMixer, mMixedPacket) == 0x4c, "stream mixer mixed packet");
static_assert(offsetof(Snd::StreamMixer, mGain) == 0x6c, "stream mixer gain");
static_assert(offsetof(Snd::StreamMixer, mOutputGain) == 0x70, "stream mixer output gain");
static_assert(offsetof(Snd::StreamMixer, mInputPackets) == 0x74, "stream mixer input packets");
static_assert(offsetof(Snd::StreamMixer, mMixBufferOffsets) == 0x78, "stream mixer mix buffer offsets");
static_assert(offsetof(Snd::StreamMixer, mMixBuffer) == 0x7c, "stream mixer mix buffer");
static_assert(offsetof(Snd::StreamMixer, mMixBufferSize) == 0x80, "stream mixer mix buffer size");
static_assert(offsetof(Snd::StreamMixer, mMixBufferDataSize) == 0x84, "stream mixer mix buffer data size");
static_assert(offsetof(Snd::StreamMixer, mSynchronizedMixing) == 0x88, "stream mixer synchronized mixing");
static_assert(offsetof(Snd::StreamMixer, mFlags) == 0x89, "stream mixer flags");
static_assert(offsetof(Snd::StreamMixer, mInputChannels) == 0x8a, "stream mixer input channels");
static_assert(offsetof(Snd::StreamMixer, mOutputChannels) == 0x8b, "stream mixer output channels");
static_assert(offsetof(Snd::StreamMixer, mInterpolationRate) == 0x8c, "stream mixer interpolation rate");
static_assert(sizeof(Snd::StreamNode) == 0xc, "stream node size");
static_assert(offsetof(Snd::StreamNode, mFlags) == 0x8, "stream node flags");
static_assert(offsetof(Snd::SourceTransport, mFlags) == 0x4, "source transport flags");
static_assert(sizeof(Snd::StreamSourceProducer) == 0x18, "stream source producer size");
static_assert(sizeof(Snd::StreamSplitter) == 0x2c, "stream splitter size");
static_assert(offsetof(Snd::StreamSplitter, StreamNode::mFlags) == 0xc, "stream splitter node flags");
static_assert(offsetof(Snd::StreamSplitter, mConnection) == 0x10, "stream splitter sink connection");
static_assert(offsetof(Snd::StreamSplitter, mPackets) == 0x18, "stream splitter packets");
static_assert(offsetof(Snd::StreamSplitter, mFlushing) == 0x2a, "stream splitter flushing");
static_assert(sizeof(Snd::GSM) == 0x28c, "gsm size");
static_assert(offsetof(Snd::GSM, LARpp) == 0x24c, "gsm larpp");
static_assert(offsetof(Snd::GSM, j) == 0x26c, "gsm j");
static_assert(offsetof(Snd::GSM, nrp) == 0x270, "gsm nrp");
static_assert(offsetof(Snd::GSM, v) == 0x272, "gsm v");
static_assert(offsetof(Snd::GSM, msr) == 0x284, "gsm msr");
static_assert(offsetof(Snd::GSM, u) == 0x23c, "gsm u");
static_assert(offsetof(Snd::GSM, verbose) == 0x286, "gsm verbose");
static_assert(sizeof(GameVoiceChat::Encoder) == 0x50c, "voice encoder size");
static_assert(offsetof(GameVoiceChat::Encoder, mSource) == 0x4, "voice encoder source");
static_assert(offsetof(GameVoiceChat::Encoder, mEncoder) == 0x60, "voice encoder gsm encoder");
static_assert(offsetof(GameVoiceChat::Encoder, mVAD) == 0x474, "voice encoder vad");
static_assert(offsetof(GameVoiceChat::Encoder, mAGC) == 0x4b4, "voice encoder agc");
static_assert(sizeof(VoiceChat::Node::ReceiveStream) == 0x18, "voice receive stream size");
static_assert(offsetof(VoiceChat::Node::ReceiveStream, packet) == 0x0, "voice receive stream packet");
static_assert(offsetof(VoiceChat::Node::ReceiveStream, bufferedData) == 0x4, "voice receive stream buffered data");
static_assert(offsetof(VoiceChat::Node::ReceiveStream, activationTime) == 0x8, "voice receive stream activation time");
static_assert(offsetof(VoiceChat::Node::ReceiveStream, lastPacketSize) == 0xc, "voice receive stream last packet size");
static_assert(offsetof(VoiceChat::Node::ReceiveStream, numPackets) == 0x10, "voice receive stream num packets");
static_assert(offsetof(VoiceChat::Node::ReceiveStream, writeIndex) == 0x12, "voice receive stream write index");
static_assert(offsetof(VoiceChat::Node::ReceiveStream, readIndex) == 0x14, "voice receive stream read index");
static_assert(offsetof(VoiceChat::Node::ReceiveStream, decoderInput) == 0x16, "voice receive stream decoder input");
static_assert(sizeof(VoiceChat::AudioPacket) == 0x10, "voice audio packet size");
static_assert(offsetof(VoiceChat::AudioPacket, mData) == 0x0, "voice audio packet data");
static_assert(offsetof(VoiceChat::AudioPacket, mSize) == 0x4, "voice audio packet size field");
static_assert(offsetof(VoiceChat::AudioPacket, mAppData) == 0x8, "voice audio packet app data");
static_assert(sizeof(VoiceChat::Node::ReceiveMetrics) == 0x24, "voice receive metrics size");
static_assert(offsetof(VoiceChat::Node::ReceiveMetrics, receiveLatency) == 0x0, "voice receive metrics latency");
static_assert(sizeof(VoiceChat::Node::SendMetrics) == 0x20, "voice send metrics size");
static_assert(sizeof(PblList<VoiceChat::Node>) == 0x14, "voice node list size");
static_assert(offsetof(PblList<VoiceChat::Node>, _head) == 0x0, "voice node list head");
static_assert(offsetof(PblList<VoiceChat::Node>, _iCount) == 0x10, "voice node list count");
static_assert(sizeof(PblList<VoiceChat::Node>::Node) == 0x10, "voice node list node size");
static_assert(offsetof(PblList<VoiceChat::Node>::Node, _pNext) == 0x4, "voice node list node next");
static_assert(offsetof(PblList<VoiceChat::Node>::Node, _pPrev) == 0x8, "voice node list node prev");
static_assert(offsetof(PblList<VoiceChat::Node>::Node, _pObject) == 0xc, "voice node list node object");
static_assert(sizeof(VoiceChat::Transport) == 0x58, "voice transport size");
static_assert(sizeof(VoiceChat::Transport::Bitmaps) == 0x10, "voice transport bitmaps size");
static_assert(sizeof(VoiceChat::Transport::Method) == 0x8, "voice transport method size");
static_assert(offsetof(VoiceChat::Transport, mLocal) == 0x0, "voice transport local");
static_assert(offsetof(VoiceChat::Transport, mIncoming) == 0x10, "voice transport incoming");
static_assert(offsetof(VoiceChat::Transport, mClientChangedNodes) == 0x20, "voice transport client changed nodes");
static_assert(offsetof(VoiceChat::Transport, mHostChangedNodes) == 0x24, "voice transport host changed nodes");
static_assert(offsetof(VoiceChat::Transport, mHostChangedPckID) == 0x28, "voice transport host changed pck id");
static_assert(offsetof(VoiceChat::Transport, mLocalNodes) == 0x2c, "voice transport local nodes");
static_assert(offsetof(VoiceChat::Transport, mHostUpdated) == 0x30, "voice transport host updated");
static_assert(offsetof(VoiceChat::Transport, mClientChangedPckID) == 0x34, "voice transport client changed pck id");
static_assert(offsetof(VoiceChat::Transport, mSize) == 0x38, "voice transport size field");
static_assert(offsetof(VoiceChat::Transport, mWidthHeight) == 0x3c, "voice transport width height");
static_assert(offsetof(VoiceChat::Transport, mWidthBytes) == 0x3d, "voice transport width bytes");
static_assert(offsetof(VoiceChat::Transport, mNumNodes) == 0x3e, "voice transport num nodes");
static_assert(offsetof(VoiceChat::Transport, mNumNodesLog2) == 0x3f, "voice transport num nodes log2");
static_assert(offsetof(VoiceChat::Transport, mNumLocalNodes) == 0x40, "voice transport num local nodes");
static_assert(offsetof(VoiceChat::Transport, mMaxLocalNodes) == 0x41, "voice transport max local nodes");
static_assert(offsetof(VoiceChat::Transport, mMethodOutgoing) == 0x44, "voice transport method outgoing");
static_assert(offsetof(VoiceChat::Transport, mMethodIncoming) == 0x4c, "voice transport method incoming");

static_assert(sizeof(MD5_CTX) == 0x58, "md5 context size");
static_assert(offsetof(MD5_CTX, state) == 0x0, "md5 context state");
static_assert(offsetof(MD5_CTX, count) == 0x10, "md5 context count");
static_assert(offsetof(MD5_CTX, buffer) == 0x18, "md5 context buffer");
static_assert(sizeof(Admin) == 0x10, "admin size");
static_assert(offsetof(Admin, addr) == 0x0, "admin addr");
static_assert(offsetof(Admin, port) == 0x4, "admin port");
static_assert(offsetof(Admin, lastRecvTime) == 0x8, "admin last recv time");
static_assert(offsetof(Admin, admin) == 0xc, "admin flag");

static_assert(sizeof(ParticleEmitterObject) == 0x110, "particle emitter object size");
static_assert(sizeof(FLEffectObject) == 0x80, "fl effect object size");
static_assert(offsetof(ParticleEmitterObject, mStopFlag) == 0x104, "particle emitter object stop flag");
static_assert(offsetof(ParticleEmitterObject, bIsEmitterEmitter) == 0x105, "particle emitter object emitter emitter");
static_assert(offsetof(ParticleEmitterObject, mbAddedToScene) == 0x106, "particle emitter object added to scene");
static_assert(offsetof(ParticleEmitterObject, mIsChildPEO) == 0x107, "particle emitter object child");
static_assert(offsetof(ParticleEmitterObject, mReflectionEnable) == 0x108, "particle emitter object reflection");

#endif
