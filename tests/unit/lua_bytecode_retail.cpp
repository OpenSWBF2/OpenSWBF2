#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "testutil.h"
#include "LuaFL/include/lua.h"
#include "LuaFL/include/lauxlib.h"
#include "LuaFL/include/lualib.h"
#include "lstate.h"
#include "lobject.h"
#include "lfunc.h"
#include "lmem.h"

namespace
{

std::string RetailRepoRoot()
{
    char acExe[MAX_PATH];
    DWORD uLen = GetModuleFileNameA(0, acExe, sizeof(acExe));
    if (uLen == 0 || uLen >= sizeof(acExe))
    {
        return std::string();
    }
    std::string sPath(acExe, uLen);
    for (;;)
    {
        size_t uiSlash = sPath.find_last_of("\\/");
        if (uiSlash == std::string::npos)
        {
            return std::string();
        }
        sPath.erase(uiSlash);
        if (GetFileAttributesA((sPath + "\\data\\scripts\\index.json").c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            return sPath;
        }
    }
}

std::string LowerCase(const std::string& sText)
{
    std::string sOut(sText);
    for (size_t i = 0; i < sOut.size(); ++i)
    {
        if (sOut[i] >= 'A' && sOut[i] <= 'Z')
        {
            sOut[i] = (char)(sOut[i] - 'A' + 'a');
        }
    }
    return sOut;
}

bool EndsWithNoCase(const char* pcName, const char* pcExt)
{
    size_t uLen = strlen(pcName);
    size_t uExt = strlen(pcExt);
    return uLen > uExt && _stricmp(pcName + uLen - uExt, pcExt) == 0;
}

void ListFilesRecursive(const std::string& sDir, const std::string& sRel, const char* pcExt,
                        std::vector<std::string>& asOut)
{
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((sDir + "\\*").c_str(), &kFind);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        if (strcmp(kFind.cFileName, ".") == 0 || strcmp(kFind.cFileName, "..") == 0)
        {
            continue;
        }
        std::string sChildRel = sRel.empty() ? std::string(kFind.cFileName) : sRel + "/" + kFind.cFileName;
        if (kFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            ListFilesRecursive(sDir + "\\" + kFind.cFileName, sChildRel, pcExt, asOut);
        }
        else if (EndsWithNoCase(kFind.cFileName, pcExt))
        {
            asOut.push_back(sChildRel);
        }
    } while (FindNextFileA(hFind, &kFind));
    FindClose(hFind);
}

bool ReadWholeFile(const std::string& sPath, std::string& sOut)
{
    sOut.clear();
    FILE* pFile = fopen(sPath.c_str(), "rb");
    if (!pFile)
    {
        return false;
    }
    bool bOk = fseek(pFile, 0, SEEK_END) == 0;
    long lSize = bOk ? ftell(pFile) : -1;
    bOk = lSize >= 0 && fseek(pFile, 0, SEEK_SET) == 0;
    if (bOk && lSize > 0)
    {
        sOut.resize((size_t)lSize);
        bOk = fread(&sOut[0], 1, (size_t)lSize, pFile) == (size_t)lSize;
    }
    fclose(pFile);
    return bOk;
}

struct RetailOrderGuard
{
    explicit RetailOrderGuard(int iSerial)
    {
        lua_setinsertionorder(iSerial);
    }
    ~RetailOrderGuard()
    {
        lua_setinsertionorder(0);
    }
};

struct RetailState
{
    explicit RetailState(int iSerial) : kGuard(iSerial), L(lua_open()) {}
    ~RetailState()
    {
        if (L)
        {
            lua_close(L);
        }
    }

    RetailOrderGuard kGuard;
    lua_State* L;
};

const char* RetailOrderName(int iSerial)
{
    return iSerial ? "serial" : "address";
}

int WriteToString(lua_State*, const void* p, size_t sz, void* ud)
{
    static_cast<std::string*>(ud)->append(static_cast<const char*>(p), sz);
    return 0;
}

void StripProto(lua_State* L, Proto* pProto)
{
    luaM_freearray(L, pProto->lineinfo, pProto->sizelineinfo, int);
    pProto->lineinfo = NULL;
    pProto->sizelineinfo = 0;
    luaM_freearray(L, pProto->locvars, pProto->sizelocvars, struct LocVar);
    pProto->locvars = NULL;
    pProto->sizelocvars = 0;
    luaM_freearray(L, pProto->upvalues, pProto->sizeupvalues, TString*);
    pProto->upvalues = NULL;
    pProto->sizeupvalues = 0;
    for (int i = 0; i < pProto->sizep; ++i)
    {
        StripProto(L, pProto->p[i]);
    }
}

struct RetailScript
{
    std::string sLvl;
    std::string sName;
    std::string sSource;
};

bool NextJsonString(const std::string& sText, const char* pcKey, size_t& uPos, std::string& sOut)
{
    const std::string sKey = std::string("\"") + pcKey + "\"";
    size_t uAt = sText.find(sKey, uPos);
    if (uAt == std::string::npos)
    {
        return false;
    }
    size_t u = uAt + sKey.size();
    while (u < sText.size() && (sText[u] == ' ' || sText[u] == ':' || sText[u] == '\t'))
    {
        ++u;
    }
    if (u >= sText.size() || sText[u] != '"')
    {
        return false;
    }
    size_t uEnd = sText.find('"', u + 1);
    if (uEnd == std::string::npos)
    {
        return false;
    }
    sOut = sText.substr(u + 1, uEnd - u - 1);
    uPos = uEnd + 1;
    return true;
}

bool ReadRetailIndex(const std::string& sRoot, std::vector<RetailScript>& akOut)
{
    std::string sText;
    if (!ReadWholeFile(sRoot + "\\data\\scripts\\index.json", sText))
    {
        return false;
    }
    size_t uPos = 0;
    RetailScript kScript;
    while (NextJsonString(sText, "lvl", uPos, kScript.sLvl))
    {
        if (!NextJsonString(sText, "name", uPos, kScript.sName) ||
            !NextJsonString(sText, "source", uPos, kScript.sSource))
        {
            return false;
        }
        akOut.push_back(kScript);
    }
    return !akOut.empty();
}

bool DumpDataScripts(lua_State* L, const std::string& sRoot, bool bStrip, std::vector<std::string>& asChunks,
                     std::vector<std::string>& asNames)
{
    std::string sDir = sRoot + "\\data\\scripts";
    std::vector<std::string> asFiles;
    ListFilesRecursive(sDir, std::string(), ".lua", asFiles);
    if (asFiles.empty())
    {
        return false;
    }
    for (size_t i = 0; i < asFiles.size(); ++i)
    {
        std::string sText;
        if (!ReadWholeFile(sDir + "\\" + asFiles[i], sText))
        {
            return false;
        }
        std::string sChunkName = "=" + asFiles[i];
        if (luaL_loadbuffer(L, sText.data(), sText.size(), sChunkName.c_str()) != 0)
        {
            lua_settop(L, 0);
            return false;
        }
        if (bStrip)
        {
            StripProto(L, clvalue(L->top - 1)->l.p);
        }
        std::string sDump;
        lua_dump(L, WriteToString, &sDump);
        lua_settop(L, 0);
        asChunks.push_back(sDump);
        asNames.push_back(asFiles[i]);
    }
    return true;
}

unsigned int ReadU32(const unsigned char* p)
{
    unsigned int u = 0;
    memcpy(&u, p, 4);
    return u;
}

bool ReadAt(FILE* pFile, long lPos, void* pOut, size_t uSize)
{
    return fseek(pFile, lPos, SEEK_SET) == 0 && fread(pOut, 1, uSize, pFile) == uSize;
}

struct LvlWalk
{
    lua_State* L;
    FILE* pFile;
    std::string sFile;
    int iChunks;
    std::set<std::string>* psFound;
};

bool LoadScrChunk(LvlWalk& kWalk, size_t uStart, size_t uEnd)
{
    std::string sName;
    std::string sBody;
    bool bHaveBody = false;
    size_t uPos = uStart;
    while (uPos + 8 <= uEnd)
    {
        unsigned char acHeader[8];
        if (!ReadAt(kWalk.pFile, (long)uPos, acHeader, 8))
        {
            return false;
        }
        unsigned int uSize = ReadU32(acHeader + 4);
        size_t uData = uPos + 8;
        if (uSize > uEnd - uData)
        {
            return false;
        }
        if (memcmp(acHeader, "NAME", 4) == 0 || memcmp(acHeader, "BODY", 4) == 0)
        {
            std::string sBytes(uSize, '\0');
            if (uSize > 0 && !ReadAt(kWalk.pFile, (long)uData, &sBytes[0], uSize))
            {
                return false;
            }
            if (memcmp(acHeader, "NAME", 4) == 0)
            {
                sName.assign(sBytes.c_str());
            }
            else
            {
                sBody.swap(sBytes);
                bHaveBody = true;
            }
        }
        uPos = uData + uSize;
        uPos += (4 - (uPos & 3)) & 3;
    }
    if (!bHaveBody)
    {
        return true;
    }
    std::string sChunkName = "=" + sName;
    int iStatus = luaL_loadbuffer(kWalk.L, sBody.data(), sBody.size(), sChunkName.c_str());
    const char* pcMsg = iStatus != 0 ? lua_tostring(kWalk.L, -1) : 0;
    std::string sMsg = pcMsg ? pcMsg : "";
    INFO(kWalk.sFile << " " << sName << ": " << sMsg);
    CHECK(iStatus == 0);
    lua_settop(kWalk.L, 0);
    ++kWalk.iChunks;
    kWalk.psFound->insert(LowerCase(kWalk.sFile + "|" + sName));
    return true;
}

bool WalkLvlChunks(LvlWalk& kWalk, size_t uStart, size_t uEnd, int iDepth)
{
    if (iDepth > 16)
    {
        return false;
    }
    size_t uPos = uStart;
    while (uPos + 8 <= uEnd)
    {
        unsigned char acHeader[8];
        if (!ReadAt(kWalk.pFile, (long)uPos, acHeader, 8))
        {
            return false;
        }
        unsigned int uSize = ReadU32(acHeader + 4);
        size_t uData = uPos + 8;
        if (uSize > uEnd - uData)
        {
            return false;
        }
        if (memcmp(acHeader, "lvl_", 4) == 0)
        {
            if (uSize < 8)
            {
                return false;
            }
            unsigned char acPrefix[8];
            if (!ReadAt(kWalk.pFile, (long)uData, acPrefix, 8))
            {
                return false;
            }
            unsigned int uInner = ReadU32(acPrefix + 4);
            if (uInner > uSize - 8)
            {
                return false;
            }
            if (!WalkLvlChunks(kWalk, uData + 8, uData + 8 + uInner, iDepth + 1))
            {
                return false;
            }
        }
        else if (memcmp(acHeader, "scr_", 4) == 0)
        {
            if (!LoadScrChunk(kWalk, uData, uData + uSize))
            {
                return false;
            }
        }
        uPos = uData + uSize;
        uPos += (4 - (uPos & 3)) & 3;
    }
    return true;
}

bool WalkLvlFile(LvlWalk& kWalk)
{
    if (fseek(kWalk.pFile, 0, SEEK_END) != 0)
    {
        return false;
    }
    long lFileSize = ftell(kWalk.pFile);
    if (lFileSize < 8)
    {
        return false;
    }
    unsigned char acHeader[8];
    if (!ReadAt(kWalk.pFile, 0, acHeader, 8) || memcmp(acHeader, "ucfb", 4) != 0)
    {
        return false;
    }
    unsigned int uSize = ReadU32(acHeader + 4);
    if (uSize > (size_t)lFileSize - 8)
    {
        return false;
    }
    return WalkLvlChunks(kWalk, 8, 8 + (size_t)uSize, 0);
}

std::vector<std::string> LuaFuzzBases()
{
    const std::string sRoot = RetailRepoRoot();
    SKIP_IF(sRoot.empty(), "data/scripts not found");
    std::vector<std::string> asBases;
    std::vector<std::string> asNames;
    lua_State* L = lua_open();
    REQUIRE(L);
    bool bOk = DumpDataScripts(L, sRoot, false, asBases, asNames) && DumpDataScripts(L, sRoot, true, asBases, asNames);
    lua_close(L);
    REQUIRE(bOk);
    REQUIRE(!asBases.empty());
    return asBases;
}

std::string LuaMutate(unsigned int uSeed, unsigned int uIter, const std::vector<std::string>& asBases)
{
    std::mt19937 rng(uSeed ^ (uIter * 0x9E3779B1u));
    std::string b = asBases[rng() % asBases.size()];
    static const unsigned char acInteresting[5] = {0, 1, 0x7F, 0x80, 0xFF};
    int iMutations = 1 + (int)(rng() % 4);
    for (int m = 0; m < iMutations; ++m)
    {
        switch (rng() % 6)
        {
        case 0:
            if (!b.empty())
            {
                size_t uPos = rng() % b.size();
                unsigned int uBit = rng() % 8;
                b[uPos] = (char)((unsigned char)b[uPos] ^ (unsigned char)(1u << uBit));
            }
            break;
        case 1:
            if (!b.empty())
            {
                unsigned int uVal = rng() % 5;
                size_t uPos = rng() % b.size();
                b[uPos] = (char)acInteresting[uVal];
            }
            break;
        case 2:
        {
            char cByte = (char)rng();
            size_t uPos = rng() % (b.size() + 1);
            b.insert(b.begin() + uPos, cByte);
            break;
        }
        case 3:
            if (!b.empty())
            {
                b.erase(b.begin() + rng() % b.size());
            }
            break;
        case 4:
            b.resize(rng() % (b.size() + 1));
            break;
        default:
        {
            const std::string& other = asBases[rng() % asBases.size()];
            size_t uCut = rng() % (b.size() + 1);
            size_t uFrom = rng() % (other.size() + 1);
            b.resize(uCut);
            b.append(other, uFrom, std::string::npos);
            break;
        }
        }
    }
    return b;
}

unsigned int LuaFuzzFnv(unsigned int uSum, const std::string& b)
{
    for (size_t i = 0; i < b.size(); ++i)
    {
        uSum = (uSum ^ (unsigned char)b[i]) * 16777619u;
    }
    return (uSum ^ (unsigned int)b.size()) * 16777619u;
}

const int kLuaFuzzHookEvery = 64;
const int kLuaFuzzHookBudget = 64;
int sLuaFuzzHooks = 0;

void LuaFuzzStepHook(lua_State* L, lua_Debug*)
{
    G(L)->GCthreshold = 0;
    if (++sLuaFuzzHooks > kLuaFuzzHookBudget)
    {
        lua_sethook(L, LuaFuzzStepHook, LUA_MASKCOUNT, 1);
        luaL_error(L, "fuzz step limit");
    }
}

void LuaFuzzOpenState(lua_State* L)
{
    luaopen_base(L);
    lua_settop(L, 0);
    const char* apcDrop[3] = { "print", "coroutine", "newproxy" };
    for (int i = 0; i < 3; ++i)
    {
        lua_pushnil(L);
        lua_setglobal(L, apcDrop[i]);
    }
}

bool RunLuaFuzz(unsigned int uSeed, unsigned int uCount, unsigned int* puSum = 0)
{
    std::vector<std::string> asBases = LuaFuzzBases();
    unsigned int uSum = 2166136261u;
    unsigned int uAccepted = 0;
    unsigned int uCompleted = 0;
    for (unsigned int uIter = 0; uIter < uCount; ++uIter)
    {
        char acLabel[64];
        sprintf(acLabel, "seed=%u iter=%u", uSeed, uIter);
        WatchdogArm(acLabel);
        std::string sIn = LuaMutate(uSeed, uIter, asBases);
        uSum = LuaFuzzFnv(uSum, sIn);
        lua_State* L = lua_open();
        if (!L)
        {
            printf("lua fuzz FAIL lua_open: %s\n", acLabel);
            fflush(stdout);
            return false;
        }
        LuaFuzzOpenState(L);
        if (luaL_loadbuffer(L, sIn.data(), sIn.size(), "=fuzz") == 0)
        {
            ++uAccepted;
            lua_setgcthreshold(L, 0);
            sLuaFuzzHooks = 0;
            lua_sethook(L, LuaFuzzStepHook, LUA_MASKCOUNT, kLuaFuzzHookEvery);
            if (lua_pcall(L, 0, 0, 0) == 0)
            {
                ++uCompleted;
            }
            lua_settop(L, 0);
            lua_setgcthreshold(L, 0);
        }
        lua_close(L);
    }
    printf("lua-fuzz-checksum seed=%u count=%u accepted=%u completed=%u sum=0x%08x\n", uSeed, uCount, uAccepted,
           uCompleted, uSum);
    fflush(stdout);
    if (puSum)
    {
        *puSum = uSum;
    }
    return true;
}

}

TEST_CASE("lua retail scripts pass the verifier" * doctest::test_suite("unit"))
{
    const std::string sRoot = RetailRepoRoot();
    SKIP_IF(sRoot.empty(), "data/scripts not found");
    std::vector<RetailScript> akIndex;
    REQUIRE(ReadRetailIndex(sRoot, akIndex));
    for (int iSerial = 0; iSerial < 2; ++iSerial)
    {
        const char* pcOrder = RetailOrderName(iSerial);
        INFO("order " << pcOrder);
        RetailState kState(iSerial);
        lua_State* L = kState.L;
        REQUIRE(L);
        for (int iStrip = 0; iStrip < 2; ++iStrip)
        {
            std::vector<std::string> asChunks;
            std::vector<std::string> asNames;
            INFO("strip " << iStrip);
            REQUIRE(DumpDataScripts(L, sRoot, iStrip != 0, asChunks, asNames));
            CHECK(asChunks.size() == akIndex.size());
            std::set<std::string> sListed;
            for (size_t i = 0; i < asNames.size(); ++i)
            {
                sListed.insert(LowerCase("data/scripts/" + asNames[i]));
            }
            for (size_t i = 0; i < akIndex.size(); ++i)
            {
                INFO(akIndex[i].sSource);
                CHECK(sListed.count(LowerCase(akIndex[i].sSource)) == 1);
            }
            for (size_t i = 0; i < asChunks.size(); ++i)
            {
                std::string sChunkName = "=" + asNames[i];
                int iStatus = luaL_loadbuffer(L, asChunks[i].data(), asChunks[i].size(), sChunkName.c_str());
                const char* pcMsg = iStatus != 0 ? lua_tostring(L, -1) : 0;
                std::string sMsg = pcMsg ? pcMsg : "";
                INFO(asNames[i] << ": " << sMsg);
                CHECK(iStatus == 0);
                lua_settop(L, 0);
            }
        }
    }
}

TEST_CASE("lua retail lvl chunks pass the verifier" * doctest::test_suite("unit"))
{
    REQUIRE_GAMEDATA();
    const std::string sRoot = RetailRepoRoot();
    SKIP_IF(sRoot.empty(), "data/scripts not found");
    std::vector<RetailScript> akIndex;
    REQUIRE(ReadRetailIndex(sRoot, akIndex));
    std::string sDir = std::string(FindGameDataRoot()) + "\\data\\_lvl_pc";
    std::vector<std::string> asFiles;
    ListFilesRecursive(sDir, std::string(), ".lvl", asFiles);
    for (int iSerial = 0; iSerial < 2; ++iSerial)
    {
        const char* pcOrder = RetailOrderName(iSerial);
        INFO("order " << pcOrder);
        RetailState kState(iSerial);
        lua_State* L = kState.L;
        REQUIRE(L);
        int iChunks = 0;
        std::set<std::string> sFound;
        for (size_t i = 0; i < asFiles.size(); ++i)
        {
            LvlWalk kWalk;
            kWalk.L = L;
            kWalk.pFile = fopen((sDir + "\\" + asFiles[i]).c_str(), "rb");
            kWalk.sFile = asFiles[i];
            kWalk.iChunks = 0;
            kWalk.psFound = &sFound;
            INFO(asFiles[i]);
            REQUIRE(kWalk.pFile);
            bool bOk = WalkLvlFile(kWalk);
            fclose(kWalk.pFile);
            CHECK(bOk);
            iChunks += kWalk.iChunks;
        }
        CHECK(iChunks > 0);
        for (size_t i = 0; i < akIndex.size(); ++i)
        {
            const std::string sKey = LowerCase(akIndex[i].sLvl + ".lvl|" + akIndex[i].sName);
            INFO(sKey);
            CHECK(sFound.count(sKey) == 1);
        }
    }
}

TEST_CASE("lua bytecode fuzz default seeds" * doctest::test_suite("fuzz"))
{
    const unsigned int auSeeds[4] = {1, 2, 3, 0xC0FFEE};
    unsigned int auSums[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; ++i)
    {
        CHECK(RunLuaFuzz(auSeeds[i], 2500, &auSums[i]));
    }
    unsigned int uAgain = 0;
    CHECK(RunLuaFuzz(auSeeds[0], 2500, &uAgain));
    CHECK(uAgain == auSums[0]);
}

TEST_CASE("lua bytecode fuzz soak" * doctest::test_suite("stress"))
{
    if (gFuzzDump)
    {
        std::vector<std::string> asBases = LuaFuzzBases();
        REQUIRE(gFuzzOutPath[0] != 0);
        std::string sIn = LuaMutate(gFuzzSeed, gFuzzIter, asBases);
        FILE* pFile = fopen(gFuzzOutPath, "wb");
        REQUIRE(pFile);
        fwrite(sIn.data(), 1, sIn.size(), pFile);
        fclose(pFile);
        return;
    }
    CHECK(RunLuaFuzz(gFuzzSeed, gFuzzCount ? gFuzzCount : 100000));
}
