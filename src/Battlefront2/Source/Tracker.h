#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblList.h"
#include "Controllable.h"

class Trackable;

class Character;

class Tracker
{
public:
    Tracker();
    ~Tracker();

    static Tracker* GetTracker(int iIndex);

    static void SetTrackable(Trackable* pTrackable, int iPlayer);

    static void SetDeathTrackable(Trackable* pTrackable, int iPlayer, Character* pKiller);

    // FUNCTION: SWBF2 0x0044e3c0
    bool IsFirstPersonView()
    {
        Trackable* pTrackable = mTrackable;
        if (pTrackable && pTrackable != (Trackable*)-1 &&
            (unsigned int)pTrackable->mTracker != 0xDDDDDDDDu &&
            pTrackable->GetControllable())
        {
            Controllable* pCtrl = pTrackable->GetControllable();
            switch (pCtrl->GetControllableClass()->mForceMode)
            {
            case 1:
                return false;
            case 2:
                return true;
            }
        }
        return mIsFirstPersonView;
    }

    void SetFirstPersonView(int iPlayer, bool bFirstPerson);

    static void EnterClient();
    static void LeaveClient();

    static void CreateClient();

    PblList<Tracker>::Node m_AllListNode;

    // FUNCTION: SWBF2 0x004294f0
    Trackable* GetTrackable() { return mTrackable; }

    void SetSpectating(bool spectating) { mIsSpectating = spectating; }

    Trackable* mTrackable;
    bool mIsFirstPersonView;
    bool mIsSpectating;

    static PblList<Tracker> sAllInstances;
    static void Cleanup();

    static void RemoveTrackable(Trackable* pTrackable);
};
 
 
 
 
