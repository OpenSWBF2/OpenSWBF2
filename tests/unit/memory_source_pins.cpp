#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <string.h>
#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "testutil.h"

namespace
{

std::string HardenSource(const char* pcRel)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRel, sText));
    return sText;
}

std::string HardenBody(const std::string& sText, const char* pcSignature)
{
    size_t uiStart = sText.find(pcSignature);
    REQUIRE_MESSAGE(uiStart != std::string::npos, pcSignature);
    REQUIRE_MESSAGE(sText.find(pcSignature, uiStart + 1) == std::string::npos, pcSignature);
    size_t uiEnd = sText.find("\n}\n", uiStart);
    REQUIRE_MESSAGE(uiEnd != std::string::npos, pcSignature);
    return sText.substr(uiStart, uiEnd + 2 - uiStart);
}

std::string PinRepoRoot()
{
    char acExe[MAX_PATH];
    DWORD uLen = GetModuleFileNameA(0, acExe, sizeof(acExe));
    REQUIRE(uLen > 0);
    REQUIRE(uLen < sizeof(acExe));
    std::string sPath(acExe, uLen);
    for (int i = 0; i < 4; ++i)
    {
        size_t uiSlash = sPath.find_last_of("\\/");
        REQUIRE(uiSlash != std::string::npos);
        sPath.erase(uiSlash);
    }
    return sPath;
}

void PinListSources(const std::string& sDir, const char* pcPattern, std::vector<std::string>& vOut)
{
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((sDir + pcPattern).c_str(), &kFind);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        if (!(kFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            vOut.push_back(kFind.cFileName);
        }
    } while (FindNextFileA(hFind, &kFind));
    FindClose(hFind);
}

bool PinHasSuffix(const std::string& sName, const char* pcSuffix)
{
    const size_t uiLen = strlen(pcSuffix);
    return sName.size() >= uiLen && _stricmp(sName.c_str() + sName.size() - uiLen, pcSuffix) == 0;
}

void PinWalkSources(const std::string& sRoot, const std::string& sRel, std::vector<std::string>& vOut)
{
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((sRoot + "\\" + sRel + "*").c_str(), &kFind);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        const std::string sName = kFind.cFileName;
        if (sName == "." || sName == "..")
        {
            continue;
        }
        if (kFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            PinWalkSources(sRoot, sRel + sName + "\\", vOut);
        }
        else if (PinHasSuffix(sName, ".cpp") || PinHasSuffix(sName, ".h"))
        {
            vOut.push_back(sRel + sName);
        }
    } while (FindNextFileA(hFind, &kFind));
    FindClose(hFind);
}

void PinSplitLines(const std::string& sText, std::vector<std::string>& vOut)
{
    size_t uiStart = 0;
    while (uiStart <= sText.size())
    {
        size_t uiEnd = sText.find('\n', uiStart);
        if (uiEnd == std::string::npos)
        {
            uiEnd = sText.size();
        }
        std::string sLine = sText.substr(uiStart, uiEnd - uiStart);
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        vOut.push_back(sLine);
        uiStart = uiEnd + 1;
    }
}

bool PinIdentChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

std::string PinIdentBefore(const std::string& sLine, size_t uiPos)
{
    size_t uiEnd = uiPos;
    while (uiEnd > 0 && sLine[uiEnd - 1] == ' ')
    {
        --uiEnd;
    }
    size_t uiBegin = uiEnd;
    while (uiBegin > 0 && PinIdentChar(sLine[uiBegin - 1]))
    {
        --uiBegin;
    }
    return sLine.substr(uiBegin, uiEnd - uiBegin);
}

bool PinIsSignature(const std::string& sLine)
{
    if (sLine.empty() || sLine[0] == ' ' || sLine[0] == '\t' || sLine[0] == '#' || sLine[0] == '{' ||
        sLine[0] == '}')
    {
        return false;
    }
    if (sLine.compare(0, 2, "//") == 0 || sLine.compare(0, 2, "/*") == 0)
    {
        return false;
    }
    if (sLine.find('(') == std::string::npos)
    {
        return false;
    }
    const size_t uiLast = sLine.find_last_not_of(" \t");
    return sLine[uiLast] != ';';
}

void SplitFunctions(const std::string& sText, std::vector<std::pair<std::string, std::string>>& vOut)
{
    std::vector<std::string> vLines;
    PinSplitLines(sText, vLines);
    std::vector<size_t> vOffsets;
    size_t uiOffset = 0;
    for (size_t i = 0; i < vLines.size(); ++i)
    {
        vOffsets.push_back(uiOffset);
        uiOffset += vLines[i].size() + 1;
    }
    size_t i = 0;
    while (i < vLines.size())
    {
        if (!PinIsSignature(vLines[i]))
        {
            ++i;
            continue;
        }
        size_t j = i + 1;
        while (j < vLines.size() && !vLines[j].empty() && (vLines[j][0] == ' ' || vLines[j][0] == '\t') &&
               vLines[j].find(';') == std::string::npos)
        {
            ++j;
        }
        if (j >= vLines.size() || vLines[j].compare(0, 1, "{") != 0)
        {
            ++i;
            continue;
        }
        const size_t uiEnd = sText.find("\n}\n", vOffsets[j]);
        if (uiEnd == std::string::npos)
        {
            ++i;
            continue;
        }
        const std::string sName = PinIdentBefore(vLines[i], vLines[i].find('('));
        vOut.push_back(std::make_pair(sName, sText.substr(vOffsets[i], uiEnd + 2 - vOffsets[i])));
        while (i < vLines.size() && vOffsets[i] <= uiEnd + 1)
        {
            ++i;
        }
    }
}

bool PinHasAny(const std::string& sBody, const char* const* ppcTokens, size_t uiCount)
{
    for (size_t i = 0; i < uiCount; ++i)
    {
        if (sBody.find(ppcTokens[i]) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

void ClampViolations(const std::string& sText, std::vector<std::string>& vOut)
{
    static const char* const apcSinks[] = {
        "RedAllocFromHeap(", "RedCreateHeap(", "RedMoveHeapEnd(", "SetCount(", "SetHeapCap(",
    };
    static const char* const apcReads[] = {
        "LuaHelper::Get", "luaL_check", "lua_to", "ReadScriptInt(",
    };
    std::vector<std::pair<std::string, std::string>> vFunctions;
    SplitFunctions(sText, vFunctions);
    for (size_t i = 0; i < vFunctions.size(); ++i)
    {
        const std::string& sBody = vFunctions[i].second;
        if (PinHasAny(sBody, apcSinks, sizeof(apcSinks) / sizeof(apcSinks[0])) &&
            PinHasAny(sBody, apcReads, sizeof(apcReads) / sizeof(apcReads[0])) &&
            sBody.find("ClampScriptSize(") == std::string::npos)
        {
            vOut.push_back(vFunctions[i].first);
        }
    }
}

std::string PinEnclosingClass(const std::vector<std::string>& vLines, size_t uiLine)
{
    for (size_t i = uiLine + 1; i-- > 0;)
    {
        const std::string& sLine = vLines[i];
        if (sLine.compare(0, 6, "class ") == 0 || sLine.compare(0, 7, "struct ") == 0)
        {
            const size_t uiLast = sLine.find_last_not_of(" \t");
            if (sLine[uiLast] == ';')
            {
                continue;
            }
            size_t uiBegin = sLine.find(' ') + 1;
            size_t uiEnd = uiBegin;
            while (uiEnd < sLine.size() && PinIdentChar(sLine[uiEnd]))
            {
                ++uiEnd;
            }
            return sLine.substr(uiBegin, uiEnd - uiBegin);
        }
        if (PinIsSignature(sLine))
        {
            const size_t uiParen = sLine.find('(');
            const size_t uiScope = sLine.rfind("::", uiParen);
            if (uiScope != std::string::npos)
            {
                return PinIdentBefore(sLine, uiScope);
            }
        }
    }
    return std::string();
}

}

TEST_CASE("script-size-clamp: lua memory sinks route through the shared clamp" * doctest::test_suite("regression"))
{
    const std::string sRel = "src\\Battlefront2\\Source\\";
    std::vector<std::string> vFiles;
    PinListSources(PinRepoRoot() + "\\" + sRel, "LuaCallbacks*.cpp", vFiles);
    vFiles.push_back("LuaMemoryArgs.cpp");
    REQUIRE(vFiles.size() >= 8);

    for (size_t i = 0; i < vFiles.size(); ++i)
    {
        const std::string sText = HardenSource((sRel + vFiles[i]).c_str());
        std::vector<std::string> vBad;
        ClampViolations(sText, vBad);
        for (size_t j = 0; j < vBad.size(); ++j)
        {
            const std::string sViolation = vFiles[i] + "::" + vBad[j];
            CHECK_MESSAGE(false, sViolation);
        }
    }

    const std::string sArgs = HardenSource("src\\Battlefront2\\Source\\LuaMemoryArgs.cpp");
    std::vector<std::pair<std::string, std::string>> vFunctions;
    SplitFunctions(sArgs, vFunctions);
    const char* const apcEntries[] = { "SetMemoryPoolSize", "SetupTempHeap", "StealArtistHeap", "AddWalkerType" };
    for (size_t i = 0; i < sizeof(apcEntries) / sizeof(apcEntries[0]); ++i)
    {
        INFO(apcEntries[i]);
        int iFound = 0;
        for (size_t j = 0; j < vFunctions.size(); ++j)
        {
            if (vFunctions[j].first == apcEntries[i])
            {
                ++iFound;
                CHECK(vFunctions[j].second.find("ClampScriptSize(") != std::string::npos);
            }
        }
        CHECK(iFound == 1);
    }
    CHECK(HardenBody(sArgs, "void StealArtistHeap(").find("ClampScriptSize(") != std::string::npos);
}

TEST_CASE("script-size-clamp: checker flags an unclamped sink" * doctest::test_suite("regression"))
{
    std::vector<std::string> vBad;
    ClampViolations("int Bad(lua_State* L)\n{\n    RedAllocFromHeap(0, LuaHelper::GetInt(1), 0);\n    return 0;\n}\n",
                    vBad);
    REQUIRE(vBad.size() == 1);
    CHECK(vBad[0] == "Bad");

    vBad.clear();
    ClampViolations("int Bad(lua_State* L)\n{\n    pPool->SetCount(0);\n    return 0;\n}\n", vBad);
    CHECK(vBad.empty());

    vBad.clear();
    ClampViolations("int Bad(lua_State* L)\n{\n    RedAllocFromHeap(0, ClampScriptSize(0, 0, LuaHelper::GetInt(1), 1), "
                    "0);\n    return 0;\n}\n",
                    vBad);
    CHECK(vBad.empty());
}

TEST_CASE("gated pools: IsFull and IsSufficientSpace sites match the pinned list" * doctest::test_suite("regression"))
{
    const char* const apcPinned[][2] = {
        { "Combo.cpp", "Combo" },
        { "Combo.cpp", "DamageSample" },
        { "CommandFlyer.cpp", "CommandFlyer" },
        { "CommandHover.cpp", "CommandHover" },
        { "CommandWalker.cpp", "CommandWalker" },
        { "DisplaySoldier.cpp", "TentacleSimulator" },
        { "EntityCarrier.cpp", "EntityCarrier" },
        { "EntityDroid.cpp", "EntityDroid" },
        { "EntityDroideka.cpp", "EntityDroideka" },
        { "EntityFlyer.cpp", "EntityFlyer" },
        { "EntityHover.cpp", "EntityHover" },
        { "EntitySoldier.cpp", "TentacleSimulator" },
        { "EntitySoldier.cpp", "EntitySoldier" },
        { "EntitySoldier.cpp", "PowerupItem" },
        { "EntityWalker.cpp", "EntityWalker" },
        { "FlagItem.cpp", "FlagItem" },
        { "LuaCallbacks_Mission.cpp", "Timer" },
        { "OrdnanceHaywire.cpp", "Ordnance" },
        { "OrdnanceMeleeThrow.cpp", "Ordnance" },
        { "ParticleEmitter.h", "ParticleEmitter" },
        { "ParticleEmitterInfo.h", "ParticleEmitterInfoData" },
        { "PathFinder.cpp", "AIPath" },
        { "PathFinder.cpp", "AIPathNode" },
        { "PathFinder.cpp", "PathNode" },
        { "PathManager.cpp", "AIPathNode" },
        { "VehicleSpawn.cpp", "EntityCarrier" },
        { "Weapon.cpp", "Weapon" },
        { "WeaponMelee.cpp", "Combo" },
    };
    std::set<std::pair<std::string, std::string>> kPinned;
    std::set<std::string> kPinnedClasses;
    for (size_t i = 0; i < sizeof(apcPinned) / sizeof(apcPinned[0]); ++i)
    {
        kPinned.insert(std::make_pair(std::string(apcPinned[i][0]), std::string(apcPinned[i][1])));
        kPinnedClasses.insert(apcPinned[i][1]);
    }
    REQUIRE(kPinned.size() == 28);
    CHECK(kPinnedClasses.size() == 23);

    const std::string sRoot = PinRepoRoot();
    std::vector<std::string> vFiles;
    PinWalkSources(sRoot, "src\\", vFiles);
    REQUIRE(vFiles.size() > 1000);

    const char* const apcGates[] = { "sMemoryPool.IsFull()", "sMemoryPool.IsSufficientSpace(" };
    std::set<std::pair<std::string, std::string>> kFound;
    for (size_t i = 0; i < vFiles.size(); ++i)
    {
        std::string sText;
        REQUIRE(ReadRepoText(vFiles[i].c_str(), sText));
        if (sText.find("sMemoryPool.Is") == std::string::npos)
        {
            continue;
        }
        const std::string sName = vFiles[i].substr(vFiles[i].find_last_of('\\') + 1);
        std::vector<std::string> vLines;
        PinSplitLines(sText, vLines);
        for (size_t j = 0; j < vLines.size(); ++j)
        {
            for (size_t k = 0; k < sizeof(apcGates) / sizeof(apcGates[0]); ++k)
            {
                const size_t uiGateLen = strlen(apcGates[k]);
                for (size_t uiPos = vLines[j].find(apcGates[k]); uiPos != std::string::npos;
                     uiPos = vLines[j].find(apcGates[k], uiPos + uiGateLen))
                {
                    std::string sClass;
                    if (uiPos >= 2 && vLines[j].compare(uiPos - 2, 2, "::") == 0)
                    {
                        sClass = PinIdentBefore(vLines[j], uiPos - 2);
                    }
                    else
                    {
                        sClass = PinEnclosingClass(vLines, j);
                    }
                    INFO(vFiles[i]);
                    INFO(vLines[j]);
                    CHECK(!sClass.empty());
                    kFound.insert(std::make_pair(sName, sClass));
                }
            }
        }
    }

    for (std::set<std::pair<std::string, std::string>>::const_iterator it = kPinned.begin(); it != kPinned.end(); ++it)
    {
        const std::string sMissing = "missing " + it->first + " " + it->second;
        CHECK_MESSAGE(kFound.count(*it) == 1, sMissing);
    }
    for (std::set<std::pair<std::string, std::string>>::const_iterator it = kFound.begin(); it != kFound.end(); ++it)
    {
        const std::string sExtra = "extra " + it->first + " " + it->second;
        CHECK_MESSAGE(kPinned.count(*it) == 1, sExtra);
    }
    CHECK(kFound == kPinned);
}
