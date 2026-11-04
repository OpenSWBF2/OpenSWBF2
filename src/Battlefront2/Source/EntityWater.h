#pragma once

#include <stddef.h>

#include "CollisionObject.h"
#include "Thread.h"

// VTABLE: SWBF2 0x0079de48 Thread
// VTABLE: SWBF2 0x0079de60 CollisionObject
class EntityWater : public CollisionObject, public Thread
{
public:
    EntityWater();
    virtual ~EntityWater();

    virtual const PblVector3& GetPosition() const;
    virtual bool RayTest(const PblVector3& vStart, const PblVector3& vEnd, float fMaxT);
    virtual const PblMatrix& GetMatrix() const;
    virtual float RayHit(const PblVector3& vStart, const PblVector3& vDir, float fLength, PblVector3* pNormal,
                         int* pBodyId, bool bOrdnance);
    virtual const CollisionObject* GetCollisionObject() const;
    virtual CollisionObject* GetCollisionObject();

    virtual bool Update(float fDeltaTime);

    float totalTime;
};
 
 
