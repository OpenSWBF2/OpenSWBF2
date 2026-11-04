#pragma once

#include "EventManager_LuaUser.h"
#include "EventInfo.h"
#include "Damageable.h"
#include "Character.h"
#include "Controllable.h"
#include "Entity.h"

class Character;
class GameObject;
class EntityClass;

namespace EventManager
{

int DummyLuaCallback(lua_State* L);

template <>
bool LuaPushItem<Character>(Character& character);

template <>
bool LuaPushItem<DamageOwner>(DamageOwner& owner);
template <>
bool LuaPushItem<GameObject>(GameObject& object);

template <>
bool LuaPushItem<AI::EventType>(AI::EventType& eType);

template <class PARAM>
class CharacterMatchesName
{
public:

    CharacterMatchesName(const char* pcName) : mId(PblHash(pcName))
    {
    }

    static const char* GetName() { return "Name"; }

    bool operator()(const Character& character, const PARAM&)
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

template <class PARAM>
class CharacterMatchesTeam
{
public:

    CharacterMatchesTeam(int iTeam) : mTeam(iTeam) {}

    static const char* GetName() { return "Team"; }

    bool operator()(const Character& character, const PARAM&) const
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

template <class PARAM>
class CharacterMatchesClass
{
public:

    CharacterMatchesClass(const char* pcName) : mClass(EntityClass::Find(PblHash(pcName)))
    {
    }

    static const char* GetName() { return "Class"; }

    bool operator()(const Character& character, const PARAM&)
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

 
 

namespace CharacterEvent
{
extern Event<Character, DamageOwner> mDeath;
extern Event<Character, char> mSpawn;
extern Event<Character, char> mChangeClass;
extern Event<Character, GameObject> mDispensePowerup;
extern Event<Character, GameObject> mDispenseControllable;
extern Event<Character, GameObject> mLandedFlyer;
extern Event<Character, GameObject> mEnterVehicle;
extern Event<Character, AI::EventType> mIssueAICommand;

extern Event<Character, int> mKillsChange;
extern Event<Character, int> mDeathsChange;
extern Event<Character, int> mScoreChange;

void Init();
void Cleanup();
}



}
