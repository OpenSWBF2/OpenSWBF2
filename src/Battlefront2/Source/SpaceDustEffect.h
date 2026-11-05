#pragma once

#include <stddef.h>
#include "FLEffect.h"
#include "PebbleFL/Common/PblVector.h"

// VTABLE: SWBF2 0x007af278
class SpaceDustEffect : public FLEffect
{
public:

    struct ParticleData
    {
        ParticleData() { mLifeLeft = -1.0f; }

        PblVector3 mCoord;
        float mLifeLeft;
        float mScale;
    };

    struct DustEmitterData
    {

        DustEmitterData();

        ~DustEmitterData();

        // FUNCTION: SWBF2 0x0064cf30
        void Init(bool bFreeParticles, bool bAllocParticles)
        {
            mEnabled = false;
            mTimeSinceLastSpawned = 0.0f;
            if (bFreeParticles && mParticle)
            {
                delete[] mParticle;
                mParticle = 0;
            }
            if (bAllocParticles && !mParticle)
            {
                mParticle = new ParticleData[80];
            }
            if (mParticle)
            {
                for (int i = 0; i < 80; i++)
                {
                    mParticle[i].mLifeLeft = -1.0f;
                }
            }
        }

        bool mEnabled;
        float mTimeSinceLastSpawned;
        float mParticleLife;
        float mMaxParticles;
        ParticleData* mParticle;
    };

    SpaceDustEffect();

    virtual ~SpaceDustEffect();

    virtual void Render(int value);
    virtual bool IsReflected();
    virtual void Init();

    void Init(bool bAllocate);
    virtual void Cleanup();
    virtual void Update(float fDeltaTime);
    virtual void ReadConfig(PblConfig& config);

    static void Create();

    static void Destroy();

    static void Start(int iEmitter);
    static void Stop(int iEmitter);

    static SpaceDustEffect* s_pInstance;

    static DustEmitterData m_emitter[1];

    static float mSpawnDistance;
    static float mMaxRandomSideOffset;
    static float mCenterDeadZoneRadius;
    static float mSpawnDelay;
    static float mReferenceSpeed;
    static float mDustParticleSpeed;
    static unsigned int mTextureHash;
    static float mMinParticleScale;
    static float mMaxParticleScale;
    static float mSpeedParticleMinLength;
    static float mSpeedParticleMaxLength;
    static float mParticleLengthMinSpeed;
    static float mParticleLengthMaxSpeed;
};
 
 
 
 
 
 
 