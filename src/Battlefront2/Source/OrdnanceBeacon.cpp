#include "OrdnanceBeacon.h"

#include <stdio.h>

#include "Ordnance.h"
#include "GameObject.h"
#include "NetGame.h"
#include "StickInfo.h"
#include "PebbleFL/Common/PblRandom.h"

#include <d3dx9math.h>

extern bool netIsLocalTurn;

// SYNTHETIC: SWBF2 0x00405a10
// `dynamic initializer for 'OrdnanceBeacon::rttiHashOrdnanceBeacon''
// OrdnanceBeacon::rttiHashOrdnanceBeacon

// GLOBAL: SWBF2 0x01ef2610
// OrdnanceBeacon::rttiHashOrdnanceBeacon
PblHash OrdnanceBeacon::rttiHashOrdnanceBeacon("OrdnanceBeacon");
// GLOBAL: SWBF2 0x0078a78c
// OrdnanceBeacon::rttiNameOrdnanceBeacon
const char OrdnanceBeacon::rttiNameOrdnanceBeacon[] = "OrdnanceBeacon";

// SYNTHETIC: SWBF2 0x00405a20
// `dynamic initializer for 'OrdnanceBeaconClass::rttiHashOrdnanceBeaconClass''
// OrdnanceBeaconClass::rttiHashOrdnanceBeaconClass

// GLOBAL: SWBF2 0x01ef257c
// OrdnanceBeaconClass::rttiHashOrdnanceBeaconClass
PblHash OrdnanceBeaconClass::rttiHashOrdnanceBeaconClass("OrdnanceBeaconClass");
// GLOBAL: SWBF2 0x0078a768
// OrdnanceBeaconClass::rttiNameOrdnanceBeaconClass
const char OrdnanceBeaconClass::rttiNameOrdnanceBeaconClass[] = "OrdnanceBeaconClass";

// FUNCTION: SWBF2 0x005f8b10
const PblVector3& OrdnanceBullet::GetOldPosition() const
{
    return mOldPosition;
}

// FUNCTION: SWBF2 0x005f8b20
const PblVector3& OrdnanceBullet::GetVelocity() const
{
    return mVelocity;
}




// SYNTHETIC: SWBF2 0x005f95d6
// ??_EOrdnanceBeacon@@WJI@AEPAXI@Z
// SYNTHETIC: SWBF2 0x005f95e1
// ??_EOrdnanceBeacon@@WBI@AEPAXI@Z
// SYNTHETIC: SWBF2 0x005f8cd0
// OrdnanceBeacon::`scalar deleting destructor'

// FUNCTION: SWBF2 0x005f8d60
OrdnanceBeacon::OrdnanceBeacon(OrdnanceBeaconClass* pClass, const OrdnanceDesc& desc)
    : OrdnanceSticky(pClass, desc)
{
    mSalvoCount = pClass->mOrdnanceClass ? pClass->mSalvoCount : 0;
}

// FUNCTION: SWBF2 0x005f8dc0
OrdnanceBeacon::~OrdnanceBeacon()
{
}

// FUNCTION: SWBF2 0x005f8de0
bool OrdnanceBeacon::Update(float aDt)
{
    while (mSalvoCount > 0 && aDt > mLifeTime)
    {
        PblMatrix matrix;
        matrix.SetRotateX(-((OrdnanceBeaconClass*)mClass)->mAimElevation);
        matrix.RotateY(((OrdnanceBeaconClass*)mClass)->mAimAzimuth);

        OrdnanceDesc desc;
        desc.mPosition = mPosition;
        desc.mPosition += ((OrdnanceBeaconClass*)mClass)->mAimDistance * matrix[2];
        desc.mPosition += (PblRandom::GetSim().GetFloat() - PblRandom::GetSim().GetFloat()) *
                          ((OrdnanceBeaconClass*)mClass)->mScatterDistance * matrix[0];
        desc.mPosition += (PblRandom::GetSim().GetFloat() - PblRandom::GetSim().GetFloat()) *
                          ((OrdnanceBeaconClass*)mClass)->mScatterDistance * matrix[1];

        desc.mDirection = -matrix[2];
        desc.mStrength = 1.0f;
        desc.mOwner = mOwner;
        desc.mDamageOwner = mDamageOwner;
        desc.mTarget = mStickInfo ? static_cast<GameObject*>(mStickInfo->mCollHandle) : 0;
        desc.mBodyID = mStickInfo ? mStickInfo->mCollBodyID : -1;

        if (NetGame::IsEnabled() && (netEnabledNext && netClientActive))
        {
            if (netIsLocalTurn)
            {
                ((OrdnanceBeaconClass*)mClass)->mOrdnanceClass->Build(desc);
            }
        }
        else
        {
            if (NetGame::IsEnabled())
            {
                NetGame::CreateEvent_CreateOrdnance(((OrdnanceBeaconClass*)mClass)->mOrdnanceClass, desc);
            }
            ((OrdnanceBeaconClass*)mClass)->mOrdnanceClass->Build(desc);
        }

        --mSalvoCount;
        mLifeTime += ((OrdnanceBeaconClass*)mClass)->mSalvoDelay;
    }
    return OrdnanceSticky::Update(aDt);
}







// FUNCTION: SWBF2 0x005f9270
OrdnanceBeaconClass::OrdnanceBeaconClass(unsigned int uHash)
    : OrdnanceStickyClass(uHash)
{
    mAimAzimuth = 0.0f;
    mAimElevation = PblMath::fHalfPi;
    mAimDistance = 100.0f;
    mSalvoCount = 1;
    mSalvoDelay = 0.0f;
    mScatterDistance = 0.0f;
    mOrdnanceClass = 0;
}

// SYNTHETIC: SWBF2 0x005f92e0
// OrdnanceBeaconClass::`scalar deleting destructor'

// FUNCTION: SWBF2 0x005f9310
OrdnanceBeaconClass::OrdnanceBeaconClass(OrdnanceBeaconClass* pBase, unsigned int uHash)
    : OrdnanceStickyClass(pBase, uHash)
{
    mAimAzimuth = pBase->mAimAzimuth;
    mAimElevation = pBase->mAimElevation;
    mAimDistance = pBase->mAimDistance;
    mSalvoCount = pBase->mSalvoCount;
    mSalvoDelay = pBase->mSalvoDelay;
    mScatterDistance = pBase->mScatterDistance;
    mOrdnanceClass = pBase->mOrdnanceClass;
}

// FUNCTION: SWBF2 0x005f9390
OrdnanceBeaconClass::~OrdnanceBeaconClass()
{
}

// FUNCTION: SWBF2 0x005f93a0
void OrdnanceBeaconClass::SetProperty(unsigned int uProperty, const char* pszValue)
{
    switch (uProperty)
    {
    case 0x3186FBFE: /* "AimAzimuth" */
        if (sscanf(pszValue, "%f", &mAimAzimuth))
        {
            mAimAzimuth *= 0.017453292f;
        }
        break;
    case 0x8339FB35: /* "AimElevation" */
        if (sscanf(pszValue, "%f", &mAimElevation))
        {
            mAimElevation *= 0.017453292f;
        }
        break;
    case 0xECDB9D1D: /* "AimDistance" */
        sscanf(pszValue, "%f", &mAimDistance);
        break;
    case 0x9260EA63: /* "SalvoCount" */
        sscanf(pszValue, "%i", &mSalvoCount);
        break;
    case 0xD76F37DB: /* "SalvoDelay" */
        sscanf(pszValue, "%f", &mSalvoDelay);
        break;
    case 0xEBD81A5E: /* "ScatterDistance" */
        sscanf(pszValue, "%f", &mScatterDistance);
        break;
    case 0x399BF05D: /* "Ordnance" */
    case 0xF8899C5E: /* "OrdnanceName" */
        mOrdnanceClass = Factory<Ordnance, OrdnanceClass, OrdnanceDesc>::Find(PblHash(pszValue));
        break;
    default:
        OrdnanceStickyClass::SetProperty(uProperty, pszValue);
        break;
    }
}

// FUNCTION: SWBF2 0x005f94d0
OrdnanceClass* OrdnanceBeaconClass::Derive(unsigned int uHash)
{
    return new OrdnanceBeaconClass(this, uHash);
}

// FUNCTION: SWBF2 0x005f9540
Ordnance* OrdnanceBeaconClass::Build(const OrdnanceDesc& desc)
{
    if (Ordnance::sMemoryPool.mUsed >= Ordnance::sMemoryPool.mCount)
    {
        return 0;
    }

    OrdnanceBeacon* pBeacon = new OrdnanceBeacon(this, desc);
    pBeacon->ActivateThread(-40, PER_LOCAL_TURN);
    pBeacon->AddOrdnanceToCollisionManager();
    return pBeacon;
}
