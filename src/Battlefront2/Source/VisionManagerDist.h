#pragma once

#include "CollisionObject.h"
#include "Controllable.h"
#include "Controller.h"
#include "Entity.h"
#include "EntityBuildingArmed.h"
#include "EntityFlyer.h"
#include "GameObject.h"
#include "VisionManager.h"

namespace VisionManager
{
    inline void MaxVisibleDistTerms(const GameObject* pSeer, const GameObject* pTarget, float& fDist,
                                    float& fModifier)
    {
        fDist = pTarget->GetCollisionObject()->GetCollisionRadius() * 20.0f;
        fModifier = mGlobalVisibilityModifier;
        if (pTarget->GetControllable() && pTarget->GetControllable()->mCtrl)
        {
            fModifier *= pTarget->GetControllable()->mCtrl->mTotalVisFactor;
        }
        if (pTarget->GetControllable() && pTarget->GetControllable()->GetPlayerId() >= 0)
        {
            fModifier *= 2.0f;
        }
        if (pSeer)
        {
            if (pSeer->GetEntity()->IsRtti(EntityFlyer::rttiHashEntityFlyer))
            {
                fModifier *= 2.0f;
            }
            else if (pSeer->GetEntity()->IsRtti(EntityBuildingArmed::rttiHashEntityBuildingArmed))
            {
                fModifier *= 10.0f;
            }
        }
    }
}
