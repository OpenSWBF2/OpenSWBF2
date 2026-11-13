#pragma once

#include "Battlefront2Lod.h"

class PropModel;
class PropObject;
class PblFileChunk;

class PropLayerLodData : public RedLodData
{
public:
    PropLayerLodData(float fMaxDist)
    {
        _LodClass = -1;
        _LodBitmask = 7;
        _LodCost[0] = 0;
        _LodCost[1] = 0;
        _LodCost[2] = 0;
        _LodCost[3] = 0;
        mLowRezFlags = 0;
        mMaxDist = fMaxDist;
    }
};
 

class PropLayer
{
public:

    PropLayer();
    ~PropLayer();
    void Clear();
    void Read(PblFileChunk& chunk);
    void CreateCollisionObjects(int x, int z, int iOffset, float fParam);
    PropObject* CreateObject(int x, int z, int iOffset, float fParam);

    PropModel* mModels[32];
    int mNumModels;
    int mSpreadFactor;
    int mObjectsPerCell;
    float mMaxDistance;
    PropLayerLodData* mLodData;
};
 
 

