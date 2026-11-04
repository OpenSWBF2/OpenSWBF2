#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "Controllable.h"
#include "GameObject.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class EntityRemoteTerminal;
class EntitySoldier;
class GameModel;
class RedPose;

// VTABLE: SWBF2 0x0079cca8
class EntityRemoteTerminalClass : public GameObjectClass, public ControllableClass
{
public:
    EntityRemoteTerminalClass(unsigned int uHash);

    EntityRemoteTerminalClass(EntityRemoteTerminalClass* pBase, unsigned int uHash);
    virtual ~EntityRemoteTerminalClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual int GetVehicleType() const;

    int mVehicleType;

    static PblHash rttiHashEntityRemoteTerminalClass;
    static const char rttiNameEntityRemoteTerminalClass[];
};
  
 
 

// VTABLE: SWBF2 0x0079ca9c EntityEx
// VTABLE: SWBF2 0x0079cbc4 Trackable
// VTABLE: SWBF2 0x0079cc0c Damageable
// VTABLE: SWBF2 0x0079cc38 RedSceneObject
// VTABLE: SWBF2 0x0079cca0 PblHandled
// VTABLE: SWBF2 0x0079ccf4 Thread
// VTABLE: SWBF2 0x0079cdc4 CollisionObject
class EntityRemoteTerminal : public GameObject, public Controllable
{
public:

    EntityRemoteTerminal(EntityRemoteTerminalClass* pClass, const EntityDesc& desc);
    virtual ~EntityRemoteTerminal();

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void Init();
    virtual const Entity* GetEntity() const;
    virtual Entity* GetEntity();
    virtual const GameObject* GetGameObject() const;
    virtual GameObject* GetGameObject();
    virtual float GetMaxSpeed() const;
    virtual float GetMaxStrafe() const;
    virtual const Controllable* GetControllable() const;
    virtual Controllable* GetControllable();
    virtual void ActivatePhysics();
    virtual void DeactivatePhysics();
    virtual GameObjectType GetGameObjectType();
    virtual EntityType GetEntityType() const;
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual GameObject* AddToClient();
    virtual void AddSends(void (*pfnSend)(GameObject*, int), int iArg);

    virtual const PblMatrix* GetRenderMatrix() const;
    virtual const RedPose* GetRenderPose() const;

    virtual void SetMatrix(const PblMatrix& matrix);
    virtual const PblMatrix& GetMatrix() const;

    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);

    virtual void HealthChangeCallback(bool bUnused, float fDelta);

    virtual bool Update(float fDeltaTime);
    virtual Trackable* GetTrackable();
    virtual ControllableClass* GetControllableClass();
    virtual const ControllableClass* GetControllableClass() const;
    virtual void SetupController();
    virtual void SetPilot(Controllable* pPilot);
    virtual bool UpdateIndirect(float fDeltaTime);

    GameObject* GetRemoteGameObject();
    // FUNCTION: SWBF2 0x004dcb30
    EntityRemoteTerminal* GetNextLinkedTerminal()
    {
        if (!mNextLinkedTerminal)
        {
            EntityEx* pEntity = EntityEx::Find(mNextLinkedTerminalHash);
            if (pEntity && pEntity->IsRtti(rttiHashEntityRemoteTerminal))
            {
                mNextLinkedTerminal = reinterpret_cast<EntityRemoteTerminal*>(pEntity);
            }
        }
        return mNextLinkedTerminal;
    }
    // FUNCTION: SWBF2 0x004dcb90
    EntityRemoteTerminal* GetPrevLinkedTerminal()
    {
        if (!mPrevLinkedTerminal)
        {
            EntityEx* pEntity = EntityEx::Find(mPrevLinkedTerminalHash);
            if (pEntity && pEntity->IsRtti(rttiHashEntityRemoteTerminal))
            {
                mPrevLinkedTerminal = reinterpret_cast<EntityRemoteTerminal*>(pEntity);
            }
        }
        return mPrevLinkedTerminal;
    }
    bool IsTerminalAvailable(const EntitySoldier* pSoldier);
    EntityRemoteTerminal* GetNextAvailableTerminal(const EntitySoldier* pSoldier);
    EntityRemoteTerminal* GetPrevAvailableTerminal(const EntitySoldier* pSoldier);
    void UpdateHealthRemote(float fAddHealth);

    static void* operator new(unsigned int uSize) { return sMemoryPool.Allocate(uSize); }
    // SYNTHETIC: SWBF2 0x004dc980
    // EntityRemoteTerminal::operator delete
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    static PblHash rttiHashEntityRemoteTerminal;

    // FUNCTION: SWBF2 0x0043ef30
    static unsigned int GetRtti() { return rttiHashEntityRemoteTerminal; }
    static const char rttiNameEntityRemoteTerminal[];
    static MemoryPool sMemoryPool;

    EntityRemoteTerminalClass* mClass;
    unsigned int mRemoteGameObjectHash;
    unsigned int mNextLinkedTerminalHash;
    unsigned int mPrevLinkedTerminalHash;
    PblHandle<GameObject> mRemoteGameObject;
    EntityRemoteTerminal* mNextLinkedTerminal;
    EntityRemoteTerminal* mPrevLinkedTerminal;
};
 
 
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x004dc920
inline bool EntityRemoteTerminal::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityRemoteTerminal == uRtti)
    {
        return true;
    }
    return reinterpret_cast<const GameObject*>(this)->GameObject::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004dc960
inline unsigned int EntityRemoteTerminal::GetDerivedRtti() const
{
    return rttiHashEntityRemoteTerminal;
}

// FUNCTION: SWBF2 0x004dc970
inline const char* EntityRemoteTerminal::GetDerivedRttiName() const
{
    return rttiNameEntityRemoteTerminal;
}

// FUNCTION: SWBF2 0x004dcc20
inline bool EntityRemoteTerminalClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityRemoteTerminalClass == uRtti)
    {
        return true;
    }
    return GameObjectClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004dcc60
inline unsigned int EntityRemoteTerminalClass::GetDerivedRtti() const
{
    return rttiHashEntityRemoteTerminalClass;
}

// FUNCTION: SWBF2 0x004dcc70
inline const char* EntityRemoteTerminalClass::GetDerivedRttiName() const
{
    return rttiNameEntityRemoteTerminalClass;
}


