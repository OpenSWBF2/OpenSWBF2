#pragma once

#include "PebbleFL/Common/PblHashTable.h"

#include <stddef.h>

#include "EntityGeometry.h"
#include "GameSound.h"
#include "RedEngineFL/Graphics/RedColor.h"

class AttachedEffects;
class EntityCloth;
class GameModel;
class RedModel;
class RedShadowProxy;

// VTABLE: SWBF2 0x0079c7f4
class EntityPropClass : public EntityGeometryClass
{
public:

    struct AttachedOdfData
    {
        PblMatrix mMatrix;
        EntityClass* mClass;
        float mDistance;
    };

    EntityPropClass(unsigned int uHash);

    EntityPropClass(EntityPropClass* pBase, unsigned int uHash);

    virtual ~EntityPropClass();
    virtual EntityClass* Derive(unsigned int uHash);
    virtual Entity* Build(const EntityDesc& desc);
    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);

    static PblHash rttiHashEntityPropClass;
    static const char rttiNameEntityPropClass[];

    bool m_bDynamicLighting;
    RedModel* mDestroyedModel;
    RedModel* mLowResModel;
    AttachedOdfData* mAttachedOdfs;
    int mNumAttachedOdfs;
    RedColor mGeometryColorMin;
    RedColor mGeometryColorMax;
    float mSwingAmount;
    float mSwingSpeed;
    GameSound mAttachedSound;
    GameSound mStartMovementSound;
    GameSound mStopMovementSound;
    PblVector3 mSoundOffset;
    bool mUseHSVFlag : 1;
    bool mReflected : 1;
    bool mAttachedSoundOnlyMoving : 1;
};
  
 
 
 
 
 
 
 
 
 
 
 
 
 
 

void ConvertHSVtoRGB(float* pR, float* pG, float* pB, float fH, float fS, float fV);

// VTABLE: SWBF2 0x0079c740 EntityEx
// VTABLE: SWBF2 0x0079c7a0 CollisionObject
// VTABLE: SWBF2 0x0079c844 RedSceneObject
class EntityProp : public EntityGeometry
{
public:

    void AddToClient();

    EntityProp(EntityPropClass* pClass, const EntityDesc& desc);

    virtual bool IsRtti(unsigned int uRtti) const;
    virtual unsigned int GetDerivedRtti() const;
    virtual const char* GetDerivedRttiName() const;
    virtual ~EntityProp();
    virtual void SetProperty(unsigned int uProperty, const char* pszValue);
    virtual void Kill();

    virtual void Render(unsigned char ucViewport, float fDist, unsigned int uiFlags);
    virtual const PblMatrix& GetBBoxMatrix() const;
    virtual const PblBox& GetBBox() const;

    virtual void RecalculateAABB();

    void UpdateSound();

    static PblHash rttiHashEntityProp;

    // FUNCTION: SWBF2 0x00497e90
    static unsigned int GetRtti() { return rttiHashEntityProp; }
    static const char rttiNameEntityProp[];

    EntityCloth* mClothPiece[8];
    PblVector3 mPrevPos;
    RedShadowProxy* mShadowProxy;
    RedColor mGeometryColor;
    GameSoundControllable mAttachedSound;
    EntityPropClass* mClass;
    bool mMoving : 1;
};
 
 
 
 
 
 
 
 
 
 

// FUNCTION: SWBF2 0x00497e50
inline bool EntityProp::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityProp == uRtti)
    {
        return true;
    }
    return reinterpret_cast<const EntityGeometry*>(this)->EntityGeometry::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x00497ef0
inline bool EntityPropClass::IsRtti(unsigned int uRtti) const
{
    if (rttiHashEntityPropClass == uRtti)
    {
        return true;
    }
    return EntityClass::IsRtti(uRtti);
}

// FUNCTION: SWBF2 0x004d8d50
inline unsigned int EntityProp::GetDerivedRtti() const
{
    return rttiHashEntityProp;
}

// FUNCTION: SWBF2 0x004d8d60
inline const char* EntityProp::GetDerivedRttiName() const
{
    return rttiNameEntityProp;
}

// FUNCTION: SWBF2 0x004d8da0
inline unsigned int EntityPropClass::GetDerivedRtti() const
{
    return rttiHashEntityPropClass;
}

// FUNCTION: SWBF2 0x004d8db0
inline const char* EntityPropClass::GetDerivedRttiName() const
{
    return rttiNameEntityPropClass;
}
