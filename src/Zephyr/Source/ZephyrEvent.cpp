#include "ZephyrEvent.h"
#include <new>
#include <string.h>

// FUNCTION: SWBF2 0x0072ee50
void ZephyrEventList::AddEvent(float fTime, unsigned int uiMessage)
{
    for (int i = 0; i < _iNumEvents; ++i)
    {
        if (_pEventMessages[i] == 0)
        {
            _pEventTimes[i] = fTime;
            _pEventMessages[i] = uiMessage;
            return;
        }
    }
}

// FUNCTION: SWBF2 0x0072ee90
void ZephyrEventBank::Init(short iNumLists)
{
    _Lists = new ZephyrEventList[iNumLists];
    _iMaxLists = iNumLists;
    _iNumLists = 0;
}

// FUNCTION: SWBF2 0x0072ef00
void ZephyrEventList::Init(short iNumEvents)
{
    _iNumEvents = iNumEvents;
    if (_iNumEvents > 0)
    {
        _pEventTimes = new float[_iNumEvents];
        _pEventMessages = new unsigned int[_iNumEvents];
        memset(_pEventTimes, 0, _iNumEvents * sizeof(float));
        memset(_pEventMessages, 0, _iNumEvents * sizeof(unsigned int));
    }
}

// FUNCTION: SWBF2 0x0072ef70
ZephyrEventList* ZephyrEventBank::New(unsigned int uiHash)
{
    if (_iNumLists < _iMaxLists)
    {
        short i = _iNumLists++;
        _HashTable.Store(uiHash, &_Lists[i]);
        return &_Lists[i];
    }
    return 0;
}
