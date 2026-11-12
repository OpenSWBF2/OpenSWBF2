#include <algorithm>
#include <vector>

#include "Battlefront2/Source/CollisionManager.h"
#include "Battlefront2/Source/GameMemory.h"
#include "Battlefront2/Source/TreeGrid.h"
#include "PebbleFL/Common/PblChunk.h"
#include "PebbleFL/Common/PblFile.h"
#include "RedEngineFL/Application/RedTerrain.h"
#include "RedEngineFL/Application/RedWater.h"

int gTerrainStubSphereCalls;

namespace GameMemory
{
int RunTimeHeap;
}

alignas(TreeGrid) static unsigned char sTerrainStubGrid[sizeof(TreeGrid)];
alignas(CollisionManager) static unsigned char sTerrainStubManager[sizeof(CollisionManager)];

CollisionManager* CollisionManager::sInstance = reinterpret_cast<CollisionManager*>(sTerrainStubManager);

static const bool sTerrainStubGridSet =
    (CollisionManager::sInstance->mTreeGrid = reinterpret_cast<TreeGrid*>(sTerrainStubGrid)) != 0;

TreeGrid* CollisionManager::GetTreeGrid()
{
    return sInstance ? sInstance->mTreeGrid : 0;
}

void TreeGrid::CallbackOnSphere(const PblVector3& rCenter, float fRadius, TreeGridCallback pCallback,
                                TreeGridCallbackData* pData)
{
    ++gTerrainStubSphereCalls;
}

std::vector<const TreeGridObject*> gTreeGridStubMembers;

TreeGrid::TreeGrid(const PblVector3&, const PblVector3&, float)
{
}

void TreeGrid::Add(TreeGridObject* pObj, const PblVector3& rPos, float fSize)
{
    gTreeGridStubMembers.push_back(pObj);
}

void TreeGrid::Remove(TreeGridObject* pObj)
{
    std::vector<const TreeGridObject*>::iterator it = std::find(gTreeGridStubMembers.begin(), gTreeGridStubMembers.end(), pObj);
    if (it != gTreeGridStubMembers.end())
    {
        gTreeGridStubMembers.erase(it);
    }
}

void TreeGrid::Update(TreeGridObject*, const PblVector3&, float)
{
}

void TreeGrid::CallbackOnRect(const PblVector3&, const PblVector3&, TreeGridCallback, TreeGridCallbackData*)
{
}

void TreeGrid::CallbackOnFatRay(const PblVector3&, const PblVector3&, float, TreeGridCallback, TreeGridCallbackData*)
{
}

void RedTerrain::_ReadPatchInfo(PblFileChunk& chunk)
{
}

void RedWater::Read(PblFileChunk& chunk)
{
}

PblFileChunk PblFileChunk::ReadNextChild()
{
    return *this;
}

char* PblFileChunk::ReadString(char* pcBuffer, int iBufSize)
{
    return pcBuffer;
}

char PblFile::Read8()
{
    return 0;
}

short PblFile::Read16()
{
    return 0;
}

int PblFile::Read32()
{
    return 0;
}

float PblFile::ReadFloat()
{
    return 0.0f;
}

void PblFile::ReadData(void* pBuffer, int iNumBytes)
{
}
