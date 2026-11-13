#pragma once

#include "CollisionObjectType.h"

class FoleyFX;
class FoleyFXCollidee;
class FoleyFXColliderClass;
class FoleyFXGroup;
class PblVector3;

class FoleyFXCollider
{
public:

    FoleyFXCollider(FoleyFXColliderClass* pClass);

    FoleyFX* GetFoleyFX(unsigned int uRtti) const;

    FoleyFXColliderClass* GetFoleyFXColliderClass() const;
    bool CollisionCallback(CollisionObjectType eType,
                           FoleyFXCollidee* const pCollidee,
                           const PblVector3* const pPos);

    void SetupFoleyFX(FoleyFXGroup* const pGroup);

    FoleyFXColliderClass* mColliderClass;
    FoleyFX* mFoleyFX;
    FoleyFXGroup* mFoleyFXGroup;
};
