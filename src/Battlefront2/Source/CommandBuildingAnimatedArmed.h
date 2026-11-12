#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "CommandPost.h"
#include "EntityBuildingAnimatedArmed.h"

// VTABLE: SWBF2 0x00797888 EntityBuildingAnimatedArmedClass
// VTABLE: SWBF2 0x00797b08 CommandPostClass
class CommandBuildingAnimatedArmedClass : public EntityBuildingAnimatedArmedClass,
                                          public CommandPostClass
{
public:
    CommandBuildingAnimatedArmedClass(unsigned int uHash);

    CommandBuildingAnimatedArmedClass(CommandBuildingAnimatedArmedClass* pBase, unsigned int uHash);

    virtual ~CommandBuildingAnimatedArmedClass();

    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static PblHash rttiHashCommandBuildingAnimatedArmedClass;
    static const char rttiNameCommandBuildingAnimatedArmedClass[];
};

   
 

// VTABLE: SWBF2 0x007978d4 Damageable
// VTABLE: SWBF2 0x00797900 CollisionObject
// VTABLE: SWBF2 0x00797954 EntityEx
// VTABLE: SWBF2 0x00797a88
// VTABLE: SWBF2 0x00797aa0 RedSceneObject
// VTABLE: SWBF2 0x00797b14 PblHandled
class CommandBuildingAnimatedArmed : public EntityBuildingAnimatedArmed
{
public:

    CommandBuildingAnimatedArmed(CommandBuildingAnimatedArmedClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~CommandBuildingAnimatedArmed();
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void Init();
    virtual void SetTeam(int iTeam);
    virtual EntityType GetEntityType() const;
    virtual void Kill();
    virtual void Respawn();
    virtual void WriteObject(NetPktGroup& group, NetObjState* pState);
    virtual void ReadObject(NetPktGroup& group, NetObjState* pState);
    virtual GameObject* AddToClient();

    static PblHash rttiHashCommandBuildingAnimatedArmed;
    static const char rttiNameCommandBuildingAnimatedArmed[];

    CommandBuildingAnimatedArmedClass* mClass;
    CommandPost* mPost;
};
 
 
 

// FUNCTION: SWBF2 0x004767d0
inline bool CommandBuildingAnimatedArmed::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandBuildingAnimatedArmed == uRtti)
    {
        return true;
    }
    return EntityBuildingAnimatedArmed::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00476830
inline unsigned int CommandBuildingAnimatedArmed::GetDerivedRtti() const
{
    return rttiHashCommandBuildingAnimatedArmed;
}

// FUNCTION: SWBF2 0x00476840
inline const char* CommandBuildingAnimatedArmed::GetDerivedRttiName() const
{
    return rttiNameCommandBuildingAnimatedArmed;
}

// FUNCTION: SWBF2 0x00476890
inline bool CommandBuildingAnimatedArmedClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashCommandBuildingAnimatedArmedClass == uRtti)
    {
        return true;
    }
    return EntityBuildingAnimatedArmedClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004768e0
inline unsigned int CommandBuildingAnimatedArmedClass::GetDerivedRtti() const
{
    return rttiHashCommandBuildingAnimatedArmedClass;
}

// FUNCTION: SWBF2 0x004768f0
inline const char* CommandBuildingAnimatedArmedClass::GetDerivedRttiName() const
{
    return rttiNameCommandBuildingAnimatedArmedClass;
}
