#pragma once

#include <stddef.h>

#include "EntityFlyer.h"
#include "GameLoop.h"
#include "GameSound.h"
#include "RedEngineFL/Graphics/RedPose.h"
#include "SoundFL/source/sndsoundparameterized.h"
#include "Zephyr/Source/ZephyrPoseDyn.h"
#include "Zephyr/Source/ZephyrSkeleton.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class Aimer;
class ControllableDestination;
class EnergyBar;
class MountedTurret;
class Obstacle;
class ParticleEmitterObject;
class PassengerSlot;
class RedPose;
class Weapon;

// VTABLE: SWBF2 0x0079a2b8
class EntityCarrierClass : public EntityFlyerClass
{
public:

    struct CargoInfo
    {
        unsigned int mHash;
        PblVector3 mOffset;
    };

    EntityCarrierClass(unsigned int uHash);
    virtual ~EntityCarrierClass();

    EntityCarrierClass(EntityCarrierClass* pSrc, unsigned int uHash);

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti);
    virtual unsigned int GetDerivedRtti();
    virtual const char* GetDerivedRttiName();
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    CargoInfo mCargoInfo[4];
    int mCargoCount;
    GameSound mSoundCargoPickup;
    GameSound mSoundCargoDropoff;

    static PblHash rttiHashEntityCarrierClass;
    static char rttiNameEntityCarrierClass[];
};

class EntityCarrier : public EntityFlyer
{
public:
    struct CargoSlot
    {
        PblVector3 mOffset;
        PblHandle<GameObject> mObject;

        // FUNCTION: SWBF2 0x00496ed0
        CargoSlot() : mObject() {}

        // FUNCTION: SWBF2 0x00496ef0
        ~CargoSlot() {}
    };

    CargoSlot mCargoArray[4];
    int mCargoCount;

    char _tailPad[0xc];

    static PblHash rttiHashEntityCarrier;
    static char rttiNameEntityCarrier[];

    static MemoryPool sMemoryPool;

    EntityCarrier(EntityCarrierClass* pClass, const EntityDesc& desc);
    virtual ~EntityCarrier();
    virtual void Kill();
    virtual bool Update(float fDeltaTime);

    static void* operator new(size_t uSize);
    // FUNCTION: SWBF2 0x00496e90
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    bool IsRtti(unsigned int uRtti) const;
    unsigned int GetDerivedRtti() const;
    const char* GetDerivedRttiName() const;

    bool AttachCargo(int, GameObject* pGameObject);
    bool DetachCargo(int iSlot);
    void UpdateLandedHeight();
    void ActivatePhysics();

    EntityCarrierClass* GetClass() const
    {
        return static_cast<EntityCarrierClass*>(mClass);
    }
};
