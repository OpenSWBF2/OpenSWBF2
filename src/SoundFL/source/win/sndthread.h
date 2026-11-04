#pragma once

#include "SoundFL/source/sndthreadbase.h"

namespace Snd
{

class Thread : public ThreadBase
{
public:

    Thread()
        : ThreadBase()
    {
    }

    Thread(void(__cdecl* pfnThreadFunc)(void*), unsigned int uiPriority, unsigned int uiStackSize)
        : ThreadBase(pfnThreadFunc, uiPriority, uiStackSize)
    {
    }

    bool Start(void* pArgs);
};
 

}
