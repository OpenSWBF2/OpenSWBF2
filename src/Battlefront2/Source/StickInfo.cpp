#include "StickInfo.h"

#include <string.h>

#include "CollisionModel.h"
#include "EntityMine.h"
#include "RedEngineFL/Memory/RedScriptLimits.h"
#include "RedEngineFL/Util/RedLog.h"

// SYNTHETIC: SWBF2 0x004064b0
// `dynamic initializer for 'StickInfo::sMemoryPool''
// SYNTHETIC: SWBF2 0x007694f0
// `dynamic atexit destructor for 'StickInfo::sMemoryPool''

// GLOBAL: SWBF2 0x01f6deb8
// StickInfo::sMemoryPool
MemoryPool StickInfo::sMemoryPool("StickInfo", sizeof(StickInfo));

namespace
{

template <typename TYPE, unsigned int CAPACITY>
struct DenseList
{
    void Add(TYPE* pItem)
    {
        for (unsigned int i = 0; i < mCount; ++i)
        {
            if (mItems[i] == pItem)
            {
                return;
            }
        }
        if (ensure(mCount < CAPACITY))
        {
            mItems[mCount++] = pItem;
        }
    }

    void RemoveAt(unsigned int uIndex)
    {
        mItems[uIndex] = mItems[--mCount];
        mItems[mCount] = 0;
    }

    void Remove(const TYPE* pItem)
    {
        for (unsigned int i = 0; i < mCount; ++i)
        {
            if (mItems[i] == pItem)
            {
                RemoveAt(i);
                return;
            }
        }
    }

    void Clear()
    {
        memset(mItems, 0, mCount * sizeof(mItems[0]));
        mCount = 0;
    }

    TYPE* mItems[CAPACITY];
    unsigned int mCount;
};

DenseList<StickInfo, kStickListCapacity> sSticks;
DenseList<CollisionObject, kMineListCapacity> sMines;

}

void* StickInfo::operator new(size_t uSize)
{
    StickInfo* pStick = (StickInfo*)sMemoryPool.Allocate(uSize);
    if (pStick)
    {
        sSticks.Add(pStick);
    }
    return pStick;
}

void StickInfo::operator delete(void* pBlock)
{
    if (pBlock)
    {
        sSticks.Remove((StickInfo*)pBlock);
    }
    sMemoryPool.Free(pBlock);
}

void StickInfo::ForgetTarget(const CollisionObject* pTarget)
{
    for (unsigned int i = 0; i < sSticks.mCount; ++i)
    {
        StickInfo* pStick = sSticks.mItems[i];
        if (pStick->mCollObject == pTarget)
        {
            pStick->mCollObject = 0;
            pStick->mCollHandle = (GameObject*)0;
        }
    }
    for (unsigned int i = 0; i < sMines.mCount;)
    {
        CollisionObject* pMine = sMines.mItems[i];
        if (pMine->mParent == pTarget)
        {
            pMine->mParent = 0;
            sMines.RemoveAt(i);
        }
        else
        {
            ++i;
        }
    }
}

void StickInfo::TrackMine(CollisionObject* pMine)
{
    sMines.Add(pMine);
}

void StickInfo::UntrackMine(CollisionObject* pMine)
{
    sMines.Remove(pMine);
}

void StickInfo::ClearRegistries()
{
    sSticks.Clear();
    sMines.Clear();
}

bool StickInfo::FollowTarget(PblMatrix& rBody, bool& rHasBody) const
{
    const CollisionObject* pObject = mCollObject;
    if (!pObject)
    {
        return false;
    }
    const CollisionModel* pModel = pObject->mCollisionModel;
    rHasBody = pModel != 0;
    if (pModel)
    {
        rBody = pModel->GetWorldMatrix(mCollBodyID, pObject->GetMatrix(), pObject->GetPose());
    }
    return true;
}
