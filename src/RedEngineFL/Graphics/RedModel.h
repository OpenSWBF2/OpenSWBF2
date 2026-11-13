#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblVector.h"

class PblFileChunk;
class PblVector3;
class RedColor;
class RedPose;
class RedSegment;
class RedShadingState;
class RedShadingPose;
class RedSkeleton;
class RedStencilShadow;

// VTABLE: SWBF2 0x00772b00
class RedModel
{
public:
    static PblHashTable<RedModel, 1024> _HashTable;

    static const RedModel* Find(unsigned int uiName);

    static void ClearAll() { _HashTable.Clear(); }

    RedModel();
    virtual ~RedModel();
    virtual void Render(const PblMatrix* pMatrix, const RedPose* pPose, const RedColor& color,
                        unsigned int uiFlags, RedShadingPose* pShadingPose) const;
    virtual void Init();

    unsigned int GetName() const { return _uiNameHash; }

    static const RedModel* GetFirst();
    static const RedModel* GetLast();
    static const RedModel* GetNext(const RedModel* const pModel);
    static const RedModel* GetPrev(const RedModel* const pModel);

    const RedSkeleton* GetSkeleton() const { return _pSkeleton; }

    unsigned int GetParentBoneAndOffsetMatrix(unsigned int uiBone, PblMatrix& mat) const;

    bool GetBoneMatrix(unsigned int uiBone, PblMatrix& mat) const;
    bool GetHierarchicalBoneMatrix(unsigned int uiBone, PblMatrix& mat) const;
    unsigned int GetParentBone(unsigned int uiBone) const;
    unsigned int GetParentBoneAndOffset(unsigned int uiBone, PblVector3& vOffset) const;

    RedShadingState* CreateShadingState(unsigned int uiMaterialNameHash) const;
    RedShadingState* CreateDefaultShadingState(unsigned int uiRenderTypeHash) const;
    bool HasTransparentSegment() const;

    static RedModel* Read(PblFileChunk& chunk);

    void SetupPose(RedPose& pose) const;

    unsigned int GetBoneIndex(const char* pcName) const;
    unsigned int GetBoneIndex(unsigned int uiName) const;

    static bool pc_shareBuffers;
    static bool pcGetShareBufferEnable() { return pc_shareBuffers; }

    static void pcSetShareBufferEnable(bool bEnable) { pc_shareBuffers = bEnable; }

private:

    void _Read(PblFileChunk& chunk);

public:

    PblMatrix _Matrix;
    bool _bHasTransform;
    unsigned int _uiNameHash;
    unsigned int _uiFlags;
    unsigned int _uiDrawListBytes;
    PblBox _BoundingBox;

    PblSphere _BoundingSphere;

    PblBox _TotalBoundingBox;
    RedSegment** _apSegments;

    int _iNumSegments;
    unsigned int _uiNumStripVtx;
    bool _bHardSkin;
    bool _bNoProjTexture;

    RedSkeleton* _pSkeleton;
    int _iNumBones;

    RedStencilShadow* _pStencilShadow;
    int _iRefCount;
};

// FUNCTION: SWBF2 0x00411c50
inline const RedModel* RedModel::Find(unsigned int uiName)
{
    return _HashTable.Find(uiName);
}

inline const RedModel* RedModel::GetFirst()
{
    return PblHashTable<RedModel, 1024>::Itor(_HashTable);
}

// FUNCTION: SWBF2 0x00544950
inline const RedModel* RedModel::GetLast()
{
    const RedModel* pLast = 0;
    for (PblHashTable<RedModel, 1024>::Itor it(_HashTable); (RedModel*)it; ++it)
    {
        pLast = it;
    }
    return pLast;
}

// FUNCTION: SWBF2 0x005449c0
inline const RedModel* RedModel::GetNext(const RedModel* const pModel)
{
    bool bFound = false;
    PblHashTable<RedModel, 1024>::Itor it(_HashTable);
    while ((RedModel*)it && !bFound)
    {
        bFound = (pModel == it);
        ++it;
    }
    const RedModel* pNext = it;
    if (!pNext)
    {
        pNext = PblHashTable<RedModel, 1024>::Itor(_HashTable);
    }
    return pNext;
}

// FUNCTION: SWBF2 0x00544a60
inline const RedModel* RedModel::GetPrev(const RedModel* const pModel)
{
    const RedModel* pPrev = pModel;
    for (PblHashTable<RedModel, 1024>::Itor it(_HashTable); (RedModel*)it; ++it)
    {
        if (pModel == it)
        {
            break;
        }
        pPrev = it;
    }
    return pPrev;
}
