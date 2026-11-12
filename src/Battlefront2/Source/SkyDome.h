#pragma once

#include <stddef.h>

#include "RedEngineFL/Application/Renderable.h"
#include "RedEngineFL/Graphics/RedColor.h"
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblVector.h"
#include "Thread.h"

class PblConfig;
class pcRedPrimitive;
class pcRedShader;
class RedModel;
class ParticleEmitterObject;
class SkyStars;

// VTABLE: SWBF2 0x007ae618 Renderable
// VTABLE: SWBF2 0x007ae600 Thread
class SkyDome : public Renderable, public Thread
{
public:

    enum DomeDetailModelType
    {
        TRACK_CAMERA = 0,
        STATIC_HEIGHT = 1
    };

    struct EffectData
    {

        // SYNTHETIC: SWBF2 0x006377c0
        // SkyDome::EffectData::EffectData

        // FUNCTION: SWBF2 0x006377e0
        ~EffectData() {}

        unsigned int hardPoint;
        unsigned int effectHash;
        PblHandle<ParticleEmitterObject> effect;
        float timer;
        float fRestartTimeMin;
        float fRestartTimeMax;
        mutable PblVector3 pos;
    };

    struct DomeModel
    {

        DomeModel();

        // FUNCTION: SWBF2 0x006377a0
        ~DomeModel() {}

        RedModel* mModel;
        float mOffset;
        float mMovementScale;
        DomeDetailModelType mType;
        bool mReflectionFlag;
        float mRotationSpeed;
        PblVector3 mRotationAxis;
        EffectData mEffectData[20];
        int mNumEffectData;
    };

    SkyDome();

    virtual ~SkyDome();

    // SYNTHETIC: SWBF2 0x00637750
    // SkyDome::`scalar deleting destructor'

    // [thunk]:SkyDome::`vector deleting destructor'
    // SYNTHETIC: SWBF2 0x00638719
    // ??_ESkyDome@@WBM@AEPAXI@Z

    int Build();

    virtual void Render(unsigned int value) const;

    virtual bool Update(float fDeltaTime);

    static void Create();

    static void Destroy();

    void Read(PblConfig& config);

    static SkyDome* sInstance;

private:

    void Init();

    void Cleanup();

public:
    RedColor mReflectionColor;
    int mNumDomeModels;
    DomeModel mDomeModels[10];
    float mDomeHeight;
    unsigned int mTerrainTextureHash;
    float mTerrainMaxDist;
    int mTerrainPatchResolution;
    float mTerrainTextureScale;
    unsigned int mTerrainDetailTextureHash;
    float mTerrainDetailTextureScale;
    bool mForceHiDetail;
    RedColor m_colorSkyDome;

    void SetSkyDomeColor(const RedColor& color) { m_colorSkyDome = color; }
    SkyStars* m_pStars;
};
 
 
 
 
 
 

 
 
 
 
 
