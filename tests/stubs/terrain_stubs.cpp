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

TreeGrid* CollisionManager::GetTreeGrid()
{
    return reinterpret_cast<TreeGrid*>(sTerrainStubGrid);
}

void TreeGrid::CallbackOnSphere(const PblVector3& rCenter, float fRadius, TreeGridCallback pCallback,
                                TreeGridCallbackData* pData)
{
    ++gTerrainStubSphereCalls;
}

void TreeGrid::Add(TreeGridObject* pObj, const PblVector3& rPos, float fSize)
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
