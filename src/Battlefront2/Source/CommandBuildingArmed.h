#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "CommandPost.h"
#include "EntityBuildingArmed.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x00797cc8 EntityBuildingArmedClass
// VTABLE: SWBF2 0x00797d14 CommandPostClass
class CommandBuildingArmedClass : public EntityBuildingArmedClass, public CommandPostClass
{
public:
    CommandBuildingArmedClass(unsigned int uHash);

    CommandBuildingArmedClass(CommandBuildingArmedClass* pBase, unsigned int uHash);

    virtual ~CommandBuildingArmedClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static PblHash rttiHashCommandBuildingArmedClass;
    static const char rttiNameCommandBuildingArmedClass[];
};

   
 

// VTABLE: SWBF2 0x00797b50 Damageable
// VTABLE: SWBF2 0x00797b7c
// VTABLE: SWBF2 0x00797b94 EntityEx
// VTABLE: SWBF2 0x00797d20 PblHandled
// VTABLE: SWBF2 0x00797d28 RedSceneObject
// VTABLE: SWBF2 0x00797d90 CollisionObject
class CommandBuildingArmed : public EntityBuildingArmed
{
public:

    CommandBuildingArmed(CommandBuildingArmedClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~CommandBuildingArmed();
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void Init();
    virtual void SetTeam(int iTeam);
    virtual EntityType GetEntityType() const;
    virtual void Kill();
    virtual void Respawn();
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual GameObject* AddToClient();

    static PblHash rttiHashCommandBuildingArmed;
    static const char rttiNameCommandBuildingArmed[];

    CommandBuildingArmedClass* mClass;
    CommandPost* mPost;
};
 
 
 

// FUNCTION: SWBF2 0x00476ee0
inline bool CommandBuildingArmed::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandBuildingArmed == uRtti)
    {
        return true;
    }
    return EntityBuildingArmed::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00476f30
inline unsigned int CommandBuildingArmed::GetDerivedRtti() const
{
    return rttiHashCommandBuildingArmed;
}

// FUNCTION: SWBF2 0x00476f40
inline const char* CommandBuildingArmed::GetDerivedRttiName() const
{
    return rttiNameCommandBuildingArmed;
}

// FUNCTION: SWBF2 0x00476f90
inline bool CommandBuildingArmedClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandBuildingArmedClass == uRtti)
    {
        return true;
    }
    return EntityBuildingArmedClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00476fe0
inline unsigned int CommandBuildingArmedClass::GetDerivedRtti() const
{
    return rttiHashCommandBuildingArmedClass;
}

// FUNCTION: SWBF2 0x00476ff0
inline const char* CommandBuildingArmedClass::GetDerivedRttiName() const
{
    return rttiNameCommandBuildingArmedClass;
}
