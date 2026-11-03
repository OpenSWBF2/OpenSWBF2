#pragma once

#include "EventManager_Timer.h"
#include "Character.h"
#include "Controllable.h"
#include "Entity.h"

class FlagItem;
class RedRegion;
class EntityClass;

namespace EventManager
{

template <>
bool LuaPushItem<FlagItem>(FlagItem& flag);

template <>
bool LuaPushItem<Character>(Character& character);

class CharacterMatchesName
{
public:

    CharacterMatchesName(const char* pcName) : mId(PblHash(pcName))
    {
    }

    static const char* GetName() { return "Name"; }

    bool operator()(const FlagItem&, const Character& character)
    {
        const Controllable* pControllable = character.GetControllable();
        if (!pControllable)
        {
            return false;
        }
        const EntityEx* pEntity = (const EntityEx*)pControllable->GetEntity();
        if (!pEntity)
        {
            return false;
        }
        return pEntity->mId == mId;
    }

    bool operator()(const RedRegion&, const Character& character)
    {
        const Controllable* pControllable = character.GetControllable();
        if (!pControllable)
        {
            return false;
        }
        const EntityEx* pEntity = (const EntityEx*)pControllable->GetEntity();
        if (!pEntity)
        {
            return false;
        }
        return pEntity->mId == mId;
    }

    bool operator==(const CharacterMatchesName& other) const
    {
        return mId == other.mId;
    }

private:
    unsigned int mId;
};

class CharacterMatchesTeam
{
public:

    CharacterMatchesTeam(int iTeam) : mTeam(iTeam) {}

    static const char* GetName() { return "Team"; }

    bool operator()(const FlagItem&, const Character& character) const
    {
        return character.mTeamNumber == mTeam;
    }

    bool operator()(const RedRegion&, const Character& character) const
    {
        return character.mTeamNumber == mTeam;
    }

    bool operator==(const CharacterMatchesTeam& other) const
    {
        return mTeam == other.mTeam;
    }

private:
    int mTeam;
};

class CharacterMatchesClass
{
public:

    CharacterMatchesClass(const char* pcName) : mClass(EntityClass::Find(PblHash(pcName)))
    {
    }

    static const char* GetName() { return "Class"; }

    bool operator()(const FlagItem&, const Character& character)
    {
        const Controllable* pControllable = character.GetControllable();
        if (!pControllable)
        {
            return false;
        }
        const Entity* pEntity = pControllable->GetEntity();
        if (!pEntity)
        {
            return false;
        }
        return pEntity->GetEntityClass() == mClass;
    }

    bool operator()(const RedRegion&, const Character& character)
    {
        const Controllable* pControllable = character.GetControllable();
        if (!pControllable)
        {
            return false;
        }
        const Entity* pEntity = pControllable->GetEntity();
        if (!pEntity)
        {
            return false;
        }
        return pEntity->GetEntityClass() == mClass;
    }

    bool operator==(const CharacterMatchesClass& other) const
    {
        return mClass == other.mClass;
    }

private:
    EntityClass* mClass;
};

 
 
 

}
