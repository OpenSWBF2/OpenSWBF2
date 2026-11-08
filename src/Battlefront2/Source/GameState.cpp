#include <vector>

#include <windows.h>
#include "PerformanceTimer.h"
#include "CommandPost.h"

#include <process.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "GameState.h"
#include "pcCrash.h"
#include "RedEngineFL/Application/pc/pcFrameLimiter.h"
#include "RedEngineFL/Graphics/PC/pcDisplaySettings.h"
#include "RedEngineFL/Graphics/PC/pcDisplayLogic.h"
#include "CommandLine.h"
#include "GameLoop.h"
#include "BonusManager.h"
#include "CollisionManager.h"
#include "NetGame.h"
#include "NetSetup.h"
#include "ShellLoop.h"
#include "CollisionBody.h"
#include "CollisionObject.h"
#include "GameMemory.h"
#include "RedEngineFL/Portals/Portal.h"
#include "GameMovie.h"
#include "GameSound.h"
#include "GameSoundEngine.h"
#include "GameModel.h"
#include "LoadDisplay.h"
#include "MissionLog.h"
#include "StringDB.h"
#include "DownloadableContent.h"
#include "LuaHelper.h"
#include "HUDManager.h"
#include "Battlefront2/Platform/Pc/pcSpawnDisplay.h"
#include "ConnectivityGraph.h"
#include "DustEffect.h"
#include "EntityFormation.h"
#include "EntityPath.h"
#include "EntityPathFollower.h"
#include "FishEffect.h"
#include "FlagItem.h"
#include "FlashyTextElement.h"
#include "FLEffect.h"
#include "ColorControl.h"
#include "FLGuiManager.h"
#include "FLInputManager.h"
#include "GamePath.h"
#include "HeroMessageDisplay.h"
#include "ItemTimerDisplay.h"
#include "LeafEffect.h"
#include "LuaMemory.h"
#include "NameDisplay.h"
#include "ScopeDisplay.h"
#include "PropGenerator.h"
#include "ReticuleDisplay.h"
#include "RumbleRegion.h"
#include "SkyObject.h"
#include "SoldierAnimatorLowRes.h"
#include "PauseMenu.h"
#include "SoldierAnimatorClass.h"
#include "StatusDisplay.h"
#include "Thread.h"
#include "RedEngineFL/Graphics/PC/pcRedTexture.h"
#include "RedEngineFL/Application/pc/pcFLRender.h"
#include "RedEngineFL/Application/RedPath.h"
#include "RedEngineFL/Interface/RedInterfaceScreen.h"

#include <yvals.h>
#include "RedEngineFL/Memory/RedMemory.h"
#ifdef OPENSWBF2_TEST
#include "FLEffect.h"
#include "GameObject.h"
#include "OrbitalStrike.h"
#include "ParticleEmitterObject.h"
#include "Timer.h"
#include "UnitController.h"
#endif
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "RedEngineFL/Memory/RedMemoryReport.h"
#include "SoundFL/source/sndbus.h"

#include "PebbleFL/Common/PblFile.h"

#include "PebbleFL/Common/PblRandom.h"
#include "RedEngineFL/Application/Renderable.h"
#include "RedEngineFL/Application/pc/pcRedDropShadow.h"
#include "RedEngineFL/Application/pc/pcRedParticle.h"
#include "RedEngineFL/Graphics/PC/pcRedAutoSettings.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Interface/RedFont.h"
#include "RedEngineFL/Interface/RedGlyphCacheElement.h"
#include "RedEngineFL/Util/RedWarning.h"
#include "RedEngineFL/Util/RedLog.h"
#include "AIVisRegion.h"
#include "Cloth.h"
#include "RedEngineFL/Util/ext/IAllocator.h"
#include "DamageRegion.h"
#include "EntitySoldier.h"

#include "Explosion.h"
#include "OrdnanceBeacon.h"
#include "OrdnanceBeam.h"
#include "OrdnanceBullet.h"
#include "OrdnanceEmitter.h"
#include "OrdnanceFatRay.h"
#include "OrdnanceGrapplingHook.h"
#include "OrdnanceGrenade.h"
#include "OrdnanceHaywire.h"
#include "OrdnanceLaser.h"
#include "OrdnanceMeleeThrow.h"
#include "OrdnanceMissile.h"
#include "OrdnanceShell.h"
#include "OrdnanceSticky.h"
#include "OrdnanceTowCable.h"
#include "WeaponAreaEffect.h"
#include "WeaponBinoculars.h"
#include "WeaponCannon.h"
#include "WeaponCatapult.h"
#include "WeaponDestruct.h"
#include "WeaponDetonator.h"
#include "WeaponDisguise.h"
#include "WeaponDispenser.h"
#include "WeaponGrapplingHook.h"
#include "WeaponGrenade.h"
#include "WeaponInvisibility.h"
#include "WeaponLaser.h"
#include "WeaponLauncher.h"
#include "WeaponMelee.h"
#include "WeaponMeleeThrow.h"
#include "WeaponRemote.h"
#include "WeaponRepair.h"
#include "WeaponShield.h"
#include "WeaponTowCable.h"

#include "EntityProp.h"
#include "EntityPropAnimated.h"
#include "EntityBuilding.h"
#include "EntityBuildingAnimated.h"
#include "EntityBuildingAnimatedArmed.h"
#include "EntityBuildingArmed.h"
#include "EntityBuildingArmedDynamic.h"
#include "EntityDoor.h"
#include "EntityTrap.h"
#include "EntityDroid.h"
#include "EntityWalker.h"
#include "EntityDroideka.h"
#include "EntityFlyer.h"
#include "EntityCarrier.h"
#include "EntityHover.h"
#include "EntityPortableTurret.h"
#include "EntityDefenseGridTurret.h"
#include "EntitySoundStatic.h"
#include "EntitySoundStream.h"
#include "CommandBuilding.h"
#include "CommandBuildingArmed.h"
#include "CommandBuildingAnimatedArmed.h"
#include "CommandHover.h"
#include "CommandWalker.h"
#include "CommandFlyer.h"
#include "VehicleSpawn.h"
#include "TargetManager.h"
#include "TeamManager.h"
#include "VehicleManager.h"
#include "PowerupStation.h"
#include "PowerupItem.h"
#include "EntityMine.h"
#include "GrassPatch.h"
#include "LeafPatch.h"
#include "Godray.h"
#include "EntityLight.h"
#include "EntityHologram.h"
#include "EntityEffect.h"
#include "Asteroid.h"
#include "EntityRemoteTerminal.h"
#include "ControllerManager.h"
#include "Damageable.h"
#include "FlyRegion.h"
#include "FoleyFXRegion.h"
#include "EntitySound.h"
#include "ParticleSystem.h"
#include "PatrolPath.h"
#include "PlayerProfile.h"
#include "ScreenTransition.h"
#include "TentacleSimulator.h"
#include "TriggerSoundRegion.h"

#include "RedEngineFL/Util/ext/types.h"

#include "LoadUtil.h"

#include "LuaCallbacks.h"

#include "AIUtil.h"

#include "FirstPerson.h"

#include "BirdEffect.h"
#include "Chunk.h"
#include "TriggerRegionManager.h"

#include "CameraManager.h"

#include "RedEngineFL/Application/pc/pcRedDebug.h"
#include "Steam/gfGalaxy.h"

#include "CommandPostManager.h"
#include "BootDisplay.h"
#include "BoundaryManager.h"
#include "CaptureDisplay.h"
#include "ChangeClassDisplay.h"
#include "CollisionMesh.h"
#include "LockOnManager.h"
#include "LuaScript.h"
#include "MessageDisplay.h"
#include "MuzzleFlash.h"
#include "ParticleEmitter.h"
#include "SpectatorDisplay.h"
#include "StatIconDisplay.h"
#include "Tracker.h"
#include "RedEngineFL/Interface/RedInterfaceElement.h"
#include "RedEngineFL/Util/RedLocalizeDB.h"
#include "RedEngineFL/Application/pc/pcRedShadowProxy.h"
#include "PebbleFL/Common/PblRegion.h"

#include "RedEngineFL/Application/RedTerrain.h"
#include "LuaEvent.h"
#include "RedEngineFL/Interface/RedIFScreen.h"

#include "RedEngineFL/Util/RedAnimation.h"
#include "RedEngineFL/Graphics/RedModel.h"

#include "SoundDebugDisplay.h"
#include "GameVoiceChat.h"
#include "GameVoiceChatNode.h"
#include "PebbleFL/Common/PblJournal.h"

namespace std
{
__declspec(noreturn) void __cdecl _Xbad_alloc();
__declspec(noreturn) void __cdecl _Xlength_error(const char*);
}

namespace DownloadableContent
{
bool IsDownloadableContent();
}

namespace LuaHelper
{
bool OpenScript(char const* pName);

bool CallGlobalProc(char const* pName);
}

namespace Lua_Callbacks
{
void InitMission();
}

namespace PropGenerator
{
    void Init();
}

namespace SkyManager
{
    void Init();
}

namespace FLRenderer
{
    void Init(bool bUberMode);
}

namespace RedInputManager
{
    void Init();
}

namespace RedInputManager
{
    void Cleanup();
}

namespace SaveGameManager
{
    void Init(bool bForceNoSaveDevice);
}

namespace StatsManager
{
    void Init();
}

class CustomContent
{
public:
    static void Create(bool bMission);
};

namespace MuzzleFlash
{
    bool Init();
}

namespace RedMissionLog
{
    void Init();
}
namespace RedProfile
{
    void Init();
}

class RedVertexCache
{
public:
    static void Init();
};
class RedFootprintManager : public RedVertexCache
{
public:
    static void Init();
};
 
 

// GLOBAL: SWBF2 0x007e68f1
bool gDrawFlyerShadows = true;

// GLOBAL: SWBF2 0x007e68f4
int WORLD_EXTENTS = 0x400;

// GLOBAL: SWBF2 0x007e5eef
bool CommandPost::sEnableVO = true;
// GLOBAL: SWBF2 0x007e651c
bool EntitySoldier::sEnableAnnouncementVO = true;
// GLOBAL: SWBF2 0x007e651d
bool EntitySoldier::sEnableMusic = true;

namespace FrameUpdate
{
    void Update();
}

namespace PblProfile
{
    void EndFrame();
}
namespace PblProfile
{
    void StartFrame();
}
namespace RedProfile
{
    void DisplayCountersByGroup();
}

namespace QOSDisplay
{
    void Reset();
}


namespace RedMissionLog
{
    void Reset();
}
namespace RedProfile
{
    void Cleanup();
}
namespace StatsManager
{
    void Cleanup();
}
namespace ReserveManager
{
    void Reset();
}
namespace SkyManager
{
    void Reset();
}

namespace SaveGameManager
{
    void Cleanup();
}
namespace GameVoiceChat
{
    unsigned int GetNumNodes();
}

extern int gNetSetup_errState;

// GLOBAL: SWBF2 0x1e563ac
GameState::State* _pNextState;
// GLOBAL: SWBF2 0x01e563b0
bool _bDoChangeState;
// GLOBAL: SWBF2 0x1e563b4
unsigned int mNextStateHash;
// GLOBAL: SWBF2 0x01e563b8
// mCurrentStateHash
unsigned int mCurrentStateHash;
// SYNTHETIC: SWBF2 0x00403e60
// `dynamic initializer for '_MissionState''
// SYNTHETIC: SWBF2 0x00768aa0
// `dynamic atexit destructor for '_MissionState''

// GLOBAL: SWBF2 0x007eb97c
// _MissionState
GameState::MissionState _MissionState;
// SYNTHETIC: SWBF2 0x00403e50
// `dynamic initializer for '_ShellState''
// SYNTHETIC: SWBF2 0x00768a90
// `dynamic atexit destructor for '_ShellState''

// GLOBAL: SWBF2 0x007eb998
// _ShellState
GameState::ShellState _ShellState;

// GLOBAL: SWBF2 0x01f9c2c0
// _pCurrentState
GameState::State* _pCurrentState;

// GLOBAL: SWBF2 0x01e563a8
// GameState::MissionState::_pCurrentState
GameState::MissionState::State* GameState::MissionState::_pCurrentState;

// GLOBAL: SWBF2 0x1e563b2
bool gDisableNetworkOnCleanup;

// GLOBAL: SWBF2 0x01e563b1
// sKioskMode
bool sKioskMode;

// GLOBAL: SWBF2 0x1e5639e
bool sbDidOneTimeInit;


namespace GameState
{
// SYNTHETIC: SWBF2 0x00403e20
// GameState::`dynamic initializer for '_TimeBase''

// GLOBAL: SWBF2 0x01ec5f40
// _TimeBase
PblTimeBase _TimeBase;
// SYNTHETIC: SWBF2 0x00403e40
// GameState::`dynamic initializer for '_TotalTimeBase''

// GLOBAL: SWBF2 0x01ec5f58
// _TotalTimeBase
PblTimeBase _TotalTimeBase;
// SYNTHETIC: SWBF2 0x00403e30
// GameState::`dynamic initializer for '_SystemTimeBase''

// GLOBAL: SWBF2 0x01ec5f90
// _SystemTimeBase
PblTimeBase _SystemTimeBase;
}

// GLOBAL: SWBF2 0x1ec5f70
PblTimeBase::Stamp SystemTimeStamp;
// GLOBAL: SWBF2 0x1ec5f80
PblTimeBase::Stamp AppTimeStamp;
// GLOBAL: SWBF2 0x1ec5fa8
PblTimeBase::Stamp TotalTimeStamp;
// GLOBAL: SWBF2 0x01f9c2c8
float sDeltaTime;
// GLOBAL: SWBF2 0x01f9c2cc
float sSystemDeltaTime;
// GLOBAL: SWBF2 0x01f9c2b8
LARGE_INTEGER perfCount;
// GLOBAL: SWBF2 0x01f9c2d0
LARGE_INTEGER oldPerfCount;
// GLOBAL: SWBF2 0x1e563a0
__int64 maxAdvanceTicks;
// GLOBAL: SWBF2 0x1e563c0
__int64 normalAdvanceTicks;
// GLOBAL: SWBF2 0x01e563c8
float g_fFrameRateScale;
// GLOBAL: SWBF2 0x01e5639f
// sIgnoreNextTime
bool sIgnoreNextTime;
// GLOBAL: SWBF2 0x01e563b3
bool gMemoryPoolErrors;

namespace GameState
{
    // GLOBAL: SWBF2 0x01f9c2c4
    int sNumFontsLoaded;
}

// GLOBAL: SWBF2 0x1f9c2d8
LARGE_INTEGER Frequency;

// GLOBAL: SWBF2 0x01f9c2a0
char* sTTYGrid;

// GLOBAL: SWBF2 0x01f9c2a4
int sTTYInvalidIndex;

// GLOBAL: SWBF2 0x01f9c2a8
int sTTYRows;

// GLOBAL: SWBF2 0x01f9c2ac
int sTTYColumns;

// FUNCTION: SWBF2 0x0052bfe0
void TTYInit()
{
    sTTYRows = 0x50;
    sTTYColumns = 0x50;
    sTTYInvalidIndex = -1;
    sTTYGrid = (char*)RedAllocFromHeap(__RedCurrHeap, 0xC800, 0);

    for (unsigned int i = 0; i < (unsigned int)sTTYColumns; ++i)
    {
        sTTYGrid[i * (sTTYRows << 3)] = 0x0A;
    }
}

// FUNCTION: SWBF2 0x0053a9e0
void GameState::OneTimeInit()
{
    if (sbDidOneTimeInit)
    {
        return;
    }

    sbDidOneTimeInit = true;
    TotalTimeStamp.Mark(&_TotalTimeBase);

    PlayerPrefs::InitAll();
    NetGame::Init();
    RedInputManager::Init();
    RedRenderer::InitStatic();

    PblPerformanceTimer::Init();

    RedRenderer::OpenDynamic();
    RedRenderer::SetTimeBase(&_TimeBase);

    if (!GameSoundEngine::Init(0, true))
    {
        LOG(LogAudio, Fatal, "Could not initialize sound engine!");
        RedWarning::DialogBoxMessage("Could not initialize sound engine!");
        pcCrash::Exit(1);
    }

    TTYInit();

    QueryPerformanceFrequency(&Frequency);
    maxAdvanceTicks = Frequency.QuadPart;
    normalAdvanceTicks = Frequency.QuadPart >> 4;

    CloseJournal();
}

// FUNCTION: SWBF2 0x0053aaf0
void GameState::OneTimeCleanup()
{
    if (!sbDidOneTimeInit)
    {
        return;
    }

    sbDidOneTimeInit = false;
    GameSoundEngine::Term(true);
    RedRenderer::CleanupStatic();
    RedInputManager::Cleanup();
}

// FUNCTION: SWBF2 0x0053ab20
void GameState::UpdateTime()
{
    if (g_bNoRender && g_fFixedRate <= 0.0f)
    {
        Sleep(1);
    }

    if (netInShell ? netEnabled : netEnabledNext)
    {
        NetGame::FrameLock();
    }

    __int64 iDeltaTicks = 0;

    if (!(g_fFixedRate > 0.0f))
    {
        FrameLimiter::FrameOps ops;
        FrameLimiter::FramePlan plan;
        static bool sbCapSuppressed = false;
        bool bThrottled = RedRenderer::pcPresentThrottles();
        bool bSuppressed = bThrottled && DisplayCfg::gDisplay.uiFps != 0;
        if (bSuppressed != sbCapSuppressed)
        {
            sbCapSuppressed = bSuppressed;
            if (bSuppressed)
            {
                LOG(LogRender, Inform, "OPENSWBF2 PACING vsync on, fps cap %u not applied",
                    DisplayCfg::gDisplay.uiFps);
            }
            else
            {
                LOG(LogRender, Inform, "OPENSWBF2 PACING fps cap %u applied",
                    DisplayCfg::gDisplay.uiFps);
            }
        }
        FrameLimiter::BindReal(ops, plan, DisplayLogic::EffectiveFpsCap(DisplayCfg::gDisplay.uiFps, bThrottled));
        __int64 iNowTicks = 0;
        iDeltaTicks = FrameLimiter::Pace(ops, plan, oldPerfCount.QuadPart, iNowTicks);
        perfCount.QuadPart = iNowTicks;

        if (iDeltaTicks > maxAdvanceTicks)
        {
            iDeltaTicks = normalAdvanceTicks;
        }

        if (g_fFrameRateScale > 0.0f)
        {
            iDeltaTicks = (__int64)((float)iDeltaTicks * g_fFrameRateScale + 0.5f);
            if (iDeltaTicks < 1)
            {
                iDeltaTicks = 1;
            }
        }

        if (!GameLoop::sPauseMode)
        {
            _TimeBase.AdvanceTicks(iDeltaTicks);
        }
        _SystemTimeBase.AdvanceTicks(iDeltaTicks);

        oldPerfCount = perfCount;
    }
    else
    {
        float fDeltaTime = 1.0f / g_fFixedRate;
        if (g_fFrameRateScale > 0.0f)
        {
            fDeltaTime = g_fFrameRateScale * fDeltaTime;
        }

        if (!GameLoop::sPauseMode)
        {
            _TimeBase.AdvanceTime(fDeltaTime);
        }
        _SystemTimeBase.AdvanceTime(fDeltaTime);
    }

    if (!sIgnoreNextTime)
    {
        _TotalTimeBase.AdvanceTicks(iDeltaTicks);
    }
    sIgnoreNextTime = false;

    sDeltaTime = AppTimeStamp.GetElapsed();
    sSystemDeltaTime = SystemTimeStamp.GetElapsed();

    float fMaxDeltaTime = (netInShell ? netEnabled : netEnabledNext) ? 1.0f : 0.1f;
    if (sDeltaTime > fMaxDeltaTime)
    {
        sDeltaTime = fMaxDeltaTime;
    }
    if (sSystemDeltaTime > fMaxDeltaTime)
    {
        sSystemDeltaTime = fMaxDeltaTime;
    }

    AppTimeStamp.Mark();
    SystemTimeStamp.Mark();
}

// FUNCTION: SWBF2 0x0053ae10
void GameState::SetState(unsigned int uiState)
{
    _bDoChangeState = true;
    switch (uiState)
    {
    case 0x8FF60339:
        _pNextState = &_MissionState;
        mNextStateHash = 0x8FF60339;
        break;
    case 0x11E1FC01:
        _pNextState = &_ShellState;
        mNextStateHash = 0x11E1FC01;
        break;
    case 0:
        _pNextState = 0;
        mNextStateHash = 0;
        break;
    default:
        _bDoChangeState = false;
        break;
    }
}

// FUNCTION: SWBF2 0x0053ae80
unsigned int GameState::GetState()
{
    return mCurrentStateHash;
}

// FUNCTION: SWBF2 0x0053ae90
bool GameState::Update()
{
    pcCrash::RunCrashTest();
    PlayerProfile::PollApplyNotice();
    if (_bDoChangeState)
    {

        _SetState(_pNextState);
        _bDoChangeState = false;
        _pNextState = 0;
    }

    if (_pCurrentState)
    {
        PblProfile::StartFrame();
        RedLog::NextFrame();
        FrameUpdate::Update();
        UpdateTime();
        _pCurrentState->Update(sDeltaTime, sSystemDeltaTime);
        PblProfile::EndFrame();
        return true;
    }
    return false;
}

// FUNCTION: SWBF2 0x0053adc0
void GameState::_SetState(State* pState)
{
    State* pOld = _pCurrentState;
    if (pOld)
    {
        pOld->Exit();
    }

    _pCurrentState = pState;
    mCurrentStateHash = mNextStateHash;
    mNextStateHash = 0;

    if (pState)
    {
        pState->Enter();
    }
}

namespace SaveGameManager
{
    // GLOBAL: SWBF2 0x01eaf000
    unsigned int mReqSaveFile2Data;
}

namespace SaveGameManager
{
    extern void (*mOnDone)();
}
namespace SaveGameManager
{
    // GLOBAL: SWBF2 0x01eaf008
    unsigned int mReqSaveFile2Size;
}
namespace SaveGameManager
{
    // GLOBAL: SWBF2 0x01eaf010
    unsigned int mReqSaveFile1Data;
}
namespace SaveGameManager
{
    // GLOBAL: SWBF2 0x01eaf014
    unsigned int mReqSaveFile1Size;
}
namespace SaveGameManager
{
    // GLOBAL: SWBF2 0x01eaeff7
    bool mForceNoSaveDevice;
}
namespace PblBuildType
{
    // GLOBAL: SWBF2 0x009cef72
    bool gCreateMemoryProfile;
}

namespace SaveGameManager
{

// FUNCTION: SWBF2 0x00632ee0
void Init(bool bForceNoSaveDevice)
{
    SaveGameManager::mOnDone = 0;
    SaveGameManager::mReqSaveFile2Data = 0;
    SaveGameManager::mReqSaveFile1Data = 0;
    SaveGameManager::mReqSaveFile2Size = 0;
    SaveGameManager::mReqSaveFile1Size = 0;
    SaveGameManager::mForceNoSaveDevice = bForceNoSaveDevice;
}
}

// FUNCTION: SWBF2 0x005396b0
void GameState::CreateBaseExplosionClasses()
{
    new ExplosionClass(PblHash("explosion"));
}

// FUNCTION: SWBF2 0x00539710
void GameState::CreateBaseOrdnanceClasses()
{
    new OrdnanceLaserClass(PblHash("bolt"));
    new OrdnanceBeaconClass(PblHash("beacon"));
    new OrdnanceBeamClass(PblHash("beam"));
    new OrdnanceBulletClass(PblHash("bullet"));
    new OrdnanceEmitterClass(PblHash("emitterordnance"));
    new OrdnanceFatRayClass(PblHash("fatray"));
    new OrdnanceGrapplingHookClass(PblHash("grapplinghook"));
    new OrdnanceGrenadeClass(PblHash("grenade"));
    new OrdnanceHaywireClass(PblHash("haywire"));
    new OrdnanceLaserClass(PblHash("laser"));
    new OrdnanceMeleeThrowClass(PblHash("meleethrowordnance"));
    new OrdnanceMissileClass(PblHash("missile"));
    new OrdnanceShellClass(PblHash("shell"));
    new OrdnanceStickyClass(PblHash("sticky"));
    new OrdnanceTowCableClass(PblHash("towcable"));
}

// FUNCTION: SWBF2 0x00539aa0
void GameState::CreateBaseWeaponClasses()
{
    new WeaponCannonClass(PblHash("weapon"));
    new WeaponAreaEffectClass(PblHash("areaeffectweapon"));
    new WeaponBinocularsClass(PblHash("binoculars"));
    new WeaponCannonClass(PblHash("cannon"));
    new WeaponCatapultClass(PblHash("catapult"));
    new WeaponDestructClass(PblHash("destruct"));
    new WeaponDetonatorClass(PblHash("detonator"));
    new WeaponDisguiseClass(PblHash("disguise"));
    new WeaponDispenserClass(PblHash("dispenser"));
    new WeaponGrapplingHookClass(PblHash("grapplinghookweapon"));
    new WeaponGrenadeClass(PblHash("grenade"));
    new WeaponInvisibilityClass(PblHash("invisibility"));
    new WeaponLaserClass(PblHash("laser"));
    new WeaponLauncherClass(PblHash("launcher"));
    new WeaponRepairClass(PblHash("repair"));
    new WeaponMeleeClass(PblHash("melee"));
    new WeaponMeleeThrowClass(PblHash("meleethrow"));
    new WeaponRemoteClass(PblHash("remote"));
    new WeaponShieldClass(PblHash("shield"));
    new WeaponTowCableClass(PblHash("towcableweapon"));
}

// FUNCTION: SWBF2 0x00539f60
void GameState::CreateBaseEntityClasses()
{
    new EntityPropClass(PblHash("prop"));
    new EntityPropClass(PblHash("building"));
    new EntityPropAnimatedClass(PblHash("animatedprop"));
    new EntityBuildingClass(PblHash("destructablebuilding"));
    new EntityBuildingAnimatedClass(PblHash("animatedbuilding"));
    new EntityBuildingAnimatedArmedClass(PblHash("armedanimatedbuilding"));
    new EntityBuildingArmedClass(PblHash("armedbuilding"));
    new EntityBuildingArmedDynamicClass(PblHash("armedbuildingdynamic"));
    new EntityDoorClass(PblHash("door"));
    new EntityTrapClass(PblHash("trap"));
    new EntitySoldierClass(PblHash("soldier"));
    new EntityDroidClass(PblHash("droid"));
    new EntityWalkerClass(PblHash("walker"));
    new EntityDroidekaClass(PblHash("walkerdroid"));
    new EntityFlyerClass(PblHash("flyer"));
    new EntityCarrierClass(PblHash("carrier"));
    new EntityHoverClass(PblHash("hover"));
    new EntityHoverClass(PblHash("treaded"));
    new EntityPortableTurretClass(PblHash("portableturret"));
    new EntityDefenseGridTurretClass(PblHash("defensegridturret"));
    new EntitySoundStaticClass(PblHash("SoundAmbienceStatic"));
    new EntitySoundStreamClass(PblHash("SoundAmbienceStreaming"));
    new CommandBuildingClass(PblHash("commandpost"));
    new CommandBuildingArmedClass(PblHash("commandarmedbuilding"));
    new CommandBuildingAnimatedArmedClass(PblHash("commandarmedanimatedbuilding"));
    new CommandHoverClass(PblHash("commandhover"));
    new CommandWalkerClass(PblHash("commandwalker"));
    new CommandFlyerClass(PblHash("commandflyer"));
    new VehicleSpawnClass(PblHash("vehiclespawn"), false);
    new VehicleSpawnClass(PblHash("vehiclepad"), true);
    new PowerupStationClass(PblHash("powerupstation"));
    new PowerupItemClass(PblHash("powerupitem"));
    new EntityMineClass(PblHash("mine"));
    new DustEffectClass(PblHash("dusteffect"));
    new CloudClusterClass(PblHash("cloudcluster"));
    new GrassPatchClass(PblHash("grasspatch"));
    new LeafPatchClass(PblHash("leafpatch"));
    new GodrayClass(0xBA5B0537u);
    new EntityLightClass(PblHash("light"));
    new EntityHologramClass(PblHash("hologram"));
    new RumbleEffectClass(PblHash("rumbleeffect"));
    new EntityEffectClass(PblHash("effectprop"));
    new AsteroidClass(PblHash("asteroid"));
    new EntityClothClass(PblHash("cloth"));
    new EntityRemoteTerminalClass(PblHash("remoteterminal"));
    new FlagItemClass(PblHash("flag"));
}

// FUNCTION: SWBF2 0x0053af10
void GameState::PreStateInit(bool bMission)
{
    GameMemory::BuildHeaps(!bMission);
    RedSetCurrentHeap(GameMemory::ApplicationHeap);
    OneTimeInit();

    if (!sKioskMode)
    {
        sIgnoreNextTime = true;
    }

    gDrawFlyerShadows = true;
    PblRandom::SetGlobalSeed(gSeedSet ? gSeed : 0);

    RedMissionLog::Init();
    MissionLog::Init();

    GameState::sNumFontsLoaded = 0;
    RedSetCurrentHeap(GameMemory::RunTimeHeap);

    PlayerProfile::tProfile* pProfile = PlayerProfile::GetCurProfile();
    if (pProfile)
    {
        pProfile->mPCOptions.ApplyPreLoadSettings();
    }

    DownloadableContent::SetCurrentMission(GameLoop::mMissionScriptHash);
    LuaHelper::InitState();
    RedProfile::Init();
    HUD::Manager::Open();
    Lua_Callbacks::Init();
    StringDB::Init();

    if (bMission)
    {
        unsigned int uiMissionHash = GameLoop::mMissionScriptHash;
        if (!LoadUtil::ReadDataFile("MISSION.lvl", 0, 0, &uiMissionHash, 1, 0))
        {
            RedTexture::Init();
            RedRenderer::Init();
            LOG(LogLoad, Fatal, "Could not open mission.lvl");
            RedWarning::DialogBoxMessage("Could not open mission.lvl");
            pcCrash::Exit(1);
        }

        bool bOpened = LuaHelper::OpenScript(GameLoop::mMissionScript);
        if (!bOpened)
        {
            char acName[256] = { 0 };
            _snprintf(acName, sizeof(acName), "MISSION\\%s.lvl", GameLoop::mMissionScript);
            if (LoadUtil::ReadDataFile(acName, 0, 0, 0, 0, 0))
            {
                bOpened = LuaHelper::OpenScript(GameLoop::mMissionScript);
            }
            else
            {
                LOG(LogLoad, Fatal, "Could not open %s", acName);
                RedWarning::DialogBoxMessage("Could not open %s", acName);
                pcCrash::Exit(1);
            }
        }

        WORLD_EXTENTS = 0x400;

        if (bOpened)
        {
            Lua_Callbacks::InitMission();
            LuaHelper::CallProc("ScriptPreInit");
        }
    }

    RedTexture::Init();
    RedRenderer::Init();

    if (pProfile && !pProfile->mInitialized)
    {
        pProfile->mPCOptions.renderOptions = RedAutoSettings::GetRenderOptions();
        pProfile->mPCOptions.mResolutionX = (unsigned short)pProfile->mPCOptions.renderOptions.defaultWidth;
        pProfile->mPCOptions.mResolutionY = (unsigned short)pProfile->mPCOptions.renderOptions.defaultHeight;
    }

    GameSoundEngine::Create();
    CommandPost::sEnableVO = true;
    EntitySoldier::sEnableMusic = true;
    EntitySoldier::sEnableAnnouncementVO = true;
    FLInputManager::InitCustomFontData();
    RedFont::Init();
    RedGlyphCacheElement::InitPool(0x1770);

    if (!bMission)
    {
        GameMovie::Create(GameMemory::RunTimeHeap);
    }

    SaveGameManager::Init(PblBuildType::gCreateMemoryProfile);
    RedRenderer::SetFogEnable(true);
    RedRenderer::SetSky(1);
    RedVertexCache::Init();
    RedParticleRenderer::Init();
    RedFootprintManager::Init();
    RedDropShadowManager::Init();
    FLInputManager::Init();
    GuiManager::InitAll();
    ScreenTransition::Create();
    StatsManager::Init();
    ParticleSystem::Init();
    ClothData::Init();
    TentacleSimulator::Init();
    AcklayData::Init();
    CreateBaseExplosionClasses();
    CreateBaseOrdnanceClasses();
    CreateBaseWeaponClasses();
    CreateBaseEntityClasses();
    WeaponMeleeClass::Init();

    new SoundStaticRegionFactory();
    new SoundStreamRegionFactory();
    new SoundSpaceRegionFactory();
    new TriggerSoundRegionFactory();
    new FoleyFXRegionFactory();
    new FlyRegionManager();
    new RumbleRegionManager();
    new DamageRegionFactory();
    new AIVisRegionManager();
    new EntityFormationFactory();
    new EntityPathFactory();
    new EntityPath::BranchRegionFactory();
    new AI::PatrolPathFactory();

    ControllerManager::Reset();
    NetGame::Create(bMission);
    CustomContent::Create(bMission);
    JournalStartTimes();
    PblRandom::SetGlobalSeed(gSeedSet ? gSeed : (unsigned int)_time64(0));

    if (bMission)
    {
        PblVector3 vMax((float)WORLD_EXTENTS, 0.0f, (float)WORLD_EXTENTS);
        PblVector3 vMin((float)-WORLD_EXTENTS, 0.0f, (float)-WORLD_EXTENTS);
        CollisionManager::Create(vMin, vMax);

        ChunkClusterManager::Create();
        TriggerRegionManager::Create();
        DamageRegion::Create();
        CommandPost::Create();
        MuzzleFlash::Init();
        LeafParticleManager::Create();
        BirdParticleManager::Create();
        FishParticleManager::Create();
        ControllerManager::Init();
        AIUtil::Init();
        ConnectivityGraphManager::Init();
    }

    Damageable::StaticInit();
    NetGame::EnterClient();
    FlashyTextManager::Init(bMission);

    if (bMission)
    {
        PauseMenu::Init();
        PropGenerator::Init();
    }

    SkyManager::Init();
    NetGame::LeaveClient();
    NetGame::EnterClient();
    FLRenderer::Init(AIUtil::IsUberMode());
    CameraManager::Create(GameLoop::GetNumCameras());

    FLEffect::InitAll(bMission);
    if (ColorControl::s_pInstance)
    {
        ColorControl::s_pInstance->SetLevel(COLORCONTROL_GAMMA_CONTRAST,
                                            RedRenderer::pcGetRenderOptions().mGammaContrast);
        ColorControl::s_pInstance->SetLevel(COLORCONTROL_GAMMA_BRIGHTNESS,
                                            RedRenderer::pcGetRenderOptions().mGammaBrightness);
        ColorControl::s_pInstance->Update(0.0f);
    }
    NetGame::LeaveClient();
    CommandPostManager::Create();

    RedDebug::Init();
    FirstPerson::PreInit();

    if (bMission && DownloadableContent::IsDownloadableContent())
    {
        RedLocalizeDB::SetSkipCount(1);
    }

    LoadUtil::ReadDataFile("core.lvl", 0, 0, 0, 0, 0);

    if (bMission && DownloadableContent::IsDownloadableContent())
    {
        PblFile file;
        char acFullName[300];
        LoadUtil::MakeFullName("core.lvl", LoadUtil::FILE_TYPE_NONE, acFullName,
                               sizeof(acFullName), true);
        if (file.Open(acFullName, false, false, true))
        {
            LoadUtil::ReadDataFile(file, 0, 0, 0, 0, 0);
            file.Close();
        }
    }

    LoadUtil::ReadDataFile("common.lvl", 0, 0, 0, 0, 0);
    RedFont::PostCommonInit();
    GameSoundEngine::InitializeBusGains();
    gSoundEnabled = true;
    LoadDisplay::Create();
    LoadDisplay::sInstance->Begin();
    GameSound::InitDeferred();
    NetGame::EnterClient();
    NetGame::LeaveClient();
    if (bMission)
    {
        RedRenderer::SetFogEnable(true);
    }
    else
    {
        RedRenderer::SetFogEnable(false);
    }
}

// FUNCTION: SWBF2 0x005391b0
GamePath* AI::PatrolPathFactory::CreatePath() const
{
    return new PatrolPath();
}

// FUNCTION: SWBF2 0x00539270
GamePath* EntityPathFactory::CreatePath() const
{
    return new EntityPath();
}

// FUNCTION: SWBF2 0x00539330
void* SoundStaticRegionFactory::CreateRegion(RedRegionDesc& desc, const char* pcName) const
{
    EntitySoundStaticClass soundClass(0xEDE52A96);
    return EntitySoundClass::CreateSoundFromRegion(desc, pcName, GetPrefix(), soundClass);
}

// FUNCTION: SWBF2 0x005393d0
void* SoundStreamRegionFactory::CreateRegion(RedRegionDesc& desc, const char* pcName) const
{
    EntitySoundStreamClass soundClass(0x93D6C380);
    return EntitySoundClass::CreateSoundFromRegion(desc, pcName, GetPrefix(), soundClass);
}

// FUNCTION: SWBF2 0x00539470
bool IsNetObjectFeatureEnabled()
{
    return false;
}

// FUNCTION: SWBF2 0x00539480
GamePath* EntityFormationFactory::CreatePath() const
{
    return new EntityFormation();
}

// FUNCTION: SWBF2 0x00539510
void GameState::State::Enter()
{
}

// FUNCTION: SWBF2 0x00539520
void GameState::State::Update(float, float)
{
}

// FUNCTION: SWBF2 0x00539530
void GameState::State::Exit()
{
}

// FUNCTION: SWBF2 0x005395a0
void GameState::MissionState::State::Enter()
{
}

// FUNCTION: SWBF2 0x005395b0
void GameState::MissionState::State::Update(float, float)
{
}

// FUNCTION: SWBF2 0x005395c0
void GameState::MissionState::State::Exit()
{
}

// FUNCTION: SWBF2 0x005395d0
void GameState::MissionState::PlayState::State::Enter()
{
}

// FUNCTION: SWBF2 0x005395e0
void GameState::MissionState::PlayState::State::Update(float, float)
{
}

// FUNCTION: SWBF2 0x005395f0
void GameState::MissionState::PlayState::State::Exit()
{
}

// FUNCTION: SWBF2 0x0053ba90
void GameState::MissionState::SetState(unsigned int uiState)
{
    switch (uiState)
    {
    case 0:
        _SetState(0);
        break;
    case 0xc2cbd863:
        _SetState(&_PlayState);
        break;
    }
}

// FUNCTION: SWBF2 0x0053bab0
void GameState::MissionState::_SetState(State* pState)
{
    if (pState == _pCurrentState)
    {
        return;
    }
    if (_pCurrentState)
    {
        _pCurrentState->Exit();
    }
    _pCurrentState = pState;
    if (pState)
    {
        pState->Enter();
    }
}

// FUNCTION: SWBF2 0x0053bcd0
void GameState::MissionState::PlayState::Enter()
{
}

// FUNCTION: SWBF2 0x0053bcf0
void GameState::MissionState::PlayState::Update(float fDeltaTime, float fSystemDeltaTime)
{
    GameLoop::Update(fDeltaTime, fSystemDeltaTime);
}

// FUNCTION: SWBF2 0x0053bce0
void GameState::MissionState::PlayState::Exit()
{
}

// FUNCTION: SWBF2 0x0053bd10
void GameState::MissionState::PlayState::NormalState::Enter()
{
}

// FUNCTION: SWBF2 0x0053bd30
void GameState::MissionState::PlayState::NormalState::Update(float, float)
{
}

// FUNCTION: SWBF2 0x0053bd20
void GameState::MissionState::PlayState::NormalState::Exit()
{
}

// FUNCTION: SWBF2 0x0053bd40
void GameState::MissionState::PlayState::PausedState::Enter()
{
}

// FUNCTION: SWBF2 0x0053bd60
void GameState::MissionState::PlayState::PausedState::Update(float, float)
{
}

// FUNCTION: SWBF2 0x0053bd50
void GameState::MissionState::PlayState::PausedState::Exit()
{
}

// FUNCTION: SWBF2 0x00539690
void GameState::SetIgnoreNextUpdate(bool bIgnore)
{
    sIgnoreNextTime = bIgnore;
}

// FUNCTION: SWBF2 0x005396a0
bool GameState::GetKioskMode()
{
    return sKioskMode;
}

// FUNCTION: SWBF2 0x0053b980
void GameState::ShellState::Enter()
{
    PreStateInit(false);
    ShellLoop::Init();
    GameSound::SetupDeferred();

    Snd::Bus* pBus = Snd::Bus::FindByHashID(0x985C8F54);
    if (pBus)
    {
        pBus->Fade(0.0f, 1.0f, -1.0f);
        Snd::Bus::UpdateAll(0.1f);
    }

    LoadDisplay::Instance()->End();
    ShellLoop::PostLoad();

    PlayerProfile::tProfile* pProfile = PlayerProfile::GetCurProfile();
    if (pProfile)
    {
        pProfile->mPCOptions.ApplyPostLoadSettings();
    }

    RedProfile::DisplayCountersByGroup();

    if (gMemoryPoolErrors == true)
    {
        MemoryPool::PoolErrorPostLoad();
    }

    AppTimeStamp.Mark(&_TimeBase);
    SystemTimeStamp.Mark(&_SystemTimeBase);
}

// FUNCTION: SWBF2 0x0053ba70
void GameState::ShellState::Update(float fDeltaTime, float)
{
#ifdef OPENSWBF2_TEST
    if (GameLoop::TakeRepeatPending())
    {
        LOG(LogSim, Inform, "OPENSWBF2 REPEAT to_mission mission=%s", GameLoop::mMissionScript);
        GameState::SetState(0x8FF60339);
    }
    else if (GameLoop::mMissionScript[0] && pcCrash::TakeShellFirst())
    {
        LOG(LogSim, Inform, "OPENSWBF2 SHELLFIRST to_mission mission=%s", GameLoop::mMissionScript);
        GameState::SetState(0x8FF60339);
    }
#endif
    ShellLoop::Update(fDeltaTime);
}

// FUNCTION: SWBF2 0x0053ba50
void GameState::ShellState::Exit()
{
    RedProfile::DisplayCountersByGroup();
    ShellLoop::Cleanup();
    PostStateCleanup(false);
}

#ifdef OPENSWBF2_TEST
extern double gSoundClockTime;
#endif

extern bool gThreadIsEven;
extern bool sHostThreadIsEven;
extern bool sClientThreadIsEven;

static bool sbMissionEntered;
static unsigned int suMissionEntrySimSeed;
static unsigned int suMissionEntryHandleId;
static bool sbMissionEntryThreadIsEven;
static bool sbMissionEntryHostThreadIsEven;
static bool sbMissionEntryClientThreadIsEven;
static int siMissionEntryLuaGCCount;
static int siMissionEntryHasNormal;
static float sfMissionEntryNormal;

static void RestoreMissionEntryState()
{
    if (!sbMissionEntered)
    {
        sbMissionEntered = true;
        suMissionEntrySimSeed = PblRandom::_Sim.GetSeed();
        suMissionEntryHandleId = PblHandled::GetNextHandleId();
        sbMissionEntryThreadIsEven = gThreadIsEven;
        sbMissionEntryHostThreadIsEven = sHostThreadIsEven;
        sbMissionEntryClientThreadIsEven = sClientThreadIsEven;
        siMissionEntryLuaGCCount = Lua_Callbacks::GetPeriodicLuaGCCount();
        PblRandom::GetNormalCache(&siMissionEntryHasNormal, &sfMissionEntryNormal);
        GameSoundEngine::SnapshotClock();
        return;
    }
    PblRandom::_Sim.SetSeed(suMissionEntrySimSeed);
    PblHandled::SetNextHandleId(suMissionEntryHandleId);
    gThreadIsEven = sbMissionEntryThreadIsEven;
    sHostThreadIsEven = sbMissionEntryHostThreadIsEven;
    sClientThreadIsEven = sbMissionEntryClientThreadIsEven;
    Lua_Callbacks::SetPeriodicLuaGCCount(siMissionEntryLuaGCCount);
    PblRandom::SetNormalCache(siMissionEntryHasNormal, sfMissionEntryNormal);
    GameSoundEngine::RestoreClock();
}

// FUNCTION: SWBF2 0x0053bae0
void GameState::MissionState::Enter()
{
    RestoreMissionEntryState();
#ifdef OPENSWBF2_TEST
    LOG(LogSim, Inform, "OPENSWBF2 MISSIONENTER rand=%u/%u snd=%.3f handle=%u even=%d luagc=%d",
        PblRandom::_Global._uiSeed, PblRandom::_Sim._uiSeed, gSoundClockTime, PblHandled::GetNextHandleId(),
        (int)gThreadIsEven, Lua_Callbacks::GetPeriodicLuaGCCount());
#endif
    gDisableNetworkOnCleanup = false;
    PreStateInit(true);
    GameLoop::Init();
    GameLoop::PostLoad();

    if (gMemoryPoolErrors == true)
    {
        MemoryPool::PoolErrorPostLoad();
    }

    unsigned int uiCount = CameraManager::sInstance->m_iNumCam;
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        int iIndex = FLInputManager::s_instance.GetHWPort(i);
        if (iIndex >= 0)
        {
            PlayerProfile::tProfile* pProfile = PlayerProfile::GetProfile(iIndex);
            if (pProfile)
            {
                pProfile->mOnlineOptions.ApplySettings();
            }
        }
    }

    PlayerProfile::tProfile* pProfile = PlayerProfile::GetCurProfile();
    if (pProfile)
    {
        pProfile->mPCOptions.ApplyPostLoadSettings();
    }

    AppTimeStamp.Mark(&_TimeBase);
    SystemTimeStamp.Mark(&_SystemTimeBase);

    RedProfile::DisplayCountersByGroup();
    CameraManager::sInstance->EnableScene(true);
    MissionState::SetState(0xC2CBD863);

    int iFree = RedGetHeapFree(GameMemory::RunTimeHeap) - 0x20800;
    if (iFree > 0x8000)
    {
        int iLow = util::Min_(0x20000, iFree);
        int iHigh = util::Min_((iFree - iLow) / 2 + iLow, 0x40000);
        EntityCloth::SetMemUseage(iLow, iHigh);
        char acLine[128];
        RedMemoryReport::FormatCloth(iFree, true, iLow, iHigh, acLine, sizeof(acLine));
        LOG(LogMemory, Inform, "%s", acLine);
        EntityCloth::SetEnable(true);
    }
    else
    {
        char acLine[128];
        RedMemoryReport::FormatCloth(iFree, false, 0, 0, acLine, sizeof(acLine));
        LOG(LogMemory, Inform, "%s", acLine);
        EntityCloth::SetEnable(false);
    }

    Snd::Bus* pBus = Snd::Bus::FindByHashID(0x985C8F54);
    if (pBus)
    {
        pBus->Fade(2.0f, 1.0f, -1.0f);
        Snd::Bus::UpdateAll(0.1f);
    }
}

// FUNCTION: SWBF2 0x0053bca0
void GameState::MissionState::Update(float fDeltaTime, float fSystemDeltaTime)
{
    if (_pCurrentState)
    {
        _pCurrentState->Update(fDeltaTime, fSystemDeltaTime);
    }
}

// FUNCTION: SWBF2 0x0053bc60
void GameState::MissionState::Exit()
{
    SetState(0);
    CameraManager::sInstance->EnableScene(false);
    RedProfile::DisplayCountersByGroup();
    GameLoop::Cleanup();
    PostStateCleanup(true);

    if (gDisableNetworkOnCleanup)
    {
        netEnabledNext = false;
        gDisableNetworkOnCleanup = false;
    }
}

// FUNCTION: SWBF2 0x0053be00
void GameState::JournalStartTimes()
{
    _TimeBase.PblJournal_Record();
    _SystemTimeBase.PblJournal_Record();
}

// TEMPLATE: SWBF2 0x0053be20
// PblList<LuaEvent>::Clear

// TEMPLATE: SWBF2 0x0053be60
// PblList<IFScreen>::Clear

// TEMPLATE: SWBF2 0x0053beb0
// PblList<Factory<Entity,EntityClass,EntityDesc> >::Clear

// TEMPLATE: SWBF2 0x0053bf00
// PblList<Factory<Ordnance,OrdnanceClass,OrdnanceDesc> >::Clear

// TEMPLATE: SWBF2 0x0053bf50
// PblList<Factory<Weapon,WeaponClass,WeaponDesc> >::Clear

// TEMPLATE: SWBF2 0x0053bfa0
// PblList<Factory<Ordnance,ExplosionClass,OrdnanceDesc> >::Clear

#ifdef OPENSWBF2_TEST
extern char* g_Heap3Start;
extern char* gArenaEnd;

template <class T>
static unsigned int HandleIdOffset()
{
    T* pObject = (T*)0x10000;
    return (unsigned int)static_cast<PblHandled*>(pObject) - (unsigned int)pObject + PblHandled::HandleIdOffset();
}

static const unsigned int kHandleTypes = 6;
static unsigned int sauHandleIdOffset[kHandleTypes];

struct HandleScan
{
    unsigned int uLo;
    unsigned int uHi;
    unsigned int uDeadVtable;
    unsigned int uNext;
    unsigned int uLive;
    unsigned int uDead;
};

static PBL_ASAN_NO_SANITIZE void ScanHandleRange(HandleScan& rScan, const unsigned int* puBase, const unsigned int* puFrom,
                                                 const unsigned int* puTo, const char* pcWhere)
{
    for (const unsigned int* pu = puFrom; pu + 1 < puTo; ++pu)
    {
        const unsigned int uObject = pu[0];
        const unsigned int uId = pu[1];
        if (uObject < rScan.uLo || uObject >= rScan.uHi || (uObject & 3) != 0 || uId == 0 || uId >= rScan.uNext)
        {
            continue;
        }
        for (unsigned int i = 0; i < kHandleTypes; ++i)
        {
            const unsigned int uAt = uObject + sauHandleIdOffset[i];
            if (uAt < rScan.uLo + 4 || uAt + 4 > rScan.uHi)
            {
                continue;
            }
            const unsigned int* puHandled = (const unsigned int*)uAt;
            const unsigned int uKey = uAt - PblHandled::HandleIdOffset();
            const bool bLive = PblHandleRegistry::IsLive((const PblHandled*)uKey, uId);
            const bool bDead = !bLive && puHandled[0] == 0 && puHandled[-1] == rScan.uDeadVtable;
            if (!bLive && !bDead)
            {
                continue;
            }
            if (rScan.uLive + rScan.uDead < 16)
            {
                LOG(LogSim, Warning, "OPENSWBF2 HANDLESCAN %s at=%s+0x%X object=0x%08X id=%u", bLive ? "live" : "dead",
                    pcWhere, (unsigned int)(pu - puBase) * 4, uObject, uId);
            }
            if (bLive)
            {
                ++rScan.uLive;
            }
            else
            {
                ++rScan.uDead;
            }
            break;
        }
    }
}

static void ScanStaleHandles(bool bMission)
{
    sauHandleIdOffset[0] = HandleIdOffset<GameObject>();
    sauHandleIdOffset[1] = HandleIdOffset<FLEffectObject>();
    sauHandleIdOffset[2] = HandleIdOffset<ParticleEmitterObject>();
    sauHandleIdOffset[3] = HandleIdOffset<UnitController>();
    sauHandleIdOffset[4] = HandleIdOffset<Timer>();
    sauHandleIdOffset[5] = HandleIdOffset<OrbitalStrike>();
    HandleScan kScan;
    unsigned int auProbe[4];
    const unsigned int uNextId = PblHandled::GetNextHandleId();
    PblHandled* pProbe = new (auProbe) PblHandled;
    pProbe->~PblHandled();
    PblHandled::SetNextHandleId(uNextId);
    kScan.uDeadVtable = auProbe[0];
    kScan.uLo = (unsigned int)g_Heap3Start;
    kScan.uHi = (unsigned int)gArenaEnd;
    kScan.uNext = PblHandled::GetNextHandleId();
    kScan.uLive = 0;
    kScan.uDead = 0;
    char* pcImage = (char*)GetModuleHandleA(0);
    IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pcImage;
    IMAGE_NT_HEADERS* pNt = (IMAGE_NT_HEADERS*)(pcImage + pDos->e_lfanew);
    IMAGE_SECTION_HEADER* pSection = IMAGE_FIRST_SECTION(pNt);
    for (unsigned int i = 0; i < pNt->FileHeader.NumberOfSections; ++i, ++pSection)
    {
        if ((pSection->Characteristics & IMAGE_SCN_MEM_WRITE) == 0)
        {
            continue;
        }
        const unsigned int* puFrom = (const unsigned int*)pcImage + pSection->VirtualAddress / 4;
        const unsigned int* puEnd = puFrom + pSection->Misc.VirtualSize / 4;
        const unsigned int* puRun = puFrom;
        const unsigned int* puAt = puFrom;
        while (puAt < puEnd)
        {
            MEMORY_BASIC_INFORMATION kInfo;
            if (VirtualQuery(puAt, &kInfo, sizeof(kInfo)) != sizeof(kInfo))
            {
                LOG(LogSim, Error, "OPENSWBF2 HANDLESCAN query-failed section=%.8s", (const char*)pSection->Name);
                break;
            }
            const unsigned int* puRegionEnd = (const unsigned int*)kInfo.BaseAddress + kInfo.RegionSize / 4;
            if (puRegionEnd > puEnd)
            {
                puRegionEnd = puEnd;
            }
            if (kInfo.State != MEM_COMMIT || (kInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
            {
                ScanHandleRange(kScan, puFrom, puRun, puAt, (const char*)pSection->Name);
                puRun = puRegionEnd;
            }
            puAt = puRegionEnd;
        }
        ScanHandleRange(kScan, puFrom, puRun, puAt, (const char*)pSection->Name);
    }
    for (int iHeap = 0; iHeap < maxheaps; ++iHeap)
    {
        void* pStart = 0;
        unsigned int uSize = 0;
        if (!RedGetHeapRange(iHeap, &pStart, &uSize))
        {
            continue;
        }
        const unsigned int uStart = (unsigned int)pStart;
        if (uStart + uSize > kScan.uLo && uStart < kScan.uHi)
        {
            continue;
        }
        ScanHandleRange(kScan, (const unsigned int*)pStart, (const unsigned int*)pStart,
                        (const unsigned int*)pStart + uSize / 4, RedGetHeapName(iHeap));
    }
    LOG(LogSim, Inform, "OPENSWBF2 HANDLESCAN state=%s live=%u dead=%u", bMission ? "mission" : "shell", kScan.uLive,
        kScan.uDead);
}
#endif

// FUNCTION: SWBF2 0x0053b640
void GameState::PostStateCleanup(bool bMission)
{
    gSoundEnabled = false;
    if (!sKioskMode)
    {
        sIgnoreNextTime = true;
    }

    RedMissionLog::Reset();
    DamageRegion::ClearList();
    AI::PatrolPath::DestroyAll();
    EntityPathFollower::ClearList();
    EntityFormation::DestroyAll();
    EntityPath::BranchRegion::DestroyAll();
    EntityPath::DestroyAll();
    GamePathFactory::ClearFactoryList();
    gNetSetup_errState = 0;
    RedParticleRenderer::Cleanup();

    NetGame::EnterClient();
    GameSoundEngine::Destroy();
    NetGame::LeaveClient();

    StringDB::Cleanup();
    RedLocalizeDB::Reset();
    LuaHelper::CleanupState();
    LuaMemory::Reset();
    gf::friends::reset();

    delete SoundDebugDisplay::sInstance;

    SoldierAnimatorLowResClass::Destroy();
    FLInputManager::Reset();
    RedAnimation::ClearAll();
    GuiManager::ResetAll();
    RedDebug::Cleanup();

    if (bMission)
    {
        ChunkClusterManager::Destroy();
        TriggerRegionManager::Destroy();
        DamageRegion::Destroy();

        BonusManager::Destroy();
        CommandPost::Destroy();
        CollisionManager::Destroy();
    }

    MuzzleFlash::Destroy();
    LeafParticleManager::Destroy();
    BirdParticleManager::Destroy();
    FishParticleManager::Destroy();
    OrdnanceTowCable::PlatformCleanup();
    OrdnanceGrapplingHook::PlatformCleanup();
    ReserveManager::Reset();
    SkyObjectManager::Clear();
    BoundaryManager::Destroy();
    LoadDisplay::Destroy();
    FirstPerson::Deinit();
    LockOnManager::DestroyAll();
    ReticuleDisplay::Destroy();
    HeroMessageDisplay::Destroy();
    ChangeClassDisplay::Destroy();
    StatIconDisplay::Destroy();
    StatusDisplay::Destroy();
    MessageDisplay::Destroy();
    BootDisplay::Destroy();
    SpectatorDisplay::Destroy();
    RedPath::ClearPaths();

    Factory<Entity, EntityClass, EntityDesc>::Reset();
    EntityEx::sLive.Clear();
    Factory<Weapon, WeaponClass, WeaponDesc>::Reset();
    Factory<Ordnance, OrdnanceClass, OrdnanceDesc>::Reset();
    Factory<Ordnance, ExplosionClass, OrdnanceDesc>::Reset();

    EntityLightClass::Reset();
    EntityHologramClass::Reset();
    AIUtil::Reset();
    ConnectivityGraphManager::Clean();
    PauseMenu::Reset();
    FlashyTextManager::Clean();
    RumbleRegionManager::Reset();
    NetGame::EnterClient();
    SkyManager::Reset();

    if (bMission)
    {
        NameDisplay::Reset();
        ItemTimerDisplay::Reset();
        ScopeDisplay::sInstance[0] = 0;
    }

    QOSDisplay::Reset();
    SpawnDisplay::Destroy();
    NetGame::LeaveClient();

    RedModel::ClearAll();
    RedTerrain::Reset();
    CollisionMesh::ClearAll();
    CollisionBodyPrimitive::ClearAll();
    LuaScriptFactory::Cleanup();
    Tracker::Cleanup();
    PropGenerator::Cleanup();
    HUD::Manager::Close();
    FLRenderer::Cleanup();
    const int iCleanupHeap = RedSetCurrentHeap(GameMemory::ApplicationHeap);
    pvs::IManager::ReserveDisplayables();
    RedSetCurrentHeap(iCleanupHeap);
    EntityCloth::ClearSpecialAllocator();
    ParticleEmitter::Cleanup();
    GameModel::Reset();

    ScreenTransition::Destroy();
    NetGame::EnterClient();
    FLEffect::CleanupAll();
    NetGame::LeaveClient();
    VehicleSpawn::Reset();
    DustEffectClass::Reset();
    LeafPatchClass::Reset();
    GrassPatchClass::Reset();
    LuaEventManager::Reset();
    PblRegion::Reset();

    RedRegionFactory::RemoveAll();
    SoldierAnimatorClass::Destroy();
    RedShadowProxy::Reset();
    FlagItem::Reset();
    StatsManager::Cleanup();
    SaveGameManager::Cleanup();
    RedDropShadowManager::Cleanup();
    GameMovie::Destroy(true);
    IFScreen::Reset();
    RedInterfaceElement::Reset();
    RedInterfaceScreen::s_screenFull.RemoveAllElements();
    RedGlyphCacheElement::CleanupPool();
    RedFont::Cleanup();
    RedTexture::Cleanup();
    ClothData::Cleanup();
    Thread::Reset();
    CollisionObject::ResetCollisionSerials();
    EntitySoldier::ResetWaterTimers();
    RedRenderer::Cleanup();
    RedProfile::Cleanup();

    if (!bMission)
    {
        for (int i = GameVoiceChat::GetNumNodes(); i != 0; --i)
        {
            GameVoiceChat::Node* pObject = GameVoiceChat::GetNode(i - 1);
            if (pObject)
            {
                pObject->SetID(0xff);
            }
        }
    }

    NetGame::Reset();
    RedMemoryReport::LogMemReport("exit");
#ifdef OPENSWBF2_TEST
    CollisionObject::LogSerialTies(true);
#endif
    MemoryPool::DestroyAll();
    TargetManager::Init();
    TeamManager::Reset();
    VehicleManager::Reset();
#ifdef OPENSWBF2_TEST
    ScanStaleHandles(bMission);
#endif
}

// TEMPLATE: SWBF2 0x0053c000
// PblJournal::Record<unsigned int>
template void PblJournal::Record<unsigned int>(unsigned int& value, PblJournalType type);

// FUNCTION: SWBF2 0x0053bff0
unsigned int __fastcall GetFactorySecondWord(const unsigned int* pThis)
{
    return pThis[1];
}

// SYNTHETIC: SWBF2 0x00403e70
// `dynamic initializer for 'GameState::MissionState::_PlayState''
// SYNTHETIC: SWBF2 0x00768ab0
// `dynamic atexit destructor for 'GameState::MissionState::_PlayState''

// GLOBAL: SWBF2 0x007eb99c
// GameState::MissionState::_PlayState
GameState::MissionState::PlayState GameState::MissionState::_PlayState;

// SYNTHETIC: SWBF2 0x00403e80
// `dynamic initializer for 'GameState::MissionState::PlayState::_NormalState''
// SYNTHETIC: SWBF2 0x00768ac0
// `dynamic atexit destructor for 'GameState::MissionState::PlayState::_NormalState''

// GLOBAL: SWBF2 0x007eb9b8
// GameState::MissionState::PlayState::_NormalState
GameState::MissionState::PlayState::NormalState GameState::MissionState::PlayState::_NormalState;

// SYNTHETIC: SWBF2 0x00403e90
// `dynamic initializer for 'GameState::MissionState::PlayState::_PausedState''
// SYNTHETIC: SWBF2 0x00768ad0
// `dynamic atexit destructor for 'GameState::MissionState::PlayState::_PausedState''

// GLOBAL: SWBF2 0x007eb978
// GameState::MissionState::PlayState::_PausedState
GameState::MissionState::PlayState::PausedState GameState::MissionState::PlayState::_PausedState;

