#pragma once
#include "NetGame.h"

#include "PebbleFL/Common/PblIndex.h"
#include "PebbleFL/Common/PblTimeBase.h"

class RedScreenGroupElement;
class RedTextElement;

class GameLoop
{
public:


    static void Update(float fDeltaTime, float fSystemDeltaTime);

    static bool sQuitRequested;
    static unsigned int sQuitTurn;

    // FUNCTION: SWBF2 0x005aecc0
    static inline bool IsQuitRequested()
    {
        return GameLoop::sQuitRequested &&
               (NetGame::GetTurnNumber() == 0 ||
                NetGame::GetTurnNumber() >= GameLoop::sQuitTurn);
    }

static char mMissionScript[64];

static unsigned int mMissionScriptHash;

static char mWorldFileName[64];
static char mTerrainFileName[64];

static void SetNextMission(char const* pcName);

// FUNCTION: SWBF2 0x004882d0
static inline char const* GetNextMission() { return mMissionScript; }

static void SetWorldFilename(char const* pcName);
static void SetTerrainFilename(char const* pcName);

static void Pause();

static void Resume();

static int Init();
static int PostLoad();

static int Cleanup();

#ifdef OPENSWBF2_TEST
static bool TakeRepeatPending();
#endif

static bool _bFirstPersonEnabled;

static inline void EnableFirstPerson(bool bEnable) { _bFirstPersonEnabled = bEnable; }

static inline bool FirstPersonEnabled() { return _bFirstPersonEnabled; }

static bool sPauseMode;

static bool gInvincible;

static bool mbMetagameRules;

// FUNCTION: SWBF2 0x0044c570
static inline bool AreMetagameRulesOn() { return mbMetagameRules; }

// FUNCTION: SWBF2 0x00596b60
static inline void SetMetagameRulesOn(bool bOn) { mbMetagameRules = bOn; }

static unsigned int mNumCameras;

// FUNCTION: SWBF2 0x004a9cb0
static inline unsigned int GetNumCameras() { return mNumCameras; }

static inline void SetNumCameras(unsigned int uNum) { mNumCameras = uNum; }

static int mMetagameSide[4];

// FUNCTION: SWBF2 0x005b40d0
static inline int GetMetagameSide(int iIndex) { return mMetagameSide[iIndex]; }

static inline void SetMetagameSide(int iIndex, int iSide) { mMetagameSide[iIndex] = iSide; }

static int mLastGameWinner;
static inline int GetLastGameWinner() { return mLastGameWinner; }
static inline void SetLastGameWinner(int iWinner) { mLastGameWinner = iWinner; }

static float GetMissionTime();

static bool IsGameOver();

static int GetJoystickYFlip();

static void SetJoystickYFlip(int iFlip);

static bool sGameOver;

static bool mShowTeamPoints[2];
static inline void SetShowTeamPoints(int iTeam, bool bShow)
{
    if (PblInBounds(mShowTeamPoints, iTeam - 1))
    {
        mShowTeamPoints[iTeam - 1] = bShow;
    }
}




static float sDeltaTime;




// FUNCTION: SWBF2 0x004439e0
static inline float GetDeltaTime() { return GameLoop::sDeltaTime; }



static void SetCanSwitchSides(bool bFlag);


    static bool GetCanSwitchSides();
    static bool mbHistoricalRules;
    static float sClientDeltaTime;
    static bool sShowStats;
    static PblTimeBase::Stamp sMissionTime;
private:
    static float DedicatedMsgX;
    static float DedicatedMsgY;
    static float DedicatedMsgDX;
    static float DedicatedMsgDY;
    static RedScreenGroupElement mAttractGroup;
    static RedTextElement mAttractInfoText;
    static RedScreenGroupElement mDedicatedGroup;
    static RedTextElement mDedicatedTitleText;
    static RedTextElement mDedicatedTeam1Text;
    static RedTextElement mDedicatedTeam2Text;
    static RedScreenGroupElement mWaitMessageGroup;
    static RedTextElement mWaitTextElement;
public:
    static float sSystemDeltaTime;
    static float clientInputAcc;
    static float clientTimePrev;
    static float mWaitTimer;
    static bool gUnlimitedAmmo;
    static float sGameOverDelayTimer;
    static float mAttractModeTime;
    static void CheckScreenShots();
    static void CheckToggleDisplay();
private:
    static void UpdateStats(float fDeltaTime, float fSystemDeltaTime);
    static void UpdateDedicatedMsg(float fDeltaTime);
    static bool mbCanSwitchSides;
public:
    static void SetFunctionIdForButtonId(int iButton, int iFunction);
    static int GetFunctionIdForButtonId(int iButton);
    static int GetFunctionIdForAnalog(int analogStick, int mode);
    static bool sFreeMode;
    static void SetFreeMode(bool bFreeCamera);
};
