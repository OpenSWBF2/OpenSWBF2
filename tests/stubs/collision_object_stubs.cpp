#include "Battlefront2/Source/CollisionObject.h"
#include "Battlefront2/Source/CollisionManager.h"
#include "Battlefront2/Source/CollisionModel.h"
#include "Battlefront2/Source/Controllable.h"
#include "Battlefront2/Source/EntityMine.h"
#include "Battlefront2/Source/GameSound.h"
#include "Battlefront2/Source/Restrictor.h"

PblVector3 gVectorZero;
CollisionResult CollisionModel::sCollisionResult;

FoleyFXCollidee::FoleyFXCollidee(FoleyFXCollideeClass* pClass) : mCollideeClass(pClass) {}
FoleyFXCollider::FoleyFXCollider(FoleyFXColliderClass* pClass) : mColliderClass(pClass), mFoleyFX(0), mFoleyFXGroup(0) {}
FoleyFX* FoleyFXCollider::GetFoleyFX(unsigned int) const { return 0; }
bool FoleyFXCollider::CollisionCallback(CollisionObjectType, FoleyFXCollidee* const, const PblVector3* const) { return false; }

bool CollisionManager::AddItemBody(CollisionObject*) { return true; }
bool CollisionManager::AddSoftBody(CollisionObject*) { return true; }
bool CollisionManager::AddRigidBody(CollisionObject*) { return true; }
bool CollisionManager::AddAsteroidBody(CollisionObject*) { return true; }
bool CollisionManager::AddStaticBody(CollisionObject*) { return true; }
bool CollisionManager::AddAttachedBody(CollisionObject*) { return true; }
bool CollisionManager::AddShieldBody(CollisionObject*) { return true; }
bool CollisionManager::RemoveBody(CollisionObject*) { return true; }

int CollisionModel::GetNumBodies() const { return 0; }
int gPrepareCollisionCalls;

bool CollisionModel::PrepareCollision(CollisionObject*, const CollisionModel*, CollisionObject*,
                                      const CollisionMask&, const CollisionMask&) const
{
    ++gPrepareCollisionCalls;
    return false;
}
void CollisionModel::RegisterCollision(int, int) {}
const CollisionMask& CollisionModel::GetCollisionMask(const CollisionObjectType&) const
{
    static CollisionMask sMask;
    return sMask;
}
float CollisionModel::RayHit(const PblMatrix&, const RedPose*, const PblVector3&, const PblVector3&,
                             float, PblVector3*, int*, bool, float) const
{
    return 1.0f;
}
bool CollisionModel::RayTest(const PblMatrix&, const RedPose*, const PblVector3&, const PblVector3&, float) const
{
    return false;
}

void Controllable::SetCurrentCollision(GameObject*) {}
void GameSound::Play(const PblVector3*, const PblVector3*, bool, bool) const {}
PblHash EntityMine::rttiHashEntityMine("EntityMine");
PblVector3 Restrictor::GetRestrictedMovement(const PblVector3&, float) { return gVectorZero; }
