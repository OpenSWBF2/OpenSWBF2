#pragma once

#include <float.h>
#include <stddef.h>

#include "sndallocator.h"
#include "sndsound.h"
#include "sndsource3d.h"
#include "sndsourcetransport.h"

#include "PebbleFL/Common/PblListSingle.h"
#include "PebbleFL/Common/PblVector.h"

#include "PebbleFL/Common/PblListDouble.h"

class PblFileChunk;


namespace Snd
{

class SoundLayered : public SourceTransport
{
public:
    class Properties;
    class SoundLayer;

    SoundLayered();

    Sound* FindLayerByHashID(unsigned int uiHashID);

    Sound* GetLayerByIndex(unsigned int uIndex);
    ~SoundLayered();

    static void __cdecl Play(Source3D* pSource3D, const Properties* pProps);

    void SetPosition(const PblVector3* pPos);
    void SetVelocity(const PblVector3* pVel);

    void Play(bool bRestart) { SourceTransport::Play(bRestart); }

    class FireForgetLayer;

    static void Open();

    static void Close();

    static void __cdecl UpdateAll(float fDeltaTime);

    static void __cdecl RemoveAll();

    static FireForgetLayer* smFireForgetLayers;
    static bool smIsOpen;
    static unsigned char smFireForgetLayerStorage[5888];

    static PblListDouble smSoundLayeredList;

protected:

    void SetProperties(const Properties* pProps, SoundLayer* (__cdecl* pLayerCreate)(void),
                       void (__cdecl* pLayerDestroy)(SoundLayer*));

    void Update(float fDeltaTime);

    void FreeLayers(void (__cdecl* pLayerDestroy)(SoundLayer*));

private:

    static void __cdecl SoundLayerDestroy(SoundLayer* pLayer);

public:
    PblListSingle mSoundList;
    PblListDouble::Node mNode;
    float mTimeElapsed;
    float mStopTime;
    const Properties* mProperties;
};
 
 
 

 

class SoundLayered::SoundLayer : public Sound
{
public:

    SoundLayer() : mPlayTime(0.0f), mStopTime(FLT_MAX) {}

    ~SoundLayer()
    {
    }

    static void operator delete(void* p) { smFreeList.Free(p); }

    PblListSingle::Node mLayerNode;
    float mPlayTime;
    float mStopTime;

    static FreeList smFreeList;
};
 
 

class SoundLayered::Properties
{
public:

    class Layer
    {
    public:

        Layer(Sound::Properties* pSoundProps, float fDelay, float fDelayVar, float fPlayTime,
              float fPlayTimeVar, unsigned int uiHashID, PblListSingle& list)
            : mSoundProps(pSoundProps), mDelay(fDelay), mDelayVar(fDelayVar),
              mPlayTime(fPlayTime), mPlayTimeVar(fPlayTimeVar), mHashID(uiHashID)
        {
            list.AddLast(&mNode);
        }

        ~Layer()
        {
        }

        static void* operator new(size_t) { return smFreeList.Alloc(); }
        static void operator delete(void* p) { smFreeList.Free(p); }

        Sound::Properties* mSoundProps;
        float mDelay;
        float mDelayVar;
        float mPlayTime;
        float mPlayTimeVar;
        unsigned int mHashID;
        PblListSingle::Node mNode;

        static FreeList smFreeList;
    };

    static Properties* __cdecl FindByHashID(unsigned int uiHashID);

    Layer* FindLayerByHashID(unsigned int uiHashID);

    unsigned int GetHashID() const { return mHashID; }

    Layer* FindLayerByIndex(unsigned int uIndex) const
    {
        return (Layer*)(mLayerList.FindByIndex(uIndex)->GetObjectA(
            &mLayerList, offsetof(Layer, mNode)));
    }

    PblListSingle mLayerList;
    unsigned int mHashID;

    static FreeList smFreeList;
    static PblListSingle smPropertyList;

    Properties() : mHashID(0)
    {
        smPropertyList.AddLast(&mNode);
    }
    static void* operator new(size_t) { return smFreeList.Alloc(); }
    static void operator delete(void* p) { smFreeList.Free(p); }

    ~Properties();
    // FUNCTION: SWBF2 0x00734010
    static void __cdecl DestroyAll()
    {
        while (!smPropertyList.IsEmpty())
        {
            delete (Properties*)smPropertyList.GetFirst()->GetObjectA(
                &smPropertyList, offsetof(Properties, mNode));
        }
    }

    void Read(PblFileChunk& chunk);

    PblListSingle::Node mNode;

    static void __cdecl Load(PblFileChunk& chunk, bool bOnce);
};

class SoundLayered::FireForgetLayer : public Allocator<SoundLayered::FireForgetLayer>
{
public:

    FireForgetLayer()
        : mTimeElapsed(0.0f),
          mTriggerTime(0.0f),
          mSource3D(0),
          mLayer(0)
    {
        smInactiveList.AddLast(&mNode);
    }


    void Stop()
    {
        mNode.Remove();
        smInactiveList.AddLast(&mNode);
    }

    static void StopAll();

    static FireForgetLayer* __cdecl GetFreeLayer();

    void Play(Source3D* pSource3D, const Properties::Layer* pLayer, float fPriority);

    static float __cdecl CalculatePriority(Source3D* pSource3D,
                                           const Sound::Properties* pProps);
    static FireForgetLayer* __cdecl StealLayer(float fPriority);

    static void UpdateAll(float fDeltaTime);

protected:

    void Update(float fDeltaTime);

public:

    float mTimeElapsed;
    float mTriggerTime;
    float mPriority;
    Source3D mSource3D;
    const Properties::Layer* mLayer;
    PblListDouble::Node mNode;

    PblVector3 mPos;
    PblVector3 mVel;

    static PblListDouble smInactiveList;

    static PblListDouble smActiveList;
};

inline Sound* SoundLayered::GetLayerByIndex(unsigned int uIndex)
{
    return (SoundLayer*)(mSoundList.FindByIndex(uIndex)->GetObjectA(
        &mSoundList, offsetof(SoundLayer, mLayerNode)));
}

}
