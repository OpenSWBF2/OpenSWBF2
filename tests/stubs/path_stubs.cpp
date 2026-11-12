#include <stdlib.h>

#include "Battlefront2/Source/AIUtil.h"
#include "Battlefront2/Source/AILowLevel.h"
#include "Battlefront2/Source/AIVisRegion.h"
#include "Battlefront2/Source/Adrenaline.h"
#include "Battlefront2/Source/Cloth.h"
#include "Battlefront2/Source/CmdInfo.h"
#include "Battlefront2/Source/CollisionUtil.h"
#include "Battlefront2/Source/CommandFsm.h"
#include "Battlefront2/Source/Controller.h"
#include "Battlefront2/Source/EntitySoldier.h"
#include "Battlefront2/Source/GameLoop.h"
#include "Battlefront2/Source/GameObject.h"
#include "Battlefront2/Source/HintManager.h"
#include "Battlefront2/Source/CollisionManager.h"
#include "Battlefront2/Source/Fsm.h"
#include "Battlefront2/Source/NavigatorGraphFollower.h"
#include "Battlefront2/Source/NavigatorPathFollower.h"
#include "Battlefront2/Source/NavigatorSlide.h"
#include "Battlefront2/Source/PathFollower.h"
#include "Battlefront2/Source/PowerupItem.h"
#include "Battlefront2/Source/NetGame.h"
#include "Battlefront2/Source/Navigator.h"
#include "Battlefront2/Source/PlayerController.h"
#include "Battlefront2/Source/Thread.h"
#include "Battlefront2/Source/UnitCmdInfo.h"
#include "Battlefront2/Source/UnitController.h"
#include "Battlefront2/Source/VehicleManager.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedShader.h"
#include "RedEngineFL/Graphics/RedColor.h"
#include "RedEngineFL/Graphics/RedLight.h"
#include "RedEngineFL/Graphics/RedMaterial.h"
#include "RedEngineFL/Graphics/RedModel.h"
#include "RedEngineFL/Graphics/RedSkeleton.h"
#include "RedEngineFL/Interface/RedInterfaceScreen.h"

int WORLD_EXTENTS = 0x400;

namespace AIDifficulty
{
void UpdateAutoBalance();
}

namespace VisionManager
{
bool RayTests(int i);
}

namespace AI
{
enum AIAffil
{
    AFFIL_ENEMY = 1
};

namespace VOHelper
{
void Update();
}
}

namespace TeamManager
{
int GetObjectsInRange(const PblVector3& vPos, float fRadius, GameObject** const ppObjects,
                      int iMax, const GameObject* pIgnore, AI::AIAffil eAffil);
}

namespace PowerupUtility
{
bool CheckHealth(GameObject* pObj, float fAmount);
bool CheckAmmo(GameObject* pObj, float fAmount, bool bFlag);
}

namespace AudibleManager
{
void UpdateAudience(Controller* pController);
}

bool AIUtil::IsUberMode() { return false; }
const Obstacle* AIUtil::GetGameObjectBarrier(const GameObject*) { return 0; }
PblVector3 AIUtil::GetSafeNormalXZ(const PblVector3& rVec) { return rVec; }
float AIUtil::GetCollisionRadius(const GameObject*) { return 0.0f; }
ConnectivityGraph::ProhibitFlag AIUtil::GetAIType(const GameObject*) { return (ConnectivityGraph::ProhibitFlag)0; }

bool CollisionUtil::CollideSphereAABB(const PblVector3&, float, const PblVector3&, const PblVector3&) { return false; }
bool CollisionUtil::CollideSphereRay(const PblVector3&, float, const PblVector3&, const PblVector3&, float) { return false; }
bool CollisionUtil::CollideSpheres2(const PblVector3&, float, const PblVector3&, float) { return false; }

bool VisionManager::RayTests(int) { return false; }
void AIDifficulty::UpdateAutoBalance() {}
void AudibleManager::UpdateAudience(Controller*) {}
float GameLoop::GetMissionTime() { return 0.0f; }
int TeamManager::GetObjectsInRange(const PblVector3&, float, GameObject** const, int, const GameObject*, AI::AIAffil) { return 0; }

bool CommandFsm::IsNewCommand() { return false; }
CmdInfo::~CmdInfo() {}
UnitCmdInfo::UnitCmdInfo(UnitControllerCommand, PblHandle<GameObject>, const PblVector3&, PblHandle<GameObject>) {}
const PblList<Controllable>& GameObject::GetControllableList() const
{
    static PblList<Controllable> sList;
    return sList;
}
int UnitController::SendEvent(AI::EventInfo*) { return 0; }
void UnitController::AddFriendlyFire(const GameObject*) {}
void UnitController::ResetFriendlyList() {}
void UnitController::UpdateHighLevel() {}
void UnitController::UpdateLowLevel(float) {}
void Controller::UpdateFreeSpace() {}
void PlayerController::LogVODeath(int, int) {}
void Adrenaline::Update(float, const GameObject*) {}
unsigned char AILowLevel::GetPathQueueDepth() { return 0; }
void AIVisRegionManager::UpdateNextRegion() {}
void AI::VOHelper::Update() {}
void VehicleManager::Update() {}
void* AIPath::operator new(unsigned int uSize) { return malloc(uSize); }

PblHash EntitySoldier::rttiHashEntitySoldier("EntitySoldier");
PblHash PlayerController::rttiHashPlayerController("PlayerController");
PblHash UnitController::rttiHashUnitController("UnitController");
PblHash EntityClass::rttiHashEntityClass("EntityClass");

EntityClass::EntityClass(unsigned int uHash) : Factory<Entity, EntityClass, EntityDesc>(uHash) {}
EntityClass::~EntityClass() {}
void EntityClass::PostReadSetup() {}

Thread::Thread() {}
Thread::~Thread() {}
void Thread::ActivateThread(int, ThreadType) {}
void Thread::DeactivateThread() {}

void EntityCloth::PlatformRender(unsigned char, RedColor, unsigned int, const RedLightingState*, const PblSphere&) {}
void ClothData::PlatformCleanup() {}
void ClothData::PlatformBindInit(const EntityClothClass*) {}
namespace AIDebug { bool PauseAfterPathFind() { return false; } }
float NetGame::GetTurnRatio() { return 1.0f; }

RedColor RedColor::WHITE;
RedColor RedColor::BLACK;
PblHashTable<RedModel, 1024> RedModel::_HashTable;
RedLightingState RedInterfaceScreen::s_lightingState;
bool RedModel::GetBoneMatrix(unsigned int, PblMatrix&) const { return false; }
PblMatrix* RedSkeleton::GetBoneMatrixByName(unsigned int) const { return 0; }
PblMatrix* RedSkeleton::GetInvBoneMatrixByName(unsigned int) const { return 0; }
int RedRenderer::GetFrameNumber() { return 0; }
const RedLightingState* RedLight::GetLightingState(const PblSphere&, int, bool, bool, bool) { return 0; }
pcRedShader* __cdecl pcRedShader::Create(unsigned int, unsigned int, const unsigned int*, unsigned int, unsigned int,
                                         const RedColor&, const RedColor&, float)
{
    return 0;
}
const char EntityClass::rttiNameEntityClass[] = "EntityClass";
void EntityClass::SetProperty(unsigned int, const char*) {}
CmdInfo::CmdInfo() {}
RedColorValue RedColorValue::WHITE;
RedLight::~RedLight() {}
void RedLight::Activate() {}
void RedLight::Deactivate() {}
bool AIUtil::IsHintAvailable(const GameObject*, const BaseHint*) { return false; }
AIUtil::EntityType AIUtil::GetEntityType(const Entity*) { return (AIUtil::EntityType)0; }
float AIUtil::GetRandomFloat(float fMin, float) { return fMin; }
bool PowerupUtility::CheckHealth(GameObject*, float) { return false; }
bool PowerupUtility::CheckAmmo(GameObject*, float, bool) { return false; }
int HintManager::RequestCoverHints(const UnitController*, const PblVector3&, int, int, const BaseHint**) { return 0; }
const PblVector3& BaseHint::GetZAxis() const
{
    static PblVector3 sAxis;
    return sAxis;
}
void BaseHint::Reserve(const GameObject*) const {}
int CollisionManager::GetObjectsInRange(const PblVector3&, float, GameObject** const, int, const GameObject*, int) { return 0; }
Fsm::Fsm() {}
Fsm::~Fsm() {}
void Fsm::Kill() {}
bool Fsm::IsDone() const { return false; }
void Fsm::FSMUpdate(float, bool) {}
void Fsm::SetNextState(int) {}
PathFollower::~PathFollower() {}
bool PathFollower::PrependPath(const AIPath*) { return false; }
void PathFollower::UpdateControllable() {}
MemoryPool PathFollower::sMemoryPool("PathFollower", sizeof(PathFollower));
PblHash NavigatorGraphFollower::rttiHashNavigatorGraphFollower("NavigatorGraphFollower");
PblHash NavigatorPathFollower::rttiHashNavigatorPathFollower("NavigatorPathFollower");
PblHash NavigatorSlide::rttiHashNavigatorSlide("NavigatorSlide");
PblHash PowerupItem::rttiHashPowerupItem("PowerupItem");
