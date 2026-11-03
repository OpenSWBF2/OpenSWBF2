#include <stdio.h>

#include "ODFPropertySet.h"
#include "HUDElementMap.h"
#include "HUDEvent.h"
#include "HUDGameEvents.h"
#include "HUDObjectiveList.h"
#include "PebbleFL/Common/PblList.h"

namespace ObjectiveManager
{
extern PblList<HUD::ObjectiveList>* gObjectiveLists;
}

namespace ObjectiveManager
{
void AddObjective(unsigned int uiTextHashID, unsigned int uiPopupTextID, int iTeam);
}


namespace ObjectiveManager
{
void SetObjectiveState(unsigned int uiTextHashID, int iState);
}


namespace ODFPropertySet
{

// GLOBAL: SWBF2 0x0078a70c
char* sScanfFormats[10] = {
    "%s", "%f", "%c", "%c", "%hd", "%hu", "%d", "%u", "%ld", "%lu",
};

// FUNCTION: SWBF2 0x005f6a50
bool SetProperty(const ODFProperty* const pProperties, unsigned int uNumProperties,
                 const void* const pObject, unsigned int uProperty,
                 const char* const pszValue)
{
    for (unsigned int i = 0; i < uNumProperties; ++i)
    {
        if (pProperties[i].id == uProperty)
        {
            sscanf(pszValue, sScanfFormats[pProperties[i].type - 1],
                   (char*)pObject + pProperties[i].memberOffset);
            return true;
        }
    }
    return false;
}

}
