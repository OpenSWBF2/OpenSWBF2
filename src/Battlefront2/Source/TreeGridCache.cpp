#include "TreeGrid.h"
#include "TreeGridCache.h"

// FUNCTION: SWBF2 0x0065c810
void TreeGridCache::CallbackOnRect(TreeGrid* pTreeGrid, const PblVector3& rMin, const PblVector3& rMax, TreeGridCallback pCallback, TreeGridCallbackData* pData)
{
    pTreeGrid->CallbackOnRect(rMin, rMax, pCallback, pData);
}

// FUNCTION: SWBF2 0x0065c830
void TreeGridCache::CallbackOnLazyRay(TreeGrid* pTreeGrid, const PblVector3& rFrom, const PblVector3& rTo, TreeGridCallback pCallback, TreeGridCallbackData* pData)
{
    pTreeGrid->CallbackOnLazyRay(rFrom, rTo, pCallback, pData);
}

// FUNCTION: SWBF2 0x0065c850
void TreeGridCache::CallbackOnSphere(TreeGrid* pTreeGrid, const PblVector3& rCenter, float fRadius, TreeGridCallback pCallback, TreeGridCallbackData* pData)
{
    pTreeGrid->CallbackOnSphere(rCenter, fRadius, pCallback, pData);
}

// FUNCTION: SWBF2 0x0065c870
void TreeGridCache::Prime(TreeGrid*, const PblVector3&, const PblVector3&)
{
}

// FUNCTION: SWBF2 0x0065c880
void TreeGridCache::Flush()
{
}
