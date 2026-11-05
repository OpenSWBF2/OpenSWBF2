#pragma once

#include "PblList.h"

template <class TYPE>
inline void PblListDeleteSelfRemoving(PblList<TYPE>& list)
{
    while (list.GetCount() > 0)
    {
        delete list.GetFirst()->_pObject;
    }
}

template <class TYPE>
inline void PblListRemoveAndDelete(PblList<TYPE>& list)
{
    while (list.GetCount() > 0)
    {
        typename PblList<TYPE>::Node* pNode = list.GetFirst();
        TYPE* pObject = pNode->_pObject;
        list.Remove(pNode);
        delete pObject;
    }
}
