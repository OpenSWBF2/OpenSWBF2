#include "third_party/doctest/doctest.h"

#include <windows.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>

#include "Battlefront2/Platform/Pc/pcSaveDevice.h"
#include "Battlefront2/Source/LuaSaveItems.h"
#include "Battlefront2/Source/PlayerProfile.h"
#include "Battlefront2/Source/ProfileCRC.h"
#include "Battlefront2/Source/SaveFsm.h"
#include "LuaFL/include/lua.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "ZlibFL/zlib.h"
#include "testutil.h"
#include "unit/lua_save_pins.h"

namespace
{

const unsigned int kProfileFileCRC = 0x59349657u;
const unsigned int kProfileCRC = 0x68186e0bu;
const unsigned int kMetagameFileCRC = 0x9e0d7d9au;
const unsigned int kCampaignFileCRC = 0xc36ba53eu;

const unsigned char kPinnedCompressed[] =
{
    0x78, 0xda, 0x63, 0x65, 0x64, 0x64, 0x66, 0x60, 0x50, 0x70, 0x60, 0x29,
    0xc8, 0x49, 0xcc, 0x4b, 0x2d, 0x61, 0x60, 0x65, 0x29, 0x4a, 0x2d, 0x66,
    0x00, 0x09, 0x39, 0x01, 0x09, 0x07, 0x07, 0x96, 0x92, 0x8c, 0xa2, 0xd4,
    0x54, 0x06, 0x96, 0x8c, 0xc4, 0xbc, 0x94, 0x9c, 0x54, 0x06, 0xa6, 0x8a,
    0x30, 0x13, 0x21, 0x96, 0xbc, 0xd4, 0xe2, 0x92, 0xd4, 0x14, 0x06, 0x56,
    0xa0, 0x8a, 0x06, 0x7b, 0x88, 0x50, 0x4a, 0x6a, 0x6a, 0x01, 0x48, 0xdb,
    0x03, 0x07, 0x06, 0x88, 0x22, 0x00, 0x2f, 0x9d, 0x13, 0x0f
};

const unsigned short kProfileName[] = { 'P', 'i', 'n', 'n', 'e', 'd', 0 };
const unsigned short kMetagameName[] = { 'P', 'i', 'n', 'n', 'e', 'd', 'G', 'C', 0 };
const unsigned short kCampaignName[] = { 'P', 'i', 'n', 'n', 'e', 'd', 'R', 'C', 0 };

class TempSaveDir
{
public:
    TempSaveDir()
    {
        GetCurrentDirectoryA(sizeof(macOld), macOld);
        char acTemp[MAX_PATH];
        GetTempPathA(sizeof(acTemp), acTemp);
        char acDir[MAX_PATH];
        _snprintf(acDir, sizeof(acDir), "%sopenswbf2-saves-%lu", acTemp, GetCurrentProcessId());
        acDir[sizeof(acDir) - 1] = 0;
        msDir = acDir;
        Clear();
        CreateDirectoryA(msDir.c_str(), 0);
        mbOk = SetCurrentDirectoryA(msDir.c_str()) != 0;
    }

    ~TempSaveDir()
    {
        SetCurrentDirectoryA(macOld);
        Clear();
    }

    bool Ok() const
    {
        return mbOk;
    }

    std::string SavePath(const char* pcFile) const
    {
        return msDir + "\\SaveGames\\" + pcFile;
    }

private:
    void Clear()
    {
        const std::string sSaves = msDir + "\\SaveGames";
        WIN32_FIND_DATAA kData;
        HANDLE hFind = FindFirstFileA((sSaves + "\\*").c_str(), &kData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(kData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    DeleteFileA((sSaves + "\\" + kData.cFileName).c_str());
                }
            } while (FindNextFileA(hFind, &kData));
            FindClose(hFind);
        }
        RemoveDirectoryA(sSaves.c_str());
        RemoveDirectoryA(msDir.c_str());
    }

    TempSaveDir(const TempSaveDir&);
    TempSaveDir& operator=(const TempSaveDir&);
    char macOld[MAX_PATH];
    std::string msDir;
    bool mbOk;
};

bool WriteLikeSaveFsm(SaveDevice& kDevice, const unsigned short* pusName, int iType, void* pData,
                      unsigned int uiSize)
{
    if (kDevice.StartRefreshFileList() != 0)
    {
        return false;
    }
    if (!kDevice.FindFileInList(pusName, iType) && kDevice.StartCreateFile(pusName, iType) != 0)
    {
        return false;
    }
    SaveFile* pFile = kDevice.FindFileInList(pusName, iType);
    if (!pFile || kDevice.StartOpenFile(*pFile, false) != 0)
    {
        return false;
    }
    unsigned int uiChecksum = CalcCRC(pData, uiSize, 0);
    const bool bOk = kDevice.StartWriteFile(*pFile, &uiChecksum, SaveFsm::CRC_SIZE) == 0 &&
                     kDevice.StartWriteFile(*pFile, pData, uiSize) == 0;
    return kDevice.StartCloseFile(*pFile, false) == 0 && bOk;
}

int ReadLikeSaveFsm(SaveDevice& kDevice, const unsigned short* pusName, int iType, void* pDest,
                    unsigned int uiSize)
{
    if (kDevice.StartRefreshFileList() != 0)
    {
        return -100;
    }
    SaveFile* pFile = kDevice.FindFileInList(pusName, iType);
    if (!pFile || kDevice.StartOpenFile(*pFile, false) != 0)
    {
        return -101;
    }
    unsigned int uiChecksum = 0;
    int iResult = kDevice.StartReadFile(*pFile, &uiChecksum, SaveFsm::CRC_SIZE);
    if (iResult == 0)
    {
        iResult = kDevice.StartReadFile(*pFile, pDest, uiSize);
    }
    kDevice.StartCloseFile(*pFile, false);
    return iResult;
}

std::vector<unsigned char> ReadFileBytes(const std::string& sPath)
{
    std::vector<unsigned char> vBytes;
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return vBytes;
    }
    unsigned char acBlock[4096];
    size_t uiRead = 0;
    while ((uiRead = fread(acBlock, 1, sizeof(acBlock), pFile)) > 0)
    {
        vBytes.insert(vBytes.end(), acBlock, acBlock + uiRead);
    }
    fclose(pFile);
    return vBytes;
}

bool WriteFileBytes(const std::string& sPath, const unsigned char* pData, size_t uiSize)
{
    FILE* pFile = fopen(sPath.c_str(), "wb");
    if (!pFile)
    {
        return false;
    }
    const bool bOk = fwrite(pData, 1, uiSize, pFile) == uiSize;
    return fclose(pFile) == 0 && bOk;
}

void Put32(unsigned char* pDest, unsigned int uiValue)
{
    pDest[0] = (unsigned char)uiValue;
    pDest[1] = (unsigned char)(uiValue >> 8);
    pDest[2] = (unsigned char)(uiValue >> 16);
    pDest[3] = (unsigned char)(uiValue >> 24);
}

void FillPattern(void* pData, size_t uiSize, unsigned int uiSeed)
{
    unsigned char* pBytes = static_cast<unsigned char*>(pData);
    for (size_t i = 0; i < uiSize; ++i)
    {
        pBytes[i] = (unsigned char)(uiSeed + i * 7);
    }
}

void FillProfile(PlayerProfile::tProfile& kProfile)
{
    memset(&kProfile, 0, sizeof(kProfile));
    kProfile.mInitialized = true;
    kProfile.mAutoSaveOn = true;
    for (unsigned int i = 0; kProfileName[i]; ++i)
    {
        kProfile.mName[i] = kProfileName[i];
    }
    kProfile.mVersion = 1;
    FillPattern(&kProfile.mGameOptions, sizeof(kProfile.mGameOptions), 0x11);
    FillPattern(&kProfile.mSoundOptions, sizeof(kProfile.mSoundOptions), 0x22);
    FillPattern(&kProfile.mOnlineOptions, sizeof(kProfile.mOnlineOptions), 0x33);
    FillPattern(&kProfile.mToolTipOptions, sizeof(kProfile.mToolTipOptions), 0x44);
    FillPattern(&kProfile.mAccumPoints, sizeof(kProfile.mAccumPoints), 0x55);
    FillPattern(&kProfile.mPCOptions, sizeof(kProfile.mPCOptions), 0x66);
    kProfile.mCurMission = 7;
    kProfile.mBonus = 0x01020304u;
    kProfile.mIsDirty = false;
    kProfile.mCRC = CalcCRC(&kProfile, offsetof(PlayerProfile::tProfile, mCRC), 0);
    kProfile.mIsDirty = true;
}

std::vector<unsigned char> ExpectedProfileFile()
{
    std::vector<unsigned char> vFile(4356, 0);
    Put32(&vFile[0], kProfileFileCRC);
    unsigned char* pPayload = &vFile[4];
    pPayload[0] = 1;
    pPayload[1] = 1;
    for (unsigned int i = 0; kProfileName[i]; ++i)
    {
        pPayload[2 + i * 2] = (unsigned char)kProfileName[i];
    }
    Put32(pPayload + 40, 1);
    FillPattern(pPayload + 48, 1472, 0x11);
    FillPattern(pPayload + 1520, 72, 0x22);
    FillPattern(pPayload + 1592, 2316, 0x33);
    FillPattern(pPayload + 3908, 66, 0x44);
    FillPattern(pPayload + 3976, 12, 0x55);
    FillPattern(pPayload + 3988, 252, 0x66);
    pPayload[4336] = 7;
    Put32(pPayload + 4340, 0x01020304u);
    pPayload[4344] = 1;
    Put32(pPayload + 4348, kProfileCRC);
    return vFile;
}

std::vector<unsigned char> ExpectedItemsFile(unsigned int uiFileCRC, size_t uiBufferSize)
{
    std::vector<unsigned char> vFile(4 + uiBufferSize, 0);
    Put32(&vFile[0], uiFileCRC);
    memcpy(&vFile[4], kPinnedCompressed, sizeof(kPinnedCompressed));
    return vFile;
}

size_t FirstDifference(const std::vector<unsigned char>& vA, const std::vector<unsigned char>& vB)
{
    size_t i = 0;
    while (i < vA.size() && i < vB.size() && vA[i] == vB[i])
    {
        ++i;
    }
    return i;
}

unsigned int CompressLikeGame(unsigned char* pDest, int iCapacity, unsigned char* pSource, int iSize)
{
    z_stream kStream;
    kStream.zalloc = Z_NULL;
    kStream.zfree = Z_NULL;
    kStream.opaque = 0;
    if (deflateInit(&kStream, 9) != Z_OK)
    {
        return 0;
    }
    kStream.next_in = pSource;
    kStream.avail_in = iSize;
    kStream.total_in = 0;
    kStream.next_out = pDest;
    kStream.avail_out = iCapacity;
    kStream.total_out = 0;
    deflate(&kStream, Z_FINISH);
    deflateEnd(&kStream);
    return kStream.total_out;
}

unsigned int UncompressLikeGame(unsigned char* pDest, int iCapacity, unsigned char* pSource, int iSize)
{
    z_stream kStream;
    kStream.zalloc = Z_NULL;
    kStream.zfree = Z_NULL;
    kStream.opaque = 0;
    if (inflateInit_(&kStream, ZLIB_VERSION, sizeof(kStream)) != Z_OK)
    {
        return 0;
    }
    kStream.next_in = pSource;
    kStream.avail_in = iSize;
    kStream.total_in = 0;
    kStream.next_out = pDest;
    kStream.avail_out = iCapacity;
    kStream.total_out = 0;
    inflate(&kStream, Z_FINISH);
    inflateEnd(&kStream);
    return kStream.total_out;
}

std::vector<unsigned char> BuildItemsBuffer(size_t uiBufferSize)
{
    std::vector<unsigned char> vBuffer(uiBufferSize, 0);
    lua_State* L = lua_open();
    if (!L)
    {
        return vBuffer;
    }
    PushPinnedItems(L);
    std::vector<unsigned char> vSerialized(0x10000, 0);
    const unsigned int uiWritten = LuaSaveItems::Write(L, &vSerialized[0], (unsigned int)vSerialized.size());
    if (uiWritten)
    {
        CompressLikeGame(&vBuffer[0], (int)vBuffer.size(), &vSerialized[0], (int)uiWritten);
    }
    lua_close(L);
    return vBuffer;
}

int CountPairs(lua_State* L, int iTable)
{
    int iCount = 0;
    lua_pushnil(L);
    while (lua_next(L, iTable))
    {
        ++iCount;
        lua_pop(L, 1);
    }
    return iCount;
}

void CheckItemsLoad(unsigned char* pBuffer, unsigned int uiSize)
{
    std::vector<unsigned char> vSerialized(0x10000, 0);
    const unsigned int uiInflated = UncompressLikeGame(&vSerialized[0], (int)vSerialized.size(), pBuffer, (int)uiSize);
    CHECK(uiInflated == sizeof(kPinnedItems));
    CHECK(memcmp(&vSerialized[0], kPinnedItems, sizeof(kPinnedItems)) == 0);
    lua_State* L = lua_open();
    REQUIRE(L != (lua_State*)0);
    CHECK(LuaSaveItems::Read(L, &vSerialized[0], uiInflated) == 5);
    REQUIRE(lua_gettop(L) == 5);
    CHECK(lua_type(L, 1) == LUA_TBOOLEAN);
    CHECK(lua_tonumber(L, 2) == 2.5);
    CHECK(lua_type(L, 3) == LUA_TSTRING);
    CHECK(strcmp(lua_tostring(L, 3), "planet") == 0);
    REQUIRE(lua_type(L, 4) == LUA_TTABLE);
    CHECK(CountPairs(L, 4) == 3);
    CHECK(lua_type(L, 5) == LUA_TNIL);
    lua_close(L);
}

void CheckItemsRoundTrip(const unsigned short* pusName, int iType, const char* pcFile, size_t uiBufferSize,
                         unsigned int uiFileCRC)
{
    TempSaveDir kDir;
    REQUIRE(kDir.Ok());
    std::unique_ptr<SaveDevice> pDevice(new SaveDevice);
    std::vector<unsigned char> vBuffer = BuildItemsBuffer(uiBufferSize);
    REQUIRE(WriteLikeSaveFsm(*pDevice, pusName, iType, &vBuffer[0], (unsigned int)vBuffer.size()));

    const std::vector<unsigned char> vFile = ReadFileBytes(kDir.SavePath(pcFile));
    const std::vector<unsigned char> vExpected = ExpectedItemsFile(uiFileCRC, uiBufferSize);
    const size_t uiFirstDiff = FirstDifference(vFile, vExpected);
    INFO(uiFirstDiff);
    CHECK(vFile.size() == 4 + uiBufferSize);
    CHECK(vFile == vExpected);

    std::vector<unsigned char> vLoaded(uiBufferSize, 0xCD);
    {
        LogCapture kCapture;
        CHECK(ReadLikeSaveFsm(*pDevice, pusName, iType, &vLoaded[0], (unsigned int)vLoaded.size()) == 0);
        CHECK(kCapture.Count("OPENSWBF2 SAVE") == 0);
    }
    CHECK(vLoaded == vBuffer);
    CheckItemsLoad(&vLoaded[0], (unsigned int)vLoaded.size());
}

}

TEST_CASE("save files match pinned x86 bytes" * doctest::test_suite("regression"))
{
    SUBCASE("profile")
    {
        TempSaveDir kDir;
        REQUIRE(kDir.Ok());
        std::unique_ptr<SaveDevice> pDevice(new SaveDevice);
        std::unique_ptr<PlayerProfile::tProfile> pProfile(new PlayerProfile::tProfile);
        FillProfile(*pProfile);
        REQUIRE(WriteLikeSaveFsm(*pDevice, kProfileName, 1, pProfile.get(), sizeof(PlayerProfile::tProfile)));

        const std::vector<unsigned char> vFile = ReadFileBytes(kDir.SavePath("Pinned.profile"));
        const std::vector<unsigned char> vExpected = ExpectedProfileFile();
        const size_t uiFirstDiff = FirstDifference(vFile, vExpected);
        INFO(uiFirstDiff);
        CHECK(vFile.size() == 4356);
        CHECK(vFile == vExpected);

        std::unique_ptr<PlayerProfile::tProfile> pLoaded(new PlayerProfile::tProfile);
        memset(pLoaded.get(), 0xCD, sizeof(PlayerProfile::tProfile));
        LogCapture kCapture;
        CHECK(ReadLikeSaveFsm(*pDevice, kProfileName, 1, pLoaded.get(), sizeof(PlayerProfile::tProfile)) == 0);
        CHECK(memcmp(pLoaded.get(), pProfile.get(), sizeof(PlayerProfile::tProfile)) == 0);
        CHECK(PlayerProfile::CheckLoadedCRC(*pLoaded, kProfileName));
        CHECK(kCapture.Count("OPENSWBF2 SAVE") == 0);
    }

    SUBCASE("metagame")
    {
        CheckItemsRoundTrip(kMetagameName, 2, "PinnedGC.gc", 512, kMetagameFileCRC);
    }

    SUBCASE("campaign")
    {
        CheckItemsRoundTrip(kCampaignName, 3, "PinnedRC.rote", 256, kCampaignFileCRC);
    }
}

TEST_CASE("save files cut short are rejected with one log line and no read past the end" * doctest::test_suite("regression"))
{
    TempSaveDir kDir;
    REQUIRE(kDir.Ok());
    std::unique_ptr<SaveDevice> pDevice(new SaveDevice);
    std::unique_ptr<PlayerProfile::tProfile> pProfile(new PlayerProfile::tProfile);
    FillProfile(*pProfile);
    REQUIRE(WriteLikeSaveFsm(*pDevice, kProfileName, 1, pProfile.get(), sizeof(PlayerProfile::tProfile)));
    const std::string sProfile = kDir.SavePath("Pinned.profile");
    const std::vector<unsigned char> vFull = ReadFileBytes(sProfile);
    REQUIRE(vFull.size() == 4356);

    REQUIRE(WriteFileBytes(sProfile, &vFull[0], vFull.size() - 1));
    std::vector<unsigned char> vDest(sizeof(PlayerProfile::tProfile) + 16, 0xA5);
    {
        LogCapture kCapture;
        CHECK(ReadLikeSaveFsm(*pDevice, kProfileName, 1, &vDest[0], sizeof(PlayerProfile::tProfile)) == -5);
        CHECK(kCapture.Count("OPENSWBF2 SAVE rejected short file \"Pinned.profile\" read 4351 of 4352 bytes") == 1);
        CHECK(kCapture.Count("OPENSWBF2 SAVE") == 1);
    }
    CHECK(memcmp(&vDest[0], &vFull[4], 4351) == 0);
    for (size_t i = 4351; i < vDest.size(); ++i)
    {
        INFO(i);
        CHECK(vDest[i] == 0xA5);
    }

    REQUIRE(WriteFileBytes(sProfile, &vFull[0], 2));
    memset(&vDest[0], 0xA5, vDest.size());
    {
        LogCapture kCapture;
        CHECK(ReadLikeSaveFsm(*pDevice, kProfileName, 1, &vDest[0], sizeof(PlayerProfile::tProfile)) == -5);
        CHECK(kCapture.Count("OPENSWBF2 SAVE rejected short file \"Pinned.profile\" read 2 of 4 bytes") == 1);
        CHECK(kCapture.Count("OPENSWBF2 SAVE") == 1);
    }
    for (size_t i = 0; i < vDest.size(); ++i)
    {
        INFO(i);
        CHECK(vDest[i] == 0xA5);
    }

    std::vector<unsigned char> vCampaign = BuildItemsBuffer(256);
    REQUIRE(WriteLikeSaveFsm(*pDevice, kCampaignName, 3, &vCampaign[0], 256));
    const std::string sCampaign = kDir.SavePath("PinnedRC.rote");
    const std::vector<unsigned char> vCampaignFile = ReadFileBytes(sCampaign);
    REQUIRE(vCampaignFile.size() == 260);
    REQUIRE(WriteFileBytes(sCampaign, &vCampaignFile[0], vCampaignFile.size() - 1));
    std::vector<unsigned char> vCampaignDest(256 + 16, 0xA5);
    {
        LogCapture kCapture;
        CHECK(ReadLikeSaveFsm(*pDevice, kCampaignName, 3, &vCampaignDest[0], 256) == -5);
        CHECK(kCapture.Count("OPENSWBF2 SAVE rejected short file \"PinnedRC.rote\" read 255 of 256 bytes") == 1);
        CHECK(kCapture.Count("OPENSWBF2 SAVE") == 1);
    }
    for (size_t i = 255; i < vCampaignDest.size(); ++i)
    {
        INFO(i);
        CHECK(vCampaignDest[i] == 0xA5);
    }
}

TEST_CASE("loaded profile with a bad CRC logs one line naming the file" * doctest::test_suite("regression"))
{
    TempSaveDir kDir;
    REQUIRE(kDir.Ok());
    std::unique_ptr<SaveDevice> pDevice(new SaveDevice);
    std::unique_ptr<PlayerProfile::tProfile> pProfile(new PlayerProfile::tProfile);
    FillProfile(*pProfile);
    pProfile->mCRC ^= 1u;
    REQUIRE(WriteLikeSaveFsm(*pDevice, kProfileName, 1, pProfile.get(), sizeof(PlayerProfile::tProfile)));

    std::unique_ptr<PlayerProfile::tProfile> pLoaded(new PlayerProfile::tProfile);
    memset(pLoaded.get(), 0xCD, sizeof(PlayerProfile::tProfile));
    REQUIRE(ReadLikeSaveFsm(*pDevice, kProfileName, 1, pLoaded.get(), sizeof(PlayerProfile::tProfile)) == 0);
    {
        LogCapture kCapture;
        CHECK_FALSE(PlayerProfile::CheckLoadedCRC(*pLoaded, kProfileName));
        CHECK(kCapture.Count("OPENSWBF2 SAVE profile CRC mismatch \"Pinned.profile\"") == 1);
        CHECK(kCapture.Count("OPENSWBF2 SAVE") == 1);
    }

    pLoaded->mCRC ^= 1u;
    pLoaded->mIsDirty = false;
    {
        LogCapture kCapture;
        CHECK(PlayerProfile::CheckLoadedCRC(*pLoaded, kProfileName));
        CHECK(kCapture.Count("OPENSWBF2 SAVE") == 0);
    }

    pLoaded->mBonus ^= 0x100u;
    {
        LogCapture kCapture;
        CHECK_FALSE(PlayerProfile::CheckLoadedCRC(*pLoaded, kProfileName));
        CHECK(kCapture.Count("OPENSWBF2 SAVE profile CRC mismatch \"Pinned.profile\"") == 1);
    }
}

TEST_CASE("profile loads check the stored CRC before the sanity check (source pin: the callback needs the game runtime)" * doctest::test_suite("regression"))
{
    std::string sText;
    REQUIRE(ReadRepoText("src\\Battlefront2\\Source\\LuaCallbacks_LoadSave.cpp", sText));
    const size_t uiStart = sText.find("static void LoadProfileFileDoneCallback()");
    REQUIRE(uiStart != std::string::npos);
    const size_t uiCheck = sText.find("PlayerProfile::CheckLoadedCRC(*PlayerProfile::GetProfile(0), sLoadProfileName);", uiStart);
    const size_t uiDone = sText.find("LoadProfileDoneCallback();", uiStart);
    REQUIRE(uiCheck != std::string::npos);
    REQUIRE(uiDone != std::string::npos);
    CHECK(uiCheck < uiDone);
    CHECK(sText.find("SaveGameManager::FinishQueue(LoadProfileFileDoneCallback);") != std::string::npos);
    CHECK(sText.find("PblUnicode::ustrlcpy(sLoadProfileName, pusName,") != std::string::npos);
}
