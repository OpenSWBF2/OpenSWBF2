#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblVector.h"
#include "PebbleFL/Common/PblQuaternion.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class CollisionObject;
class GameObject;

class StickInfo
{
public:

    static void* operator new(size_t uSize) { return sMemoryPool.Allocate(uSize); }
    static void operator delete(void* pBlock) { sMemoryPool.Free(pBlock); }

    // SYNTHETIC: SWBF2 0x004d0880
    // StickInfo::StickInfo

    PblHandle<GameObject> mCollHandle;
    CollisionObject* mCollObject;
    int mCollBodyID;
    PblVector3 mLocalPosition;
    PblQuaternion mLocalRotation;

    static MemoryPool sMemoryPool;
};

 
 
