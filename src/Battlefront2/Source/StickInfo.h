#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblVector.h"
#include "PebbleFL/Common/PblQuaternion.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"
#include "GameObject.h"

class CollisionObject;
class GameObject;

class StickInfo
{
public:
    static void SetupMemoryPool(unsigned int uCount, unsigned int uSize, unsigned int uGrow) { sMemoryPool.Setup(uCount, uSize, uGrow); }

    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    bool IsStuckTo(const CollisionObject* pOther) const
    {
        return pOther && mCollObject == pOther && mCollKey == CollisionObject::LastCollidedKey(pOther) &&
               (!mCollHandle.mObject || mCollHandle.IsLive());
    }

    // SYNTHETIC: SWBF2 0x004d0880
    // StickInfo::StickInfo

    PblHandle<GameObject> mCollHandle;
    CollisionObject* mCollObject;
    int mCollBodyID;
    PblVector3 mLocalPosition;
    PblQuaternion mLocalRotation;
    unsigned int mCollKey;

    static MemoryPool sMemoryPool;
};

 
 
