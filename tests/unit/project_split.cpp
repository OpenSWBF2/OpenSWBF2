#include "third_party/doctest/doctest.h"

#include <windows.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "testutil.h"

namespace
{

struct LibProject
{
    const char* pcDir;
    const char* pcProject;
};

const LibProject kLibs[] = {
    {"src\\LuaFL", "src\\LuaFL\\LuaFL.vcxproj"},
    {"src\\RedEngineFL", "src\\RedEngineFL\\RedEngineFL.vcxproj"},
    {"src\\mimalloc", "src\\mimalloc\\mimalloc.vcxproj"},
    {"src\\SoundFL", "src\\SoundFL\\SoundFL.vcxproj"},
    {"src\\Zephyr", "src\\Zephyr\\ZephyrFL.vcxproj"},
    {"src\\ZlibFL", "src\\ZlibFL\\ZLibFL.vcxproj"},
};

const LibProject kDlls[] = {
    {"src\\FFmpegFL", "src\\FFmpegFL\\FFmpegFL.vcxproj"},
};

std::string Lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)tolower(c); });
    return s;
}

std::vector<std::string> CompileItems(const std::string& sText)
{
    std::vector<std::string> vItems;
    const std::string sTag = "<ClCompile Include=\"";
    size_t uiAt = 0;
    while ((uiAt = sText.find(sTag, uiAt)) != std::string::npos)
    {
        uiAt += sTag.size();
        const size_t uiEnd = sText.find('"', uiAt);
        REQUIRE(uiEnd != std::string::npos);
        vItems.push_back(sText.substr(uiAt, uiEnd - uiAt));
        uiAt = uiEnd;
    }
    return vItems;
}

std::string ReadProject(const char* pcPath)
{
    std::string sText;
    REQUIRE_MESSAGE(ReadRepoText(pcPath, sText), pcPath);
    return sText;
}

bool StartsWith(const std::string& s, const std::string& sPrefix)
{
    return Lower(s).compare(0, sPrefix.size(), Lower(sPrefix)) == 0;
}

void FindProjects(const std::string& sRoot, const std::string& sRel, std::vector<std::string>& vOut)
{
    WIN32_FIND_DATAA kData;
    HANDLE hFind = FindFirstFileA((sRoot + "\\" + sRel + "\\*").c_str(), &kData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        const std::string sName = kData.cFileName;
        if (sName == "." || sName == "..")
        {
            continue;
        }
        const std::string sChild = sRel + "\\" + sName;
        if (kData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            FindProjects(sRoot, sChild, vOut);
        }
        else if (sName.size() > 8 && Lower(sName.substr(sName.size() - 8)) == ".vcxproj")
        {
            vOut.push_back(Lower(sChild));
        }
    } while (FindNextFileA(hFind, &kData));
    FindClose(hFind);
}

}

TEST_CASE("game projects together compile every game TU exactly once" * doctest::test_suite("regression"))
{
    std::set<std::string> kSeen;
    size_t uiCount = 0;
    for (const std::string& sItem : CompileItems(ReadProject("BattlefrontII.vcxproj")))
    {
        CHECK_MESSAGE(kSeen.insert(Lower(sItem)).second, sItem);
        ++uiCount;
    }
    for (const LibProject& kLib : kLibs)
    {
        for (const std::string& sItem : CompileItems(ReadProject(kLib.pcProject)))
        {
            const std::string sFull = std::string(kLib.pcDir) + "\\" + sItem;
            CHECK_MESSAGE(kSeen.insert(Lower(sFull)).second, sFull);
            ++uiCount;
        }
    }
    CHECK(uiCount == 792);
    CHECK(kSeen.size() == 792);
}

TEST_CASE("adler32 links only from unzip.cpp as the one copy in the retail exe" * doctest::test_suite("regression"))
{
    for (const std::string& sItem : CompileItems(ReadProject("src\\ZlibFL\\ZLibFL.vcxproj")))
    {
        CHECK_MESSAGE(Lower(sItem) != "adler32.c", sItem);
    }
    const std::vector<std::string> vExe = CompileItems(ReadProject("BattlefrontII.vcxproj"));
    CHECK(std::find(vExe.begin(), vExe.end(), "src\\Battlefront2\\Source\\unzip.cpp") != vExe.end());
    const std::string sUnzip = ReadProject("src\\Battlefront2\\Source\\unzip.cpp");
    CHECK(sUnzip.find("// LIBRARY: SWBF2 0x0066bbc0\n// adler32\nuLong ZEXPORT adler32(") != std::string::npos);
}

TEST_CASE("each library project compiles only its own directory and the exe project compiles none of them" * doctest::test_suite("regression"))
{
    for (const LibProject& kLib : kLibs)
    {
        const std::vector<std::string> vItems = CompileItems(ReadProject(kLib.pcProject));
        CHECK_MESSAGE(!vItems.empty(), kLib.pcProject);
        for (const std::string& sItem : vItems)
        {
            CHECK_MESSAGE(sItem.find("..") == std::string::npos, sItem);
            CHECK_MESSAGE(sItem.find(':') == std::string::npos, sItem);
        }
    }
    for (const std::string& sItem : CompileItems(ReadProject("BattlefrontII.vcxproj")))
    {
        for (const LibProject& kLib : kLibs)
        {
            CHECK_MESSAGE(!StartsWith(sItem, std::string(kLib.pcDir) + "\\"), sItem);
        }
        for (const LibProject& kDll : kDlls)
        {
            CHECK_MESSAGE(!StartsWith(sItem, std::string(kDll.pcDir) + "\\"), sItem);
        }
    }
}

TEST_CASE("every game project imports the shared props and the exe links library objects" * doctest::test_suite("regression"))
{
    std::vector<std::string> vFound;
    FindProjects(RepoRoot(), "src", vFound);
    std::sort(vFound.begin(), vFound.end());
    std::vector<std::string> vExpected;
    for (const LibProject& kLib : kLibs)
    {
        vExpected.push_back(Lower(kLib.pcProject));
    }
    for (const LibProject& kDll : kDlls)
    {
        vExpected.push_back(Lower(kDll.pcProject));
    }
    std::sort(vExpected.begin(), vExpected.end());
    CHECK(vFound == vExpected);
    for (const LibProject& kLib : kLibs)
    {
        const std::string sText = ReadProject(kLib.pcProject);
        CHECK_MESSAGE(sText.find("<Import Project=\"..\\..\\OpenSWBF2.Game.props\" />") != std::string::npos, kLib.pcProject);
        CHECK_MESSAGE(sText.find("<ConfigurationType>StaticLibrary</ConfigurationType>") != std::string::npos, kLib.pcProject);
    }
    for (const LibProject& kDll : kDlls)
    {
        const std::string sText = ReadProject(kDll.pcProject);
        CHECK_MESSAGE(sText.find("<Import Project=\"..\\..\\OpenSWBF2.Game.props\" />") != std::string::npos, kDll.pcProject);
        CHECK_MESSAGE(sText.find("<ConfigurationType>DynamicLibrary</ConfigurationType>") != std::string::npos, kDll.pcProject);
    }
    const std::string sExe = ReadProject("BattlefrontII.vcxproj");
    CHECK(sExe.find("<Import Project=\"OpenSWBF2.Game.props\" />") != std::string::npos);
    size_t uiPrev = 0;
    for (const LibProject& kLib : kLibs)
    {
        const std::string sRef = std::string("<ProjectReference Include=\"") + kLib.pcProject + "\">";
        const size_t uiAt = sExe.find(sRef);
        REQUIRE_MESSAGE(uiAt != std::string::npos, kLib.pcProject);
        CHECK_MESSAGE(uiAt > uiPrev, kLib.pcProject);
        uiPrev = uiAt;
        const size_t uiEnd = sExe.find("</ProjectReference>", uiAt);
        REQUIRE(uiEnd != std::string::npos);
        const std::string sBlock = sExe.substr(uiAt, uiEnd - uiAt);
        CHECK_MESSAGE(sBlock.find("<LinkLibraryDependencies>true</LinkLibraryDependencies>") != std::string::npos, kLib.pcProject);
        CHECK_MESSAGE(sBlock.find("<UseLibraryDependencyInputs>true</UseLibraryDependencyInputs>") != std::string::npos, kLib.pcProject);
    }
}

namespace
{
struct MasmItem
{
    std::string sInclude;
    std::string sTag;
};

std::vector<MasmItem> MasmItems(const std::string& sText)
{
    std::vector<MasmItem> vItems;
    const std::string sTag = "<MASM Include=\"";
    size_t uiAt = 0;
    while ((uiAt = sText.find(sTag, uiAt)) != std::string::npos)
    {
        const size_t uiStart = uiAt;
        uiAt += sTag.size();
        const size_t uiEnd = sText.find('"', uiAt);
        const size_t uiClose = sText.find('>', uiAt);
        REQUIRE(uiEnd != std::string::npos);
        REQUIRE(uiClose != std::string::npos);
        MasmItem kItem;
        kItem.sInclude = sText.substr(uiAt, uiEnd - uiAt);
        kItem.sTag = sText.substr(uiStart, uiClose + 1 - uiStart);
        vItems.push_back(kItem);
        uiAt = uiEnd;
    }
    return vItems;
}

void CheckMasmItems(const char* pcProject, const std::vector<std::string>& vExpected)
{
    const std::vector<MasmItem> vItems = MasmItems(ReadProject(pcProject));
    std::vector<std::string> vFound;
    for (const MasmItem& kItem : vItems)
    {
        vFound.push_back(kItem.sInclude);
        const std::string sWant = "<MASM Include=\"" + kItem.sInclude + "\" Condition=\"'$(Platform)'=='x64'\" />";
        CHECK_MESSAGE(kItem.sTag == sWant, kItem.sTag);
    }
    INFO(pcProject);
    CHECK(vFound == vExpected);
}
}

TEST_CASE("game MASM items are x64 only by item condition" * doctest::test_suite("regression"))
{
    const char* const apcAsm[] = {"src\\PebbleFL\\pc\\pcD3dxX87X64.asm", "src\\PebbleFL\\pc\\pcCrtTrigX64.asm"};
    std::vector<std::string> vGame;
    std::vector<std::string> vTests;
    for (const char* pcAsm : apcAsm)
    {
        vGame.push_back(pcAsm);
        vTests.push_back(std::string("..\\") + pcAsm);
    }
    CheckMasmItems("BattlefrontII.vcxproj", vGame);
    CheckMasmItems("tests\\OpenSWBF2Tests.vcxproj", vTests);
    for (const LibProject& kLib : kLibs)
    {
        CheckMasmItems(kLib.pcProject, std::vector<std::string>());
    }
    const char* const apcProjects[] = {"BattlefrontII.vcxproj", "tests\\OpenSWBF2Tests.vcxproj", "OpenSWBF2.Game.props"};
    for (const char* pcProject : apcProjects)
    {
        CHECK_MESSAGE(ReadProject(pcProject).find("ExcludedFromBuild") == std::string::npos, pcProject);
    }
    for (const LibProject& kLib : kLibs)
    {
        CHECK_MESSAGE(ReadProject(kLib.pcProject).find("ExcludedFromBuild") == std::string::npos, kLib.pcProject);
    }
    const std::string sProps = ReadProject("OpenSWBF2.Game.props");
    const std::string sExe = ReadProject("BattlefrontII.vcxproj");
    const std::string sTests = ReadProject("tests\\OpenSWBF2Tests.vcxproj");
    CHECK(sProps.find("<Import Project=\"$(VCTargetsPath)\\BuildCustomizations\\masm.props\" />") != std::string::npos);
    CHECK(sProps.find("<ObjectFileName>$(IntDir)%(RelativeDir)%(FileName).obj</ObjectFileName>") != std::string::npos);
    CHECK(sExe.find("<Import Project=\"$(VCTargetsPath)\\BuildCustomizations\\masm.targets\" />") != std::string::npos);
    CHECK(sTests.find("<Import Project=\"$(VCTargetsPath)\\BuildCustomizations\\masm.props\" />") != std::string::npos);
    CHECK(sTests.find("<Import Project=\"$(VCTargetsPath)\\BuildCustomizations\\masm.targets\" />") != std::string::npos);
}

TEST_CASE("project link order keeps the identity matrix first and the shader base before every shader" * doctest::test_suite("regression"))
{
    const std::vector<std::string> vExe = CompileItems(ReadProject("BattlefrontII.vcxproj"));
    REQUIRE(!vExe.empty());
    CHECK(vExe.front() == "src\\PebbleFL\\pc\\pcPblMatrix.cpp");
    const std::vector<std::string> vRed = CompileItems(ReadProject("src\\RedEngineFL\\RedEngineFL.vcxproj"));
    const std::vector<std::string>::const_iterator itBase = std::find(vRed.begin(), vRed.end(), "Graphics\\PC\\pcRedShader.cpp");
    REQUIRE(itBase != vRed.end());
    int iShaders = 0;
    for (std::vector<std::string>::const_iterator it = vRed.begin(); it != vRed.end(); ++it)
    {
        if (StartsWith(*it, "Graphics\\PC\\Shaders\\"))
        {
            ++iShaders;
            CHECK_MESSAGE(it > itBase, *it);
        }
    }
    CHECK(iShaders > 0);
}
