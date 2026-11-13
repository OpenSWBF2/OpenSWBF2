#include <windows.h>
#include "GameMovie.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "LoadUtil.h"
#include "DataFileName.h"
#include "SkyManager.h"

#include "BoundaryManager.h"
#include "CollisionBody.h"
#include "Entity.h"
#include "FoleyFX.h"
#include "GamePath.h"
#include "HintManager.h"
#include "LoadDisplay.h"
#include "LuaScript.h"
#include "OdfValue.h"
#include "PathFinder.h"
#include "ProceduralAnimation.h"
#include "StringDB.h"
#include "TriggerSoundRegion.h"
#include "Weapon.h"
#include "ConnectivityGraph.h"
#include "Explosion.h"
#include "FLEffect.h"
#include "Ordnance.h"
#include "SoldierAnimatorClass.h"
#include "WeaponMelee.h"
#include "Cloth.h"
#include "CollisionMesh.h"
#include "Music.h"
#include "RedEngineFL/Util/RedAnimation.h"
#include "PebbleFL/Common/PblChunk.h"
#include "PebbleFL/Common/PblConfig.h"
#include "PebbleFL/Common/PblFile.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblUnicode.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Graphics/PC/pcRedTexture.h"
#include "RedEngineFL/Graphics/RedLight.h"
#include "RedEngineFL/Graphics/RedModel.h"
#include "RedEngineFL/Graphics/RedSkeleton.h"
#include "RedEngineFL/Interface/RedFont.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/movie/RedMovie.h"
#include "RedEngineFL/Portals/Portal.h"
#include "RedEngineFL/Util/RedLocalizeDB.h"
#include "RedEngineFL/Util/RedRegion.h"
#include "RedEngineFL/Util/RedSceneManager.h"
#include "RedEngineFL/Util/RedStack.h"
#include "SoundFL/source/sndenginebase.h"
#include "SoundFL/source/sndsamplebankbase.h"

#include "RedEngineFL/Application/RedTerrain.h"

#include "PortalReader.h"

void PumpWaitingMessages();
extern HWND hGlobalWnd;

namespace DownloadableContent
{
bool IsDownloadableContent();
char const* GetContentDirectory();
}

namespace ConnectivityGraphManager
{
void ReadWorldGraph(PblFileChunk& chunk);
}

namespace LoadUtil
{
void ProcessHint(PblFileChunk& chunk);
void ProcessBarrier(PblFileChunk& chunk);
}

#include "GameLoop.h"






// GLOBAL: SWBF2 0x01faaa78

int sUnknownChunkSeen;

class PblMatrix;
extern PblMatrix gMatrixIdentity;

namespace HUD
{
    namespace Manager
    {
        void Load(PblFileChunk& chunk);
    }
}
namespace PropGenerator
{
    void Read(PblFileChunk& chunk);
}

extern bool sLoadNestedLevels;

// GLOBAL: SWBF2 0x01e56fa3
bool gLoading;




// FUNCTION: SWBF2 0x00578ff0
void FileBlockCallback()
{
    PumpWaitingMessages();
    if (LoadDisplay::sInstance && GetForegroundWindow() == hGlobalWnd)
    {
        LoadDisplay::sInstance->Update();
    }
}

// FUNCTION: SWBF2 0x00579020
void LoadUtil::ProcessBin(PblFileChunk& chunk)
{
    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        if (child._iId == CHUNK_ID('N', 'A', 'M', 'E'))
        {
            char acName[256];
            child.ReadString(acName, sizeof(acName));
            PblHash hName(acName);
            (void)hName;
        }
    }
}

// FUNCTION: SWBF2 0x00579090
char const* LoadUtil::GetDataDirectory()
{
    return "data\\_lvl_pc\\";
}

// FUNCTION: SWBF2 0x005790a0
void LoadUtil::MakeFullName(char const* pName, FileType eType, char* pBuffer, int iBufferCount,
                            bool bAddPrefix)
{
    char acRoot[300];
    strlcpy(acRoot, "data\\", sizeof(acRoot) - 1);
    if (bAddPrefix)
    {
        _snprintf(acRoot, sizeof(acRoot), "%s\\Data\\", DownloadableContent::GetContentDirectory());
    }

    int iLength;
    switch (eType)
    {
    case FILE_TYPE_LEVEL:
        iLength = _snprintf(pBuffer, iBufferCount, "%s%s%s.%s", acRoot, "_lvl_pc\\", pName, "lvl");
        break;
    case FILE_TYPE_MOVIE:
        iLength = _snprintf(pBuffer, iBufferCount, "%s%s%s.%s", acRoot, "_lvl_pc\\movies\\", pName, "bik");
        break;
    case FILE_TYPE_NONE:
        iLength = _snprintf(pBuffer, iBufferCount, "%s%s%s", acRoot, "_lvl_pc\\", pName);
        break;
    default:
        return;
    }
    if (iLength < 0)
    {
        pBuffer[iBufferCount - 1] = 0;
    }
}

// FUNCTION: SWBF2 0x00579160
void LoadUtil::LoadSoundBank(PblFileChunk& chunk, char const* pName, void* pData)
{
    (void)pData;

    Snd::SampleBankBase* const pBank = Snd::SampleBankBase::LoadBank(chunk, pName, false, 0);
    if (pBank && pBank->mTemplate == 1)
    {
        PblFile file;
        const bool bOpened = file.Open("data\\_lvl_pc\\sound\\common.bnk", true, false, true);
        PblFileChunk commonChunk(&file);
        if (bOpened)
        {
            Snd::SampleBankBase::LoadBank(commonChunk, pName, false, pBank->mHashID);
            file.Close();
        }
    }
}

// FUNCTION: SWBF2 0x00579210
bool LoadUtil::ReadDataFileChunk(char const* pName, PblFileChunk& chunk,
                                 unsigned int const* puiIds, unsigned int uiIdCount,
                                 unsigned int const* puiHashes, unsigned int uiCount,
                                 char const** ppNames)
{
    unsigned int uiLvlsRead = 0;
    bool abFound[256];

    bool bAllLvls = false;
    if (uiCount == 1 && puiHashes[0] == 0x2F0C9F3D)
    {
        bAllLvls = true;
    }

    while (!chunk.IsDone())
    {
        FileBlockCallback();
        PblFileChunk child = chunk.ReadNextChild();

        Snd::EngineBase::CallbackOverride aSoundOverrides[2] = {
            { 0x5E262256, LoadSoundBank, 0 },
            { 0x9A644D15 /* "SampleBank" */, LoadSoundBank, 0 },
        };

        if (puiIds)
        {
            bool bFound = false;
            unsigned int i;
            for (i = 0; i < uiIdCount; ++i)
            {
                if (puiIds[i] == child._iId)
                {
                    bFound = true;
                    break;
                }
            }
            if (!bFound)
            {
                continue;
            }
            if (i < 256)
            {
                abFound[i] = true;
            }
        }

        switch (child._iId)
        {
        case CHUNK_ID('p', 'o', 'r', 't'):
        {
            RedSceneManager::Read(child);
            pvs::IManager& manager = pvs::IManager::GetInstance();
            PblConfig config(child);
            pvs::PortalReader reader(manager);
            reader.Read(manager, config);
            break;
        }
        case CHUNK_ID('f', 'o', 'n', 't'):
            RedFont::Read(child);
            break;
        case CHUNK_ID('l', 'g', 'h', 't'):
            RedLight::Read(child);
            break;
        case CHUNK_ID('p', 'l', 'n', 'p'):
            ConnectivityGraphManager::ReadWorldPatch(child);
            break;
        case CHUNK_ID('t', 'e', 'r', 'n'):
            RedTerrain::Read(child);
            break;
        case CHUNK_ID('p', 'l', 'a', 'n'):
            ConnectivityGraphManager::ReadWorldGraph(child);
            break;
        case CHUNK_ID('s', 'a', 'n', 'm'):
            SoldierAnimatorClass::Read(child);
            break;
        case CHUNK_ID('p', 'r', 'i', 'm'):
            CollisionBodyPrimitive::Read(child);
            break;
        case CHUNK_ID('c', 'o', 'l', 'l'):
            CollisionMesh::Read(child);
            break;
        case CHUNK_ID('s', 'k', 'e', 'l'):
            RedSkeleton::Read(child);
            break;
        case CHUNK_ID('m', 'o', 'd', 'l'):
            RedModel::Read(child);
            break;
        case CHUNK_ID('L', 'o', 'c', 'l'):
            RedLocalizeDB::Read(child);
            break;
        case CHUNK_ID('p', 'a', 't', 'h'):
            GamePathFactory::Read(child);
            break;
        case CHUNK_ID('m', 'c', 'f', 'g'):
            if (GameMovie::sUseMovies)
            {
                RedMovie::Load(child, false);
            }
            break;
        case CHUNK_ID('g', 'm', 'o', 'd'):
            GameModel::Read(child);
            break;
        case CHUNK_ID('w', 'r', 'l', 'd'):
            ProcessWorld(child);
            break;
        case CHUNK_ID('e', 'n', 't', 'c'):
            EntityClass::Read(child);
            break;
        case CHUNK_ID('w', 'p', 'n', 'c'):
            WeaponClass::Read(child);
            break;
        case CHUNK_ID('e', 'x', 'p', 'c'):
            ExplosionClass::Read(child);
            break;
        case CHUNK_ID('o', 'r', 'd', 'c'):
            OrdnanceClass::Read(child);
            break;
        case CHUNK_ID('c', 'o', 'm', 'b'):
            WeaponMeleeClass::ReadCombo(child);
            break;
        case CHUNK_ID('s', 'k', 'y', '_'):
            SkyManager::Read(child);
            break;
        case CHUNK_ID('f', 'f', 'x', '_'):
            FoleyFX::Load(child);
            break;
        case CHUNK_ID('t', 'e', 'x', '_'):
            RedTexture::Read(child);
            break;
        case CHUNK_ID('m', 'u', 's', '_'):
            Music::Load(child);
            break;
        case CHUNK_ID('t', 's', 'r', '_'):
            TriggerSoundRegion::Properties::Load(child);
            break;
        case CHUNK_ID('s', 'c', 'r', '_'):
            LuaScriptFactory::Read(child);
            break;
        case CHUNK_ID('p', 'r', 'p', '_'):
            PropGenerator::Read(child);
            break;
        case CHUNK_ID('l', 'v', 'l', '_'):
            while (!child.IsDone())
            {
                PblFileChunk lvl = child.ReadNextChild();
                bool bRead = bAllLvls;
                if (uiCount && !bAllLvls)
                {
                    for (unsigned int i = 0; i < uiCount; ++i)
                    {
                        bRead = puiHashes[i] == lvl._iId;
                        if (bRead == true)
                        {
                            if (i < 256)
                            {
                                abFound[i] = true;
                            }
                            break;
                        }
                    }
                }
                if (bRead)
                {
                    ++uiLvlsRead;
                    sLoadNestedLevels = false;
                    const bool bOk = ReadDataFileChunk(pName, lvl, puiIds, uiIdCount, 0, 0, ppNames);
                    sLoadNestedLevels = true;
                    if (!bOk)
                    {
                        return false;
                    }
                }
            }
            break;
        case CHUNK_ID('z', 'a', 'f', '_'):
        {
            const unsigned int uiAllocSize = child._iSize + 8;
            void* pData = RedAllocFromHeap(__RedTempHeap, uiAllocSize, 0);
            child.GetChunkData(pData);
            PblMemChunk memChunk(pData);
            RedAnimation::ReadZaf(memChunk);
            RedFreeToHeap(__RedTempHeap, pData, 0);
            break;
        }
        case CHUNK_ID('h', 'u', 'd', '_'):
            HUD::Manager::Load(child);
            break;
        case CHUNK_ID('s', 'n', 'd', '_'):
            Snd::EngineBase::LoadConfiguration(child, 0, true, 0, 0);
            break;
        case CHUNK_ID('b', 'n', 'd', '_'):
            BoundaryManager::Instance()->Read(child);
            break;
        case CHUNK_ID('z', 'a', 'a', '_'):
        {
            const unsigned int uiAllocSize = child._iSize + 8;
            void* pData = RedAllocFromHeap(__RedTempHeap, uiAllocSize, 0);
            child.GetChunkData(pData);
            PblMemChunk memChunk(pData);
            RedAnimation::ReadZaa(memChunk);
            RedFreeToHeap(__RedTempHeap, pData, 0);
            break;
        }
        case CHUNK_ID('f', 'x', '_', '_'):
            FLEffect::Read(child);
            break;
        case CHUNK_ID('B', 'I', 'N', '_'):
            ProcessBin(child);
            break;
        case 0x5E262256:
            Snd::EngineBase::LoadConfiguration(child, pName, true, aSoundOverrides, 2);
            break;
        case CHUNK_ID('T', 'N', 'L', 'F'):
        case CHUNK_ID('S', 'H', 'D', 'R'):
            RedRenderer::Read(child);
            break;
        case CHUNK_ID('C', 'L', 'T', 'H'):
            ClothData::Read(child);
            break;
        }
    }

    if (uiLvlsRead >= uiCount)
    {
        return true;
    }
    return false;
}

// FUNCTION: SWBF2 0x00579930
bool LoadUtil::ReadDataFileOnHeap(char const* pName, int iHeap,
                                  unsigned int const* puiIds, unsigned int uiIdCount,
                                  unsigned int const* puiHashes, unsigned int uiCount,
                                  char const** ppNames)
{
    PblFile file;
    RedStack stack;
    char* pcFullName = (char*)stack.Alloc(0x100);
    char* pcFileName = (char*)stack.Alloc(0x100);
    char* pcLvlName = (char*)stack.Alloc(0x100);
    pcLvlName[0] = '\0';
    unsigned int uiLvlHash = 0;
    gLoading = true;

    if (!SplitDataFileName(pName, pcFileName, pcLvlName, 0x100))
    {
        gLoading = false;
        return false;
    }

    if (!_stricmp(pcFileName, "mission.lvl") && DownloadableContent::IsDownloadableContent())
    {
        strlcpy(pcFullName, DownloadableContent::GetContentDirectory(), 0x100);
        strlcat(pcFullName, "\\Data\\_lvl_pc\\", 0x100);
        strlcat(pcFullName, pcFileName, 0x100);
    }
    else if (!_strnicmp(pcFileName, "dc:", 3))
    {
        strlcpy(pcFullName, DownloadableContent::GetContentDirectory(), 0x100);
        strlcat(pcFullName, "\\Data\\_lvl_pc\\", 0x100);
        strlcat(pcFullName, pcFileName + 3, 0x100);
    }
    else if (!_strnicmp(pcFileName, "raw:", 4))
    {
        strlcpy(pcFullName, pcFileName, 0x100);
    }
    else
    {
        strlcpy(pcFullName, "data\\_lvl_pc\\", 0x100);
        strlcat(pcFullName, pcFileName, 0x100);
    }

    int iOldHeap = 0;
    if (iHeap != -1)
    {
        iOldHeap = RedSetCurrentHeap(iHeap);
    }
    PblFile::SetBlockCallback(FileBlockCallback);
    int bOpened = file.Open(pcFullName, true, false, true);
    if (iHeap != -1)
    {
        RedSetCurrentHeap(iOldHeap);
    }

    if (!bOpened)
    {
        gLoading = false;
        PblFile::SetBlockCallback(0);
        return false;
    }

    if ((!uiCount || !puiHashes) && pcLvlName[0] != '\0')
    {
        uiLvlHash = PblHash(pcLvlName);
    }

    PblFileChunk chunk(&file);
    if (uiLvlHash)
    {
        ReadDataFileChunk(pcFullName, chunk, puiIds, uiIdCount, &uiLvlHash, 1, ppNames);
    }
    else
    {
        ReadDataFileChunk(pcFullName, chunk, puiIds, uiIdCount, puiHashes, uiCount, ppNames);
    }

    if (iHeap != -1)
    {
        iOldHeap = RedSetCurrentHeap(iHeap);
    }
    file.Close();
    if (iHeap != -1)
    {
        RedSetCurrentHeap(iOldHeap);
    }

    PblFile::SetBlockCallback(0);
    gLoading = false;
    return true;
}

// FUNCTION: SWBF2 0x00579c30
bool LoadUtil::ReadDataFile(char const* pName, unsigned int const* puiIds, unsigned int uiIdCount,
                            unsigned int const* puiHashes,
                            unsigned int uiCount, char const** ppNames)
{
    return ReadDataFileOnHeap(pName, __RedTempHeap, puiIds, uiIdCount, puiHashes, uiCount, ppNames);
}

// FUNCTION: SWBF2 0x0057aa60
bool LoadUtil::ReadDataFile(PblFile& file, unsigned int const* puiIds, unsigned int uiIdCount,
                            unsigned int const* puiHashes,
                            unsigned int uiCount, char const** ppNames)
{
    PblFile::SetBlockCallback(FileBlockCallback);
    PblFileChunk chunk(&file);
    ReadDataFileChunk("Memory File", chunk, puiIds, uiIdCount, puiHashes, uiCount, ppNames);
    PblFile::SetBlockCallback(0);
    return true;
}

// FUNCTION: SWBF2 0x00579c50
void LoadUtil::ProcessWorld(PblFileChunk& chunk)
{
    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        char acName[256];
        switch (child._iId)
        {
        case CHUNK_ID('N', 'A', 'M', 'E'):
            child.ReadString(acName, 256);
            GameLoop::SetWorldFilename(acName);
            break;
        case CHUNK_ID('T', 'N', 'A', 'M'):
            child.ReadString(acName, 256);
            GameLoop::SetTerrainFilename(acName);
            break;
        case CHUNK_ID('B', 'A', 'R', 'R'):
            ProcessBarrier(child);
            break;
        case CHUNK_ID('a', 'n', 'm', 'g'):
            PAAnimationGroup::Read(child);
            break;
        case CHUNK_ID('a', 'n', 'm', 'h'):
            PAAnimationGroup::ReadHierarchy(child);
            break;
        case CHUNK_ID('a', 'n', 'i', 'm'):
            PAAnimation::Read(child);
            break;
        case CHUNK_ID('r', 'e', 'g', 'n'):
            ProcessRegion(child);
            break;
        case CHUNK_ID('H', 'i', 'n', 't'):
            ProcessHint(child);
            break;
        case CHUNK_ID('i', 'n', 's', 't'):
            ProcessEntity(child);
            break;
        default:
            sUnknownChunkSeen = 1;
            break;
        }
    }
}

// FUNCTION: SWBF2 0x00579d80
Entity* LoadUtil::ProcessEntityInfo(PblFileChunk& chunk)
{
    EntityDesc desc;
    EntityClass* pClass = 0;
    char acName[128];

    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        switch (child._iId)
        {
        case CHUNK_ID('N', 'A', 'M', 'E'):
            desc.mNameId = PblHash(child.ReadString(acName, sizeof(acName)));
            StringDB::Store(desc.mNameId, acName);
            break;
        case CHUNK_ID('T', 'Y', 'P', 'E'):
            pClass = Factory<Entity, EntityClass, EntityDesc>::Find(
                PblHash(child.ReadString(acName, sizeof(acName))));
            break;
        case CHUNK_ID('X', 'F', 'R', 'M'):
        {
            float x = child._pFile->ReadFloat();
            float y = child._pFile->ReadFloat();
            float z = child._pFile->ReadFloat();
            desc.mMatrix.SetXAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            desc.mMatrix.SetYAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            desc.mMatrix.SetZAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            desc.mMatrix.SetTranslation(x, y, z, 1.0f);
            break;
        }
        }
    }

    if (!pClass)
    {
        return 0;
    }
    return pClass->Build(desc);
}

// FUNCTION: SWBF2 0x0057a000
void LoadUtil::ProcessEntity(PblFileChunk& chunk)
{
    Entity* pEntity = 0;
    char acValue[128];

    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        switch (child._iId)
        {
        case CHUNK_ID('I', 'N', 'F', 'O'):
            pEntity = ProcessEntityInfo(child);
            if (!pEntity)
            {
                return;
            }
            break;
        case CHUNK_ID('P', 'R', 'O', 'P'):
        {
            const unsigned int uProperty = child._pFile->Read32();
            char* pcValue = child.ReadString(acValue, sizeof(acValue));
            OdfValue::Sanitize(uProperty, pcValue);
            pEntity->SetProperty(uProperty, pcValue);
            break;
        }
        }
    }
    pEntity->Init();
}

// FUNCTION: SWBF2 0x0057a0a0
RedRegion* LoadUtil::ProcessRegionInfo(PblFileChunk& chunk)
{
    RedRegionDesc desc;
    desc.mType = PblRegion::type_invalid;
    char acName[128];

    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        switch (child._iId)
        {
        case CHUNK_ID('T', 'Y', 'P', 'E'):
            desc.mTypeId = PblHash(child.ReadString(acName, sizeof(acName)));
            break;
        case CHUNK_ID('N', 'A', 'M', 'E'):
            desc.mNameId = PblHash(child.ReadString(acName, sizeof(acName)));
            StringDB::Store(desc.mNameId, acName);
            break;
        case CHUNK_ID('S', 'I', 'Z', 'E'):
        {
            desc.mSize.x = child._pFile->ReadFloat();
            desc.mSize.y = child._pFile->ReadFloat();
            desc.mSize.z = child._pFile->ReadFloat();
            desc.mSize.w = D3DXVec3Length(reinterpret_cast<const D3DXVECTOR3*>(&desc.mSize));
            break;
        }
        case CHUNK_ID('X', 'F', 'R', 'M'):
        {
            float x = child._pFile->ReadFloat();
            float y = child._pFile->ReadFloat();
            float z = child._pFile->ReadFloat();
            desc.mMatrix.SetXAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            desc.mMatrix.SetYAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            desc.mMatrix.SetZAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            desc.mMatrix.SetTranslation(x, y, z, 1.0f);
            break;
        }
        }
    }

    const RedRegionFactory* pFactory = RedRegionFactory::Find(acName);
    void* pRegion = pFactory->CreateRegion(desc, acName);
    if (pFactory == RedRegionFactory::GetDefault())
    {
        return (RedRegion*)(pRegion != 0);
    }
    return (RedRegion*)pRegion;
}

// FUNCTION: SWBF2 0x0057a360
void LoadUtil::ProcessRegion(PblFileChunk& chunk)
{
    RedRegion* pRegion = 0;
    char acValue[128];

    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        switch (child._iId)
        {
        case CHUNK_ID('I', 'N', 'F', 'O'):
            pRegion = ProcessRegionInfo(child);
            break;
        case CHUNK_ID('P', 'R', 'O', 'P'):
            if ((unsigned int)pRegion > 1)
            {
                const unsigned int uProperty = child._pFile->Read32();
                char* pcValue = child.ReadString(acValue, sizeof(acValue));
                OdfValue::Sanitize(uProperty, pcValue);
                pRegion->SetProperty(uProperty, pcValue);
            }
            break;
        }
    }
}

bool __cdecl IsCoverHintFilteringEnabled();

inline void* BaseHint::operator new(size_t uSize)
{
    return sMemoryPool.Allocate(uSize);
}

namespace LoadUtil
{

// FUNCTION: SWBF2 0x0057a3e0
BaseHint* ProcessHintInfo(PblFileChunk& chunk)
{
    HintDesc desc;
    unsigned int uType = 0;
    char acName[128];

    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        switch (child._iId)
        {
        case CHUNK_ID('N', 'A', 'M', 'E'):
            desc.mNameId = PblHash(child.ReadString(acName, sizeof(acName)));
            break;
        case CHUNK_ID('T', 'Y', 'P', 'E'):
            uType = PblHash(child.ReadString(acName, sizeof(acName)));
            break;
        case CHUNK_ID('X', 'F', 'R', 'M'):
        {
            float x = child._pFile->ReadFloat();
            float y = child._pFile->ReadFloat();
            float z = child._pFile->ReadFloat();
            desc.mMatrix.SetXAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            desc.mMatrix.SetYAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            desc.mMatrix.SetZAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            desc.mMatrix.SetTranslation(x, y, z, 1.0f);
            break;
        }
        }
    }

    BaseHint* pHint = 0;
    switch (uType)
    {
    case 0x350CA8AF: /* "0" */
        pHint = new SnipeHint(desc);
        break;
    case 0x340CA71C: /* "1" */
        pHint = new PatrolHint(desc);
        break;
    case 0x370CABD5: /* "2" */
        pHint = new CoverHint(desc);
        break;
    case 0x310CA263:
        pHint = new JetJumpHint(desc);
        break;
    case 0x300CA0D0:
        pHint = new MinefieldHint(desc);
        break;
    case 0x330CA589: /* "6" */
        pHint = new LandHint(desc);
        break;
    case 0x320CA3F6: /* "7" */
        pHint = new FortificationHint(desc);
        break;
    case 0x3D0CB547: /* "8" */
        pHint = new VehicleCoverHint(desc);
        break;
    }
    return pHint;
}
}

// FUNCTION: SWBF2 0x0057a6e0
void LoadUtil::ProcessHint(PblFileChunk& chunk)
{
    BaseHint* pHint = 0;
    char acValue[128];

    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        switch (child._iId)
        {
        case CHUNK_ID('I', 'N', 'F', 'O'):
            pHint = LoadUtil::ProcessHintInfo(child);
            if (!pHint)
            {
                return;
            }
            break;
        case CHUNK_ID('P', 'R', 'O', 'P'):
        {
            const unsigned int uProperty = child._pFile->Read32();
            char* pcValue = child.ReadString(acValue, sizeof(acValue));
            OdfValue::Sanitize(uProperty, pcValue);
            pHint->SetProperty(uProperty, pcValue);
            break;
        }
        }
    }

    if (pHint)
    {
        if (!IsCoverHintFilteringEnabled() || pHint->GetType() != BaseHint::HINT_COVER)
        {
            HintManager::AddHint(pHint);
        }
    }
}

namespace LoadUtil
{

// FUNCTION: SWBF2 0x0057a790
void ProcessBarrierInfo(PblFileChunk& chunk)
{
    PblMatrix mMatrix = gMatrixIdentity;
    PblVector4 vSize(0.0f);
    unsigned int iFlag = 0;
    char acName[128];

    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        switch (child._iId)
        {
        case CHUNK_ID('F', 'L', 'A', 'G'):
            iFlag = child._pFile->Read32();
            break;
        case CHUNK_ID('N', 'A', 'M', 'E'):
            child.ReadString(acName, sizeof(acName));
            break;
        case CHUNK_ID('S', 'I', 'Z', 'E'):
        {
            const float fX = child._pFile->ReadFloat();
            const float fY = child._pFile->ReadFloat();
            const float fZ = child._pFile->ReadFloat();
            const float fLength = Length(PblVector3(fX, fY, fZ));
            vSize = PblVector4(fX, fY, fZ, fLength);
            break;
        }
        case CHUNK_ID('X', 'F', 'R', 'M'):
        {
            float x = child._pFile->ReadFloat();
            float y = child._pFile->ReadFloat();
            float z = child._pFile->ReadFloat();
            mMatrix.SetXAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            mMatrix.SetYAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            mMatrix.SetZAxis(x, y, z, 0.0f);
            x = child._pFile->ReadFloat();
            y = child._pFile->ReadFloat();
            z = child._pFile->ReadFloat();
            mMatrix.SetTranslation(x, y, -z, 1.0f);
            break;
        }
        }
    }

    PathFinder::AddObstacle(mMatrix, vSize, (ConnectivityGraph::ProhibitFlag)iFlag, PblHash(acName));
}
}

// FUNCTION: SWBF2 0x0057a9e0
void LoadUtil::ProcessBarrier(PblFileChunk& chunk)
{
    char acValue[128];

    while (!chunk.IsDone())
    {
        PblFileChunk child = chunk.ReadNextChild();
        switch (child._iId)
        {
        case CHUNK_ID('I', 'N', 'F', 'O'):
            LoadUtil::ProcessBarrierInfo(child);
            break;
        case CHUNK_ID('P', 'R', 'O', 'P'):
            child._pFile->Read32();
            child.ReadString(acValue, sizeof(acValue));
            break;
        }
    }
}

// TEMPLATE: SWBF2 0x00578f10
// util::HashTable_<pvs::ISector *,char const *,util::Vector<pvs::ISector *,util::BaseArray<pvs::ISector *,util::TypedArrayAllocator<pvs::ISector *,util::TNewArrayAlloc<pvs::ISector *> > > >,util::Vector<char const *,util::BaseArray<char const *,util::TypedArrayAllocator<char const *,util::TNewArrayAlloc<pvs::ISector *> > > >,util::Vector<unsigned int,util::BaseArray<unsigned int,util::TypedArrayAllocator<unsigned int,util::TNewArrayAlloc<pvs::ISector *> > > > >::~HashTable_<pvs::ISector *,char const *,util::Vector<pvs::ISector *,util::BaseArray<pvs::ISector *,util::TypedArrayAllocator<pvs::ISector *,util::TNewArrayAlloc<pvs::ISector *> > > >,util::Vector<char const *,util::BaseArray<char const *,util::TypedArrayAllocator<char const *,util::TNewArrayAlloc<pvs::ISector *> > > >,util::Vector<unsigned int,util::BaseArray<unsigned int,util::TypedArrayAllocator<unsigned int,util::TNewArrayAlloc<pvs::ISector *> > > > >
template util::HashTable_<pvs::ISector*, char const*, util::HashTable<pvs::ISector*, char const*, util::TNewArrayAlloc<pvs::ISector*> >::ElemVecT, util::HashTable<pvs::ISector*, char const*, util::TNewArrayAlloc<pvs::ISector*> >::KeyVecT, util::HashTable<pvs::ISector*, char const*, util::TNewArrayAlloc<pvs::ISector*> >::HashVecT>::~HashTable_();
