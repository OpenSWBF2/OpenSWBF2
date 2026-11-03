#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblVector.h"

class Restrictor
{
public:

    Restrictor(int iMaxRestrictions) : mNumRestrictions(0), mMaxRestrictions(iMaxRestrictions) {}
    void Clear() { mNumRestrictions = 0; }

    // FUNCTION: SWBF2 0x00463a80
    bool AddRestriction(const PblVector3& vRestriction)
    {

        if (mNumRestrictions < mMaxRestrictions)
        {
            mRestrictions[mNumRestrictions++] = vRestriction;
            return true;
        }
        return false;
    }

    PblVector3 RestrictMovement(const PblVector3& vDir, float& fDist);
    PblVector3 GetRestrictedMovement(const PblVector3& vDir, float fDist);
    void RestrictMovement(PblVector3& vMovement);

    PblVector3 mRestrictions[20];
    int mNumRestrictions;
    int mMaxRestrictions;
    float mHeightOffTerrain;
};
 
 
 
