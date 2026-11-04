#include "EventManager_Timer.h"
#include "Controllable.h"

namespace EventManager
{
namespace ControllableEvent
{
extern Event<Controllable, int> mWeaponChange;
extern Event<Controllable, int> mTeamChange;
extern Event<Controllable, int> mPlayerChange;
extern Event<Controllable, TargetInfo> mTargetChange;
void Cleanup();
}
}

namespace EventManager
{
namespace ControllableEvent
{
extern Event<Controllable, int> mWeaponChange;
}
}
namespace EventManager
{
namespace ControllableEvent
{
extern Event<Controllable, int> mTeamChange;
}
}
namespace EventManager
{
namespace ControllableEvent
{
extern Event<Controllable, int> mPlayerChange;
}
}
namespace EventManager
{
namespace ControllableEvent
{
extern Event<Controllable, TargetInfo> mTargetChange;
}
}

namespace EventManager
{

// TEMPLATE: SWBF2 0x005121f0
// EventManager::Event<Controllable,int>::Callback::operator()

// TEMPLATE: SWBF2 0x00512200
// EventManager::Event<Controllable,TargetInfo>::Callback::operator()

template bool Event<Controllable, int>::Callback::operator()(Controllable&, int&);
template bool Event<Controllable, TargetInfo>::Callback::operator()(Controllable&, TargetInfo&);

}

namespace EventManager
{

// FUNCTION: SWBF2 0x005121c0
void ControllableEvent::Cleanup()
{
    mWeaponChange.Cleanup();
    mTeamChange.Cleanup();
    mPlayerChange.Cleanup();
    mTargetChange.Cleanup();
}
}

namespace EventManager
{
namespace ControllableEvent
{

// SYNTHETIC: SWBF2 0x004032c0
// EventManager::ControllableEvent::`dynamic initializer for 'mWeaponChange''
// SYNTHETIC: SWBF2 0x007679a0
// EventManager::ControllableEvent::`dynamic atexit destructor for 'mWeaponChange''

// GLOBAL: SWBF2 0x007ebc80
// mWeaponChange
Event<Controllable, int> mWeaponChange("WeaponChange");

// SYNTHETIC: SWBF2 0x004032f0
// EventManager::ControllableEvent::`dynamic initializer for 'mTeamChange''
// SYNTHETIC: SWBF2 0x00767a20
// EventManager::ControllableEvent::`dynamic atexit destructor for 'mTeamChange''

// GLOBAL: SWBF2 0x007ebbe0
// mTeamChange
Event<Controllable, int> mTeamChange("TeamChange");

// SYNTHETIC: SWBF2 0x00403320
// EventManager::ControllableEvent::`dynamic initializer for 'mPlayerChange''
// SYNTHETIC: SWBF2 0x00767aa0
// EventManager::ControllableEvent::`dynamic atexit destructor for 'mPlayerChange''

// GLOBAL: SWBF2 0x007ebc50
// mPlayerChange
Event<Controllable, int> mPlayerChange("PlayerChange");

// SYNTHETIC: SWBF2 0x00403350
// EventManager::ControllableEvent::`dynamic initializer for 'mTargetChange''
// SYNTHETIC: SWBF2 0x00767b20
// EventManager::ControllableEvent::`dynamic atexit destructor for 'mTargetChange''

// GLOBAL: SWBF2 0x007ebc20
// mTargetChange
Event<Controllable, TargetInfo> mTargetChange("TargetChange");

}
}

// SYNTHETIC: SWBF2 0x00512210
// EventManager::Event<Controllable,int>::Callback::`scalar deleting destructor'

// SYNTHETIC: SWBF2 0x00512280
// EventManager::Event<Controllable,TargetInfo>::Callback::`scalar deleting destructor'
