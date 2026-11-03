#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "CommandPost.h"
#include "EntityBuildingAnimated.h"
#include "PebbleFL/Common/PblHashTable.h"

// VTABLE: SWBF2 0x007977f8 EntityBuildingAnimatedClass
// VTABLE: SWBF2 0x007977e4 CommandPostClass
class CommandBuildingClass : public EntityBuildingAnimatedClass, public CommandPostClass
{
public:
    CommandBuildingClass(unsigned int uHash);

    CommandBuildingClass(CommandBuildingClass* pBase, unsigned int uHash);
    virtual ~CommandBuildingClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static PblHash rttiHashCommandBuildingClass;
    static const char rttiNameCommandBuildingClass[];
};

   
 

// VTABLE: SWBF2 0x007975bc RedSceneObject
// VTABLE: SWBF2 0x00797624 CollisionObject
// VTABLE: SWBF2 0x00797678 EntityEx
// VTABLE: SWBF2 0x007977a0 Damageable
// VTABLE: SWBF2 0x007977cc
// VTABLE: SWBF2 0x007977f0 PblHandled
class CommandBuilding : public EntityBuildingAnimated
{
public:

    CommandBuilding(CommandBuildingClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    unsigned int GetDerivedRtti() const;
    const char* GetDerivedRttiName() const;
    virtual ~CommandBuilding();
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void Init();
    virtual void SetTeam(int iTeam);
    virtual void Kill();
    virtual void Respawn();

    EntityType GetEntityType() const;
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual GameObject* AddToClient();

    static PblHash rttiHashCommandBuilding;
    static const char rttiNameCommandBuilding[];

    CommandBuildingClass* mClass;
    CommandPost* mPost;
};
 
 
 

// FUNCTION: SWBF2 0x004760a0
inline bool CommandBuildingClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandBuildingClass == uRtti)
    {
        return true;
    }
    return EntityBuildingAnimatedClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004760f0
inline unsigned int CommandBuildingClass::GetDerivedRtti() const
{
    return rttiHashCommandBuildingClass;
}

// FUNCTION: SWBF2 0x00476100
inline const char* CommandBuildingClass::GetDerivedRttiName() const
{
    return rttiNameCommandBuildingClass;
}

// FUNCTION: SWBF2 0x00476040
inline unsigned int CommandBuilding::GetDerivedRtti() const
{
    return rttiHashCommandBuilding;
}

// FUNCTION: SWBF2 0x00476050
inline const char* CommandBuilding::GetDerivedRttiName() const
{
    return rttiNameCommandBuilding;
}

// FUNCTION: SWBF2 0x00475ff0
inline bool CommandBuilding::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandBuilding == uRtti)
    {
        return true;
    }
    return EntityBuildingAnimated::IsRtti(uRtti);
}
