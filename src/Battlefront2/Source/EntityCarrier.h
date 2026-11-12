#pragma once

#include "PebbleFL/Common/PblHashTable.h"

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
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    CargoInfo mCargoInfo[4];
    int mCargoCount;
    GameSound mSoundCargoPickup;
    GameSound mSoundCargoDropoff;

    static PblHash rttiHashEntityCarrierClass;
    static const char rttiNameEntityCarrierClass[];
};

 
 
 
 
 
 
 
 
 
 
 
 

// VTABLE: SWBF2 0x0079a168 CollisionObject
// VTABLE: SWBF2 0x0079a1bc Thread
// VTABLE: SWBF2 0x0079a28c PblHandled
// VTABLE: SWBF2 0x0079a294
// VTABLE: SWBF2 0x0079a304 Trackable
// VTABLE: SWBF2 0x0079a34c EntityEx
// VTABLE: SWBF2 0x0079a470 Damageable
// VTABLE: SWBF2 0x0079a49c RedSceneObject
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
    static const char rttiNameEntityCarrier[];

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

    static void ClearCargoParents(CargoSlot* aSlots, int iCount, const CollisionObject* pCarrier)
    {
        for (int i = 0; i < iCount; ++i)
        {
            GameObject* pObject = aSlots[i].mObject;
            if (pObject && pObject->mParent == pCarrier)
            {
                pObject->mParent = 0;
            }
        }
    }

    bool AttachCargo(int, GameObject* pGameObject);
    bool DetachCargo(int iSlot);
    void UpdateLandedHeight();
    void ActivatePhysics();

    EntityCarrierClass* GetClass() const
    {
        return (EntityCarrierClass*)mClass;
    }
};
 
 
 
 

 
 

// FUNCTION: SWBF2 0x00496e20
inline bool EntityCarrier::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityCarrier == uRtti)
    {
        return true;
    }
    return EntityFlyer::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00496e70
inline unsigned int EntityCarrier::GetDerivedRtti() const
{
    return rttiHashEntityCarrier;
}

// FUNCTION: SWBF2 0x00496e80
inline const char* EntityCarrier::GetDerivedRttiName() const
{
    return rttiNameEntityCarrier;
}

// FUNCTION: SWBF2 0x00496f00
inline bool EntityCarrierClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityCarrierClass == uRtti)
    {
        return true;
    }
    return EntityFlyerClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00496f40
inline unsigned int EntityCarrierClass::GetDerivedRtti() const
{
    return rttiHashEntityCarrierClass;
}

// FUNCTION: SWBF2 0x00496f50
inline const char* EntityCarrierClass::GetDerivedRttiName() const
{
    return rttiNameEntityCarrierClass;
}

// FUNCTION: SWBF2 0x004a9cd0
inline unsigned int EntityFlyer::GetDerivedRtti() const
{
    return rttiHashEntityFlyer;
}

// FUNCTION: SWBF2 0x004a9ce0
inline const char* EntityFlyer::GetDerivedRttiName() const
{
    return "EntityFlyer";
}

// FUNCTION: SWBF2 0x00477680
inline bool EntityFlyer::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityFlyer == uRtti)
    {
        return true;
    }
    return GameObject::IsRtti(uRtti);
}
