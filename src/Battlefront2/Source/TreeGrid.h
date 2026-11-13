#pragma once

#include "PebbleFL/Common/PblVector.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

class MemoryPool;
class TreeGridObject;
class TreeGridStack;

class TreeGridStack
{
public:

    TreeGridStack() : mStackSize(0), mNext(0) {}
    static void* operator new(size_t uiSize);
    static void operator delete(void* pBlock);
    TreeGridStack* AddObject(TreeGridObject* pObj, const PblVector3& rPos, float fSize);
    TreeGridStack* RemoveObject(TreeGridObject* pObj);

    static void SetupMemoryPool(unsigned int uCount, unsigned int uSize, unsigned int uGrow)
    {
        sMemoryPool.Setup(uCount, uSize, uGrow);
    }

    int mStackSize;
    int mData[1][15];
    PblSphere mSphere[15];
    TreeGridObject* mObject[15];
    TreeGridStack* mNext;

    static MemoryPool sMemoryPool;
};
 

class TreeGridObject
{
public:

    TreeGridObject() {}

    // FUNCTION: SWBF2 0x00463a40
    explicit TreeGridObject(int iData) : mStackPtr(0), mStackIdx(-1), mTreeGridCellIndex(-1)
    {
        mData[0] = iData;
    }

    // FUNCTION: SWBF2 0x0043a910
    const PblVector3& GetCollisionCenter() const
    {
        return mStackPtr ? mStackPtr->mSphere[mStackIdx].GetCenter() : mCollisionSphere.GetCenter();
    }

    // FUNCTION: SWBF2 0x0043a930
    const float GetCollisionRadius() const
    {
        return mStackPtr ? mStackPtr->mSphere[mStackIdx].GetRadius() : mCollisionSphere.GetRadius();
    }
    int GetObjectData(int iIndex) const
    {
        return mStackPtr ? mStackPtr->mData[iIndex][mStackIdx] : mData[iIndex];
    }

    // FUNCTION: SWBF2 0x0045dad0
    void SetObjectData(int iIndex, int iData)
    {
        if (mStackPtr)
        {
            mStackPtr->mData[iIndex][mStackIdx] = iData;
        }
        mData[iIndex] = iData;
    }

    // FUNCTION: SWBF2 0x00497e10
    const PblSphere& GetCollisionSphere() const
    {
        return mStackPtr ? mStackPtr->mSphere[mStackIdx] : mCollisionSphere;
    }

    // FUNCTION: SWBF2 0x00443f80
    void SetCollisionCenter(const PblVector3& rCenter)
    {
        if (mStackPtr)
        {
            mStackPtr->mSphere[mStackIdx].SetCenter(rCenter);
        }
        mCollisionSphere.SetCenter(rCenter);
    }
    // FUNCTION: SWBF2 0x00454da0
    void SetCollisionRadius(float fRadius)
    {
        if (mStackPtr)
        {
            mStackPtr->mSphere[mStackIdx].w = fRadius;
        }
        mCollisionSphere.w = fRadius;
        m2dRadius = fRadius;
    }
    void Set2DCollisionRadius(float fRadius) { m2dRadius = fRadius; }

    TreeGridStack* mStackPtr;
    int mStackIdx;
    PblSphere mCollisionSphere;
    float m2dRadius;
    int mData[1];
    int mTreeGridCellIndex;
};

struct TreeGridCallbackData
{
    TreeGridCallbackData() { mData[0] = -1; }

    int mData[1];
};

typedef bool (*TreeGridCallback)(const TreeGridObject*, TreeGridCallbackData*, float&);

class TreeGrid
{
public:
    struct TreeGridLayerData
    {
        float mSize;
        float mSizePerMeter;
        int mStartIndex;
        int mGridSize;
    };

    TreeGrid(const PblVector3& rMin, const PblVector3& rMax, float fMinSize);
    ~TreeGrid();

    void Add(TreeGridObject* pObj, const PblVector3& rPos, float fSize);

    void Add(TreeGridObject* pObj, const PblVector3& rPos, float fSize, float f2dSize);
    void Remove(TreeGridObject* pObj);
    void Update(TreeGridObject* pObj, const PblVector3& rPos, float fSize);
    void Update(TreeGridObject* pObj, const PblVector3& rPos, float fSize, float f2dSize);
    void CallbackOnRect(const PblVector3& rMin, const PblVector3& rMax,
                        TreeGridCallback pCallback, TreeGridCallbackData* pData);
    void CallbackOnSphere(const PblVector3& rCenter, float fRadius,
                          TreeGridCallback pCallback, TreeGridCallbackData* pData);

    void CallbackOnRay(const PblVector3& rFrom, const PblVector3& rTo,
                       TreeGridCallback pCallback, TreeGridCallbackData* pData);
    void CallbackOnLazyRay(const PblVector3& rFrom, const PblVector3& rTo,
                           TreeGridCallback pCallback, TreeGridCallbackData* pData);
    void CallbackOnFatRay(const PblVector3& rFrom, const PblVector3& rTo, float fRadius,
                          TreeGridCallback pCallback, TreeGridCallbackData* pData);

    // FUNCTION: SWBF2 0x0065adb0
    bool ProcessCell(int iCell, TreeGridCallback pCallback, TreeGridCallbackData* pData,
                     float& fDist)
    {
        for (TreeGridStack* pStack = mGrid[iCell]; pStack; pStack = pStack->mNext)
        {
            for (int i = 0; i < pStack->mStackSize; ++i)
            {
                if (!pCallback(pStack->mObject[i], pData, fDist))
                {
                    return false;
                }
            }
        }
        return true;
    }
    bool ProcessCellRect(int iCell, TreeGridCallback pCallback, TreeGridCallbackData* pData,
                         float& fDist, const PblVector3& rMin, const PblVector3& rMax);
    bool ProcessCellRay(int iCell, TreeGridCallback pCallback, TreeGridCallbackData* pData,
                        float& fDist, const PblVector3& rFrom, const PblVector3& rDir,
                        float fLength);
    bool ProcessCellSphere(int iCell, TreeGridCallback pCallback, TreeGridCallbackData* pData,
                           float& fDist, const PblVector3& rCenter, float fRadius);
    bool ProcessNeighborhood(int iX, int iZ, int iGridSize, int iStartIndex, int* piExtents,
                             TreeGridCallback pCallback, TreeGridCallbackData* pData,
                             float& fDist, bool bFull);
    void _CallbackOnRay(const PblVector3& rFrom, const PblVector3& rTo, int* piExtents,
                        TreeGridCallback pCallback, TreeGridCallbackData* pData);

    float mMaxSize;
    float mMinSize;
    float mOffset;
    float mSizePerMeter;
    int mDepth;
    int mMaxGridSize;
    int mNumCells;
    TreeGridStack** mGrid;
    TreeGridLayerData* mLayerData;
};
 
