#include <string.h>
#include "PopupText.h"
#include "FLJoystick.h"
#include "FLInputManager.h"
#include "GameSoundEngine.h"
#include "PauseMenu.h"
#include "LuaHelper.h"
#include "Trigger.h"
#include "GameLoop.h"
#include "HUDElementMap.h"
#include "HUDGameEvents.h"
#include "HUDEvent.h"
#include "NetGame.h"
#include "Battlefront2/Platform/Pc/pcSpawnDisplay.h"
#include "PebbleFL/Common/PblUnicode.h"
#include "SoundFL/source/sndbus.h"

namespace HUD
{
namespace GameEvents
{
extern void DisableAllForObjectiveDisplay(int iViewport, bool bInit);
}
}

// FUNCTION: SWBF2 0x00527ea0
bool GUIInputs::IsLeftPressed() const
{
    return mProcessedGUIInputs[eGUIINPUT_Left] >= 0.125f ||
           mProcessedGUIInputs[eGUIINPUT_xAxis] <= -0.5f;
}

// FUNCTION: SWBF2 0x00527ed0
bool GUIInputs::IsRightPressed() const
{
    return mProcessedGUIInputs[eGUIINPUT_Right] >= 0.125f ||
           mProcessedGUIInputs[eGUIINPUT_xAxis] >= 0.5f;
}

namespace PopupText
{


// GLOBAL: SWBF2 0x01eaedfb
bool gMapEnabled[1];
// GLOBAL: SWBF2 0x01eaedfa
bool gSpawnDisplayEnabled[1];
// GLOBAL: SWBF2 0x01eaee20
int gNumHints;
// GLOBAL: SWBF2 0x01eaee24
int gCurHint;
// GLOBAL: SWBF2 0x01eaee29
bool gDirectionPressedLastUpdate;

// GLOBAL: SWBF2 0x01eaee1f

bool gAcceptReleased;

// GLOBAL: SWBF2 0x01eaee28

bool gPopupShownLatch;
// GLOBAL: SWBF2 0x01eaee2c
unsigned int gLastObjectiveTextID;
// GLOBAL: SWBF2 0x01eaee30
PopupType gType;
// GLOBAL: SWBF2 0x01eaee38
unsigned int gHints[64];

// GLOBAL: SWBF2 0x01eaee2a
bool gPreventJump;
// GLOBAL: SWBF2 0x01ef7fcc
Trigger gButtonTrigger;

}

// FUNCTION: SWBF2 0x00620af0
void PopupText::ShowPopup(PopupText::PopupType iType, unsigned int uTextId)
{
    using namespace PopupText;
    if (GameLoop::mNumCameras != 1 || (iType == HINT && gHints[gCurHint] == 0))
    {
        return;
    }

    if (IsEnabled())
    {
        PopupText::DisablePopup();
    }
    else
    {
        Snd::Bus* pBus = Snd::Bus::FindByHashID(0xB38617C5 /* "mainmain" */);
        if (pBus)
        {
            pBus->Fade(0.5f, 0.0f, -1.0f);
        }
        HUD::ElementMap* pMap = HUD::ElementMap::FindByPlayerIndex(0);
        gMapEnabled[0] = pMap && ((RedInterfaceElement*)(RedGroupElement*)pMap)->IsEnabled();
        HUD::GameEvents::DisableAllForObjectiveDisplay(0, true);
        SpawnDisplay* pSpawn = SpawnDisplay::Instance(0);
        if (pSpawn->mVisible && pSpawn->mMode == SpawnDisplay::eCHANGECLASS)
        {
            gSpawnDisplayEnabled[0] = true;
            pSpawn->Hide(NetGame::GetLocalPlayer(NetGame::GetLocalIndex(0)));
        }
        else
        {
            gSpawnDisplayEnabled[0] = false;
        }
    }

    if (!NetGame::IsEnabled())
    {
        GameLoop::Pause();
    }
    gType = (PopupType)iType;
    gButtonTrigger.Clear();

    switch (iType)
    {
    case SELECTION:
        if (uTextId)
        {
            gLastObjectiveTextID = 0;
            HUD::Event evSelection(HUD::GameEvents::gObjectiveEvents.selectionPopup, uTextId);
            evSelection.Send();
        }
        break;
    case OBJECTIVE:
        if (uTextId)
        {
            gLastObjectiveTextID = uTextId;
            HUD::Event evObjective(HUD::GameEvents::gObjectiveEvents.objectivePopup, uTextId);
            evObjective.Send();
        }
        break;
    case HINT:
    {
        HUD::Event evHint(HUD::GameEvents::gObjectiveEvents.hintPopup, gHints[gCurHint]);
        evHint.Send();
        unsigned short page[16];
        PblUnicode::usnprintf(page, 16, "%d/%d", gCurHint + 1, gNumHints);
        HUD::Event evPage(HUD::GameEvents::gObjectiveEvents.hintPopupPageNumber,
                          (const wchar_t*)page);
        evPage.Send();
        break;
    }
    }

    HUD::Event evPressSelect(HUD::GameEvents::gPlayerEvents[0].pressSelectToReturnDisable, true);
    evPressSelect.Send();
    HUD::Event evObjectiveDetails(HUD::GameEvents::gPlayerEvents[0].objectiveDetailsDisable, true);
    evObjectiveDetails.Send();
    HUD::Event evHintsAvailable(HUD::GameEvents::gPlayerEvents[0].hintsAvailableDisable, true);
    evHintsAvailable.Send();
    gPreventJump = true;
    gPopupShownLatch = true;
    gAcceptReleased = false;
}

namespace ObjectiveManager
{
unsigned int GetCurrentPopupText();
}

// FUNCTION: SWBF2 0x00620d20
void PopupText::DisablePopup()
{
    if (!PopupText::IsEnabled())
    {
        return;
    }

    const unsigned int uNumCameras = GameLoop::mNumCameras;
    Snd::Bus* pBus = Snd::Bus::FindByHashID(0xB38617C5 /* "mainmain" */);
    if (pBus)
    {
        pBus->Fade(0.5f, 1.0f, -1.0f);
    }
    if (GameLoop::sPauseMode)
    {
        GameLoop::Resume();
    }

    for (unsigned int i = 0; i < uNumCameras; ++i)
    {
        if (PopupText::gMapEnabled[i])
        {
            HUD::Event mapEvent(HUD::GameEvents::gPlayerEvents[i].mapEnable, true);
            mapEvent.Send();
        }
        if (PopupText::gSpawnDisplayEnabled[i])
        {
            Character* pPlayer = NetGame::GetLocalPlayer(NetGame::GetLocalIndex(i));
            SpawnDisplay::Instance(i)->ChangeClass(pPlayer);
        }
    }

    switch (PopupText::gType)
    {
    case PopupText::HINT:
        {
            HUD::Event popupEvent(HUD::GameEvents::gObjectiveEvents.hintPopupDisable, true);
            popupEvent.Send();
        }
        break;
    case PopupText::OBJECTIVE:
        {
            HUD::Event popupEvent(HUD::GameEvents::gObjectiveEvents.objectivePopupDisable, true);
            popupEvent.Send();
        }
        break;
    case PopupText::SELECTION:
        {
            HUD::Event popupEvent(HUD::GameEvents::gObjectiveEvents.selectionPopupDisable, true);
            popupEvent.Send();
        }
        break;
    default: break;
    }

    if (uNumCameras == 1)
    {
        HUD::ElementMap* pMap = HUD::ElementMap::FindByPlayerIndex(0);
        if (pMap && pMap->mMode == HUD::ElementMap::mode_Large)
        {
            HUD::Event listEvent(HUD::GameEvents::gPlayerEvents[0].objectiveListEnable, true);
            listEvent.Send();
            HUD::Event returnEvent(HUD::GameEvents::gPlayerEvents[0].pressSelectToReturn, true);
            returnEvent.Send();
            if (ObjectiveManager::GetCurrentPopupText())
            {
                HUD::Event detailsEvent(HUD::GameEvents::gPlayerEvents[0].objectiveDetails, true);
                detailsEvent.Send();
            }
            if (PopupText::gNumHints > 0)
            {
                HUD::Event hintsEvent(HUD::GameEvents::gPlayerEvents[0].hintsAvailable, true);
                hintsEvent.Send();
            }
        }
    }

    PopupText::gType = PopupText::DISABLED;
    PopupText::gButtonTrigger.Clear();
}

// FUNCTION: SWBF2 0x006210d0
void PopupText::SetPreventJump(bool bPrevent)
{
    gPreventJump = bPrevent;
}

namespace PopupText
{

// FUNCTION: SWBF2 0x00620ec0
void AddHint(unsigned int uHint)
{
    if (PopupText::gNumHints < 64)
    {
        PopupText::gHints[PopupText::gNumHints++] = uHint;
    }
}

// FUNCTION: SWBF2 0x00620f00
void Update(float fDeltaTime)
{
    GameSoundEngine::Update(fDeltaTime);
    if (PauseMenu::GetPaused(PauseMenu::GetPausingViewport()) || FLInputManager::s_instance.AnyMissing())
    {
        return;
    }

    ProcessedInputs* pInput = FLInputManager::s_instance.GetPlayerInputs(0);
    GUIInputs* pGui = FLInputManager::s_instance.GetPlayerGuiInputs(0);
    const bool bAcceptDown = pGui->mProcessedGUIInputs[eGUIINPUT_Accept] >= 0.125f;
    if (!bAcceptDown)
    {
        gAcceptReleased = true;
    }

    switch (gType)
    {
    case SELECTION:
        if (pInput)
        {
            gButtonTrigger.Update(fDeltaTime, pInput->IsAcceptHeroPressed());
            if (gButtonTrigger.IsPressed())
            {
                LuaHelper::CallGlobalProc("SetFlyerControllerConfig", 1.0f);
                DisablePopup();
                return;
            }
            gButtonTrigger.Update(fDeltaTime, pInput->IsDeclineHeroPressed());
            if (gButtonTrigger.IsPressed())
            {
                LuaHelper::CallGlobalProc("SetFlyerControllerConfig", 2.0f);
                DisablePopup();
            }
        }
        break;
    case OBJECTIVE:
        if (gAcceptReleased && bAcceptDown)
        {
            DisablePopup();
        }
        break;
    case HINT:
        if (gAcceptReleased && bAcceptDown)
        {
            DisablePopup();
        }
        else if (pGui->IsLeftPressed() && !gDirectionPressedLastUpdate)
        {
            if (--gCurHint < 0)
            {
                gCurHint = gNumHints - 1;
            }
            ShowPopup(HINT, 0);
        }
        else if (pGui->IsRightPressed() && !gDirectionPressedLastUpdate)
        {
            if (++gCurHint >= gNumHints)
            {
                gCurHint = 0;
            }
            ShowPopup(HINT, 0);
        }
        gDirectionPressedLastUpdate = pGui->IsRightPressed() || pGui->IsLeftPressed();
        break;
    }
}
}

// FUNCTION: SWBF2 0x00620a90
void PopupText::Init()
{
    gType = DISABLED;
    memset(gHints, 0, sizeof(gHints));
    gCurHint = 0;
    gNumHints = 0;
    gLastObjectiveTextID = 0;
    gDirectionPressedLastUpdate = false;
    gMapEnabled[0] = false;
    gSpawnDisplayEnabled[0] = false;
}

// FUNCTION: SWBF2 0x00620ef0
bool PopupText::IsEnabled()
{
    return gType != DISABLED;
}

// FUNCTION: SWBF2 0x006210e0
bool PopupText::GetPreventJump()
{
    return gPreventJump;
}


// FUNCTION: SWBF2 0x00620ee0
int PopupText::GetNumHints()
{
    return PopupText::gNumHints;
}
