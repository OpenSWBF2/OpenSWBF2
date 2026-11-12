#include "HUDEventQueue.h"
#include "GameLoop.h"

namespace HUD
{

// SYNTHETIC: SWBF2 0x0055e1a0
// HUD::DelayedEvent::DelayedEvent
// SYNTHETIC: SWBF2 0x0055e1b0
// HUD::DelayedEvent::~DelayedEvent

// SYNTHETIC: SWBF2 0x00404000
// HUD::`dynamic initializer for 'gEventList''
// SYNTHETIC: SWBF2 0x00768b90
// HUD::`dynamic atexit destructor for 'gEventList''

// GLOBAL: SWBF2 0x01ec6110
// gEventList
DelayedEvent gEventList[32];

// FUNCTION: SWBF2 0x0055e1c0
void RemoveAllEvents()
{
    for (int i = 0; i < 32; i++)
    {
        gEventList[i].activationTime = -1.0f;
    }
}

// FUNCTION: SWBF2 0x0055e1f0
void EventQueue::Create()
{
    RemoveAllEvents();
    sEventQueue.ActivateThread(0, PER_FRAME);
}

// SYNTHETIC: SWBF2 0x0055e290
// HUD::EventQueue::`scalar deleting destructor'

// FUNCTION: SWBF2 0x0055e2c0
EventQueue::~EventQueue()
{
    RemoveAllEvents();
}

// FUNCTION: SWBF2 0x0055e2d0
bool EventQueue::Update(float)
{
    for (int i = 0; i < 32; i++)
    {
        DelayedEvent& kEvent = gEventList[i];
        if (kEvent.activationTime > 0.0f
            && GameLoop::GetMissionTime() > kEvent.activationTime)
        {
            kEvent.e.Send();
            kEvent.activationTime = -1.0f;
        }
    }
    return true;
}

}

// FUNCTION: SWBF2 0x0055e210
void HUD::EventQueue::Destroy()
{
    RemoveAllEvents();
    sEventQueue.DeactivateThread();
}

// FUNCTION: SWBF2 0x0055e220
void HUD::EventQueue::AddEvent(const HUD::Event* pEvent, float fDelay)
{
    if (!pEvent)
    {
        return;
    }
    for (int i = 0; i < 32; ++i)
    {
        HUD::DelayedEvent& slot = HUD::gEventList[i];
        if (slot.activationTime < 0.0f)
        {
            slot.e = *pEvent;
            slot.activationTime = GameLoop::GetMissionTime() + fDelay;
            return;
        }
    }
}

// FUNCTION: SWBF2 0x0055e270
HUD::EventQueue::EventQueue()
{
    RemoveAllEvents();
}

// SYNTHETIC: SWBF2 0x00404030
// `dynamic initializer for 'HUD::EventQueue::sEventQueue''
// SYNTHETIC: SWBF2 0x00768bb0
// `dynamic atexit destructor for 'HUD::EventQueue::sEventQueue''

// GLOBAL: SWBF2 0x01ec60f0
// HUD::EventQueue::sEventQueue
HUD::EventQueue HUD::EventQueue::sEventQueue;
