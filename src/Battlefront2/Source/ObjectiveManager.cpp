#include "HUDObjectiveList.h"
#include "HUDElementMap.h"
#include "HUDEvent.h"
#include "HUDGameEvents.h"
#include "PebbleFL/Common/PblList.h"

extern bool netIsOneWorld;

namespace ObjectiveManager
{

// GLOBAL: SWBF2 0x007ebbcc
PblList<HUD::ObjectiveList> gHostObjectiveLists;
// GLOBAL: SWBF2 0x007ebbf8
PblList<HUD::ObjectiveList> gClientObjectiveLists;
// GLOBAL: SWBF2 0x01eaed58
PblList<HUD::ObjectiveList>* gObjectiveLists;

// FUNCTION: SWBF2 0x005f6920
unsigned int GetCurrentPopupText()
{
    if (gObjectiveLists && gObjectiveLists->GetCount() > 0)
    {
        return gObjectiveLists->GetFirst()->_pObject->GetCurrentPopupText();
    }
    return 0;
}

// FUNCTION: SWBF2 0x005f6940
void CleanList(PblList<HUD::ObjectiveList>& list)
{
    while (list.GetCount() > 0)
    {
        PblList<HUD::ObjectiveList>::Node* pNode = list.GetFirst();
        if (pNode->_pObject)
        {
            delete pNode->_pObject;
        }
        list.Remove(pNode);
    }
}

// FUNCTION: SWBF2 0x005f6990
void Init()
{
    CleanList(gClientObjectiveLists);
    CleanList(gHostObjectiveLists);
    gObjectiveLists = &gHostObjectiveLists;
}

// FUNCTION: SWBF2 0x005f69d0
void CreateClient()
{
    if (!netIsOneWorld)
    {
        CleanList(gClientObjectiveLists);
    }
}

// FUNCTION: SWBF2 0x005f69f0
void EnterClient()
{
    if (!netIsOneWorld)
    {
        gObjectiveLists = &gClientObjectiveLists;
    }
}

// FUNCTION: SWBF2 0x005f6a10
void LeaveClient()
{
    if (!netIsOneWorld)
    {
        gObjectiveLists = &gHostObjectiveLists;
    }
}

}

namespace ObjectiveManager
{

// FUNCTION: SWBF2 0x005f69b0
void DestroyAll()
{
    CleanList(gClientObjectiveLists);
    CleanList(gHostObjectiveLists);
}
}

namespace ObjectiveManager
{

// FUNCTION: SWBF2 0x005f6760
void AddObjectiveList(HUD::ObjectiveList* pList)
{
    PblList<HUD::ObjectiveList>* pLists = ObjectiveManager::gObjectiveLists;
    if (!pLists)
    {
        return;
    }

    for (PblList<HUD::ObjectiveList>::Itor it = pLists->Begin(); it != pLists->End(); ++it)
    {
        if (it->_viewportId == pList->_viewportId)
        {
            return;
        }
    }

    ObjectiveManager::gObjectiveLists->Add(new PblList<HUD::ObjectiveList>::Node, pList);
}

}

namespace ObjectiveManager
{

// FUNCTION: SWBF2 0x005f67f0
void AddObjective(unsigned int uiTextHashID, unsigned int uiPopupTextID, int iTeam)
{
    if (!ObjectiveManager::gObjectiveLists)
    {
        return;
    }
    for (PblList<HUD::ObjectiveList>::Itor it = ObjectiveManager::gObjectiveLists->Begin();
         it != ObjectiveManager::gObjectiveLists->End(); ++it)
    {
        it->AddObjective(uiTextHashID, uiPopupTextID, iTeam);
    }
}

// FUNCTION: SWBF2 0x005f6840
void SetObjectiveState(unsigned int uiTextHashID, int iState)
{
    HUD::Objective::ObjectiveState state = (HUD::Objective::ObjectiveState)iState;
    for (PblList<HUD::ObjectiveList>::Itor it = ObjectiveManager::gObjectiveLists->Begin();
         it != ObjectiveManager::gObjectiveLists->End(); ++it)
    {
        HUD::ObjectiveList* pList = it;
        if (state == HUD::Objective::ACTIVE && pList->GetState(uiTextHashID) != state)
        {
            HUD::ElementMap* pMap = HUD::ElementMap::FindByPlayerIndex(pList->GetViewport());
            if (pMap && pMap->GetMode() == HUD::ElementMap::mode_Small)
            {
                HUD::Event event(HUD::GameEvents::gPlayerEvents[pList->GetViewport()].objectivesUpdated, true);
                event.SendDelayed(0.25f);
                HUD::Event eventOff(HUD::GameEvents::gPlayerEvents[it->GetViewport()].objectivesUpdatedDisable, true);
                eventOff.SendDelayed(6.0f);
            }
        }
        it->SetState(uiTextHashID, state);
    }
}

// FUNCTION: SWBF2 0x005f6a30
HUD::ObjectiveList* GetObjectiveList()
{
    PblList<HUD::ObjectiveList>* list = ObjectiveManager::gObjectiveLists;
    return list && list->GetCount() > 0 ? list->GetFirst()->_pObject : 0;
}

}
