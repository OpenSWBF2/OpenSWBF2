#pragma once

#include "PebbleFL/Common/PblListDoubleSorted.h"

#include "sndsourcevirtual.h"
#include "sndvoicebase.h"

namespace Snd
{

class VoiceVirtualManager;

class VoiceVirtual : public VoiceBase, public SourceVirtual
{
public:
    enum Flags
    {
        flags_updateVoice = 1,
        flags_active = 2,
        flags_restorePlaybackPos = 4,
        flags_updatePriority = 8
    };

    typedef void (__cdecl* InactiveCallback)(const VoiceVirtual* const, void* const);

    VoiceVirtual(unsigned int uiCustomFlags, VoiceVirtualManager* pManager,
                 bool bRestorePlaybackPos);
    ~VoiceVirtual();

    void Stop();

    VoiceBase* GetVoice() { return mVoice; }
    unsigned int GetCustomFlags() const { return mCustomFlags; }

    float CalculatePriority(bool bForce);
    void Play(bool bRestart);

    void Pause();

    void ResetPriority();
    void Set3DMode(bool b3DMode);
    void SetInactiveCallback(InactiveCallback pfnCallback,
                             void* const pData);

    void SetVoiceVirtualManager(VoiceVirtualManager* pManager);

protected:
    void RemoveAll();
    void Update3DMode();

private:

    bool GetActive() const { return (mFlags & flags_active) != 0; }
    bool GetVoiceChanged()
    {
        const bool bChanged = (mFlags & flags_updateVoice) != 0;
        mFlags &= ~(unsigned int)flags_updateVoice;
        return bChanged;
    }

    void Inactive()
    {
        if (mInactiveCallback)
        {
            mInactiveCallback(this, mInactiveCallbackData);
            mInactiveCallback = 0;
            mInactiveCallbackData = 0;
        }
        mNode.mSortValue = 0.0f;
    }
    void Remove();
    void SetVoice(VoiceBase* pVoice);
    void Update(float fDeltaTime);
    friend class VoiceVirtualManager;

public:

    PblListDoubleSorted::Node mNode;
    unsigned int mFlags;
    VoiceBase* mVoice;
    unsigned int mCustomFlags;
    VoiceVirtualManager* mVoiceManager;
    InactiveCallback mInactiveCallback;
    void* mInactiveCallbackData;
};

 

}
