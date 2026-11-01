#pragma once

#include <stddef.h>

#include "Aimer.h"
#include "AmmoCounter.h"
#include "Character.h"
#include "Controllable.h"
#include "EnergyBar.h"
#include "NetPacket.h"
#include "ParticleEmitterObject.h"
#include "WeaponAreaEffect.h"
#include "WeaponCannonInstance.h"
#include "WeaponDetonator.h"
#include "WeaponDisguise.h"
#include "WeaponDispenser.h"
#include "WeaponGrenade.h"
#include "WeaponInvisibility.h"
#include "WeaponLaser.h"
#include "WeaponLauncher.h"
#include "WeaponMeleeInstance.h"
#include "WeaponMeleeThrow.h"
#include "WeaponRemote.h"
#include "WeaponTowCable.h"

class GameModel;
class Team;
class EntityBuildingArmed;
class EntityHologram;
class CommandBuilding;
class CommandWalker;
class NetObjState;
class CannonStrengthConvert;

template <class CONVERT> void WriteFixed(CONVERT& convert, NetPktGroup& group, float value);
template <class CONVERT> void ReadFixed(CONVERT& convert, NetPktGroup& group, float& value);
class WeaponMelee;

void BeginNetLOD();
Team* ReadTeam(NetPktGroup& group);
void WriteString(NetPktGroup* group, const char* text, unsigned int capacity);
void ReadString(NetPktGroup& group, char* text, unsigned int capacity);

void __fastcall PropagateTeam(Character* pCharacter, int iTeam);
void WriteEntityClass(NetPktGroup& group, const void* pValue);
void WriteWeaponClass(NetPktGroup& group, const void* pValue);

class NetTrigger : public Trigger
{
public:
    void Write(NetPktGroup& group);
    void Read(NetPktGroup& group);
};

void WriteTrigger(NetPktGroup& group, Trigger* pTrigger);
void ReadTrigger(NetPktGroup& group, Trigger* pTrigger);
void WriteSwitch(NetPktGroup& group, float value);
void ReadSwitch(NetPktGroup& group, float& value);
void WriteJetFuel(NetPktGroup& group, float value);
void ReadJetFuel(NetPktGroup& group, float& value);

#include "GameObject.h"

unsigned int GetUniqObjectId(const GameObject* pObject);
int GetUniqObjectBits();
GameObject* FindClientObject(unsigned int uId);
