#pragma once

#include <string.h>

#include "FLJoystick.h"
#include "ActionKey.h"
#include "RedEngineFL/input/RedInputEventHandler.h"

// VTABLE: SWBF2 0x0079f1e0
class FLInputManager : public RedInputEventHandler
{
public:

    FLInputManager();

    // SYNTHETIC: SWBF2 0x00529bd0
    // FLInputManager::`scalar deleting destructor'

    // FUNCTION: SWBF2 0x00529b30
    virtual ~FLInputManager()
    {
    }

    RawControllerInputs* GetPlayerJoystick(int iPlayer);

    void StopAllRumble();

    int GetPlayerFromHWPort(int iHWPort) const;
    void EnableRumble(int iPort, bool bEnable);

    void StopIntermittentRumble(int iPlayer);

    // FUNCTION: SWBF2 0x004294d0
    static FLInputManager* Instance() { return &s_instance; }

    // FUNCTION: SWBF2 0x00527f30
    int GetPrimaryPort() const { return mPrimaryPort; }
    bool GetReadAll() const { return mReadAllControllers; }
    bool GetAllowBindMultiple() const { return mAllowBindMultiple; }

    int GetSecondaryPort() const { return mSecondaryPort; }
    void SetAllowBindMultiple(bool b) { mAllowBindMultiple = b; }
    void SetBindAll(bool b) { mBindAll = b; }
    bool GetBindAll() const { return mBindAll != 0; }
    void SetAutoAcquire(bool b) { mAutoAcquire = b; }
    void SetIgnoreControllerRemoval(bool b) { mIgnoreMissing = b; }

    void SetReadAll(bool bReadAll);
    void SwapPrimarySecondary();
    void RestorePrimarySecondary();

    void BindPlayerInput();

    bool AnyMissing() const;

    GUIInputs* GetPlayerGuiInputs(int iPlayer);
    ProcessedInputs* GetPlayerInputs(int iPlayer);

    int GetHWPort(int iViewport) const;
    void SetHotController(int iJoystick);

    static void Reset();
    int GetBindCount() const;
    int CountControllers() const;
    static void Init();
    static void InitCustomFontData();
    void RescanForMissing();

    bool Update(float fDeltaTime);
    bool IsControllerBound(int iPlayer) const;
    void UnbindController(int iPlayer);

    void ProcessChatMessageInput(const unsigned short* pwText);

    bool mLCDPageToggle[1];
    unsigned char mReservedAfterPageToggle;

    unsigned short mChatText[0x200];
    unsigned char mReservedBeforeTextTimer[2];
    float mTextTimer;
    float mPageTimer;
    int mPageIndex;
    unsigned char mReservedBeforeJoysticks[4];

    RawControllerInputs mJoysticks[1];

    ProcessedInputs m_inputs[1];
    Rumble m_rumble[1];
    GUIInputs m_guiInputs[1];

    int m_HWPort[1];
    int m_HWPort_Full[1];
    int mPrimaryPort;
    int mSecondaryPort;
    int mShadowPrimary;
    int mShadowSecondary;
    bool mActiveControllers[1];
    bool mReadAllControllers;
    bool mAllowBindMultiple;
    bool mBindAll;
    bool m_AnyMissing;
    bool m_bPausedByInput;
    bool m_bHudGroupsEnabled[3];
    unsigned char mReservedBeforeMissingPort[3];
    int m_MissingPort;
    bool m_bIsMissing[1];
    bool m_bGameWasPaused;
    bool mAutoAcquire;
    unsigned char mReservedBeforeAskController;
    unsigned short mAskControllerMsg[128];
    bool mIgnoreMissing;

    static FLInputManager s_instance;
};
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

void InitMagicCharMap(void);

wchar_t GetRawFromProcessed_(ePROCESSEDINPUT_TYPE uiAction);

bool CanProcessChatMessage(void);
