#pragma once

#include "Battlefront2/Source/EventManager.h"
#include "EventManager_LuaCallback.h"
#include "PebbleFL/Common/PblHandle.h"
#include "Timer.h"
#include "LuaHelper.h"

namespace EventManager
{

template <>
bool LuaPushItem<char>(char& item);

class TimerMatches
{
public:

    TimerMatches(const Timer* pTimer) : mTimer(pTimer) {}
    TimerMatches(const char* pcName);

    static const char* GetName() { return ""; }

    bool operator==(const TimerMatches& other) const
    {
        return mTimer.SameAs(other.mTimer);
    }

    bool operator()(Timer& timer, char&) const
    {
        return mTimer == &timer;
    }

    PblHandle<Timer const> mTimer;
};

namespace TimerEvent
{
extern Event<Timer, char> mElapsed;
void Cleanup();
}

}

