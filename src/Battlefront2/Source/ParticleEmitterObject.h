#pragma once

#include <stddef.h>

#include "ParticleEmitterInfo.h"
#include "FLEffect.h"
#include "RedEngineFL/Util/RedSceneObject.h"

class ParticleEmitter;
class ParticleEmitterClass;
class RedColorValue;

// VTABLE: SWBF2 0x007ad28c PblHandled
// VTABLE: SWBF2 0x007ad294
// VTABLE: SWBF2 0x007ad308 Thread
class ParticleEmitterObject : public FLEffectObject, public RedSceneObject
{
public:

    ParticleEmitterObject();
private:

    ParticleEmitterObject(const ParticleEmitterClass* pClass);
public:
    virtual ~ParticleEmitterObject();

    virtual bool IsEffectActive() const;
    virtual void ActivateEffect();
    virtual void DeactivateEffect();
    virtual void StopAndFinish();
    virtual bool Update(float fDeltaTime);
    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);
    virtual void Render_(unsigned char ucViewport, float fDist, unsigned int uiFlags);
    virtual bool IsReflected() const;

    void StopNow();
    void DeleteEmitter();

    void ScaleMultiply(float fScale);

    void ScaleSet(float fScale);

    void SetEmitterColor(const RedColorValue& color);

    void Reset(const ParticleEmitterClass* pClass, unsigned int uSeed);
    void SetChildFlag();
    // FUNCTION: SWBF2 0x006371c0
    void PermaRemoveParticleEffect()
    {
        RedSceneObject::RemoveFromScene();
        mbAddedToScene = false;
    }
    void SetInheritVelocity(float fVelocity);

    static ParticleEmitterObject* Create(const ParticleEmitterClass* pClass);

    static void* operator new(size_t size) { return sMemoryPool.Allocate((unsigned int)size); }
    // SYNTHETIC: SWBF2 0x00609d70
    // ParticleEmitterObject::operator delete
    static void operator delete(void* p) { sMemoryPool.Free(p); }

    static void SetupMemoryPool(unsigned int uCount, unsigned int uSize, unsigned int uGrow) { sMemoryPool.Setup(uCount, uSize, uGrow); }
    static MemoryPool sMemoryPool;

    void RegisterStep(float fDeltaTime);

    static bool sForceRegisterStep;

    ParticleEmitter* mEmitter;
    ParticleEmitterInfoData* mEmitterInfoData;
    float mRegisterTime;
    float mLifeTime;
    float mEmitterBoundingRadius;
    PblVector3 mMidPos;
    PblVector3 mMidSize;
    bool mStopFlag;
    bool bIsEmitterEmitter;
    bool mbAddedToScene;
    bool mIsChildPEO;
    bool mReflectionEnable;
};
 
 
 
 
