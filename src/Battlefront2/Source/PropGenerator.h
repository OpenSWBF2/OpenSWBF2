#pragma once

#include <stddef.h>

#include "PropCluster.h"
#include "PropModel.h"
#include "Battlefront2Lod.h"
#include "PropLayer.h"
#include "Thread.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "RedEngineFL/Util/ext/basevector.h"

class PropModel;
class PblFileChunk;

class PropClusterInfo
{
public:
    void Setup(int x, int z);

    unsigned char mLayerInfo;
    unsigned char mOwner;
};

namespace PropGenerator
{
    // VTABLE: SWBF2 0x007ade90
    class PropGeneratorThread : public Thread
    {
    public:
        virtual bool Update(float fDeltaTime);
    };

    void Init();
    void Cleanup();
    void Read(PblFileChunk& chunk);
    void SetupClusters();
    void SetRandSeed(unsigned int uiSeed);
    float GetRand();
    void Update();

    void FlushClusterObjects();
    void FlushClusterObjects_();
    void SortClusters();
    void ResetClusterRenderedFlags();
    int GetClusterIndex(float x, float z);
    float CalcDistScore_(const PropCluster* pCluster, const PblVector3& vPos, bool bAllCameras);

    float GetLayerMaxDist(int iLayer);
    void AddObjectsFromClusterIndex(int iClusterIndex, int iLayer, PblList<PropObject>& objects);

    void GetTerrainY(float x, float z, float& y);

    const RedLodData* GetLodData(int iIndex);

    const PropObject* FindPropObject(const PblVector3& vPos, bool bParam);

    extern PropClusterInfo* mClusterMap;
    extern float mClusterCellWidth;
    extern PropCluster** mClusterObjectArray;
    extern int mNumClusterObjects;
    extern int mClusterMapWidth;
    extern bool mInitFlag;
    extern bool mLoaded;
    extern PropLayer mPropLayers[4];
    extern PblMatrix m_amCamera[1];
    extern PropGeneratorThread mThread;

    extern unsigned int mSeed;
    extern float mLayerCellWidthAspect;
    extern int mLayerMapWidth;
    extern float mLayerCellWidth;
    extern float mClusterCellWidthAspect;
}
