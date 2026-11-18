#include "third_party/doctest/doctest.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "testutil.h"

namespace
{

std::string LuacConfiguration()
{
    char acExe[MAX_PATH];
    DWORD uLen = GetModuleFileNameA(0, acExe, sizeof(acExe));
    if (uLen == 0 || uLen >= sizeof(acExe))
    {
        return std::string();
    }
    std::string sDir(acExe, uLen);
    size_t uSlash = sDir.find_last_of("\\/");
    if (uSlash == std::string::npos)
    {
        return std::string();
    }
    sDir.erase(uSlash);
    uSlash = sDir.find_last_of("\\/");
    return uSlash == std::string::npos ? std::string() : sDir.substr(uSlash + 1);
}

std::string LuacExe(const char* pcPlatform)
{
    return RepoRoot() + "\\bin\\" + pcPlatform + "\\" + LuacConfiguration() + "\\LuaCompiler.exe";
}

void FindLuacPair(std::string& sX86, std::string& sX64)
{
    sX86 = LuacExe("Win32");
    sX64 = LuacExe("x64");
    const std::string* apExe[2] = { &sX86, &sX64 };
    for (int i = 0; i < 2; ++i)
    {
        if (GetFileAttributesA(apExe[i]->c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            std::string sReason = "luac-compare: " + *apExe[i] + " not built";
            SkipCase(sReason.c_str());
        }
    }
}

DWORD RunLuac(const std::string& sExe, const std::string& sArgs)
{
    std::string sCmd = "\"" + sExe + "\" " + sArgs;
    std::vector<char> vCmd(sCmd.begin(), sCmd.end());
    vCmd.push_back(0);
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));
    std::string sRoot = RepoRoot();
    fflush(stdout);
    if (!CreateProcessA(0, &vCmd[0], 0, 0, TRUE, CREATE_NO_WINDOW, 0, sRoot.c_str(), &si, &pi))
    {
        return 0xFFFFFFFF;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD uCode = 0xFFFFFFFF;
    GetExitCodeProcess(pi.hProcess, &uCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    WatchdogArm(sArgs.c_str());
    return uCode;
}

struct FileCompare
{
    bool bRead;
    bool bSame;
    unsigned long long uFirst;
    unsigned long long uSize86;
    unsigned long long uSize64;
};

FileCompare CompareFiles(const std::string& sX86, const std::string& sX64)
{
    FileCompare k = { false, false, 0, 0, 0 };
    FILE* pX86 = fopen(sX86.c_str(), "rb");
    FILE* pX64 = fopen(sX64.c_str(), "rb");
    if (pX86 && pX64)
    {
        static char acX86[65536];
        static char acX64[65536];
        bool bDiffer = false;
        for (;;)
        {
            size_t uX86 = fread(acX86, 1, sizeof(acX86), pX86);
            size_t uX64 = fread(acX64, 1, sizeof(acX64), pX64);
            size_t uBoth = uX86 < uX64 ? uX86 : uX64;
            for (size_t i = 0; !bDiffer && i < uBoth; ++i)
            {
                if (acX86[i] != acX64[i])
                {
                    k.uFirst = k.uSize86 + i;
                    bDiffer = true;
                }
            }
            if (!bDiffer && uX86 != uX64)
            {
                k.uFirst = k.uSize86 + uBoth;
                bDiffer = true;
            }
            k.uSize86 += uX86;
            k.uSize64 += uX64;
            if (uX86 == 0 && uX64 == 0)
            {
                break;
            }
        }
        k.bRead = ferror(pX86) == 0 && ferror(pX64) == 0;
        k.bSame = !bDiffer;
    }
    if (pX86)
    {
        fclose(pX86);
    }
    if (pX64)
    {
        fclose(pX64);
    }
    return k;
}

std::vector<std::string> IndexedSources()
{
    std::vector<std::string> vSources;
    std::string sText;
    if (!ReadRepoText("data\\scripts\\index.json", sText))
    {
        return vSources;
    }
    const std::string sKey = "\"source\":";
    size_t uAt = 0;
    while ((uAt = sText.find(sKey, uAt)) != std::string::npos)
    {
        size_t uOpen = sText.find('"', uAt + sKey.size());
        size_t uClose = uOpen == std::string::npos ? std::string::npos : sText.find('"', uOpen + 1);
        if (uClose == std::string::npos)
        {
            break;
        }
        if (uClose > uOpen + 1)
        {
            vSources.push_back(sText.substr(uOpen + 1, uClose - uOpen - 1));
        }
        uAt = uClose + 1;
    }
    return vSources;
}

void RemoveTree(const std::string& sDir)
{
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((sDir + "\\*").c_str(), &kFind);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::string sName = kFind.cFileName;
            if (sName == "." || sName == "..")
            {
                continue;
            }
            std::string sPath = sDir + "\\" + sName;
            const DWORD uAttr = kFind.dwFileAttributes;
            if ((uAttr & FILE_ATTRIBUTE_DIRECTORY) && !(uAttr & FILE_ATTRIBUTE_REPARSE_POINT))
            {
                RemoveTree(sPath);
            }
            else
            {
                DeleteFileA(sPath.c_str());
            }
        } while (FindNextFileA(hFind, &kFind));
        FindClose(hFind);
    }
    RemoveDirectoryA(sDir.c_str());
}

class LuacScratch
{
public:
    explicit LuacScratch(const char* pcName)
    {
        char acTemp[MAX_PATH];
        DWORD uLen = GetTempPathA(MAX_PATH, acTemp);
        mRoot = std::string(acTemp, uLen < MAX_PATH ? uLen : 0) + "owb2_luac_" + pcName + "_" +
                std::to_string(GetCurrentProcessId());
        RemoveTree(mRoot);
        CreateDirectoryA(mRoot.c_str(), 0);
        mX86 = mRoot + "\\x86";
        mX64 = mRoot + "\\x64";
        CreateDirectoryA(mX86.c_str(), 0);
        CreateDirectoryA(mX64.c_str(), 0);
    }

    ~LuacScratch() { RemoveTree(mRoot); }

    std::string mRoot;
    std::string mX86;
    std::string mX64;

private:
    LuacScratch(const LuacScratch&);
    LuacScratch& operator=(const LuacScratch&);
};

}

TEST_CASE("luac x86 and x64 write the same plain bytecode" * doctest::test_suite("regression"))
{
    std::string sX86;
    std::string sX64;
    FindLuacPair(sX86, sX64);
    std::vector<std::string> vSources = IndexedSources();
    REQUIRE(!vSources.empty());
    LuacScratch kDir("plain");
    size_t uSame = 0;
    for (size_t i = 0; i < vSources.size(); ++i)
    {
        const std::string& sSource = vSources[i];
        std::string sName = std::to_string(i) + ".luac";
        std::string sOut86 = kDir.mX86 + "\\" + sName;
        std::string sOut64 = kDir.mX64 + "\\" + sName;
        DWORD uCode86 = RunLuac(sX86, "-s -o \"" + sOut86 + "\" \"" + sSource + "\"");
        DWORD uCode64 = RunLuac(sX64, "-s -o \"" + sOut64 + "\" \"" + sSource + "\"");
        CHECK_MESSAGE(uCode86 == 0, "x86 luac failed on " << sSource);
        CHECK_MESSAGE(uCode64 == 0, "x64 luac failed on " << sSource);
        if (uCode86 != 0 || uCode64 != 0)
        {
            continue;
        }
        FileCompare k = CompareFiles(sOut86, sOut64);
        CHECK_MESSAGE(k.bRead, "cannot read the bytecode of " << sSource);
        CHECK_MESSAGE(k.uSize86 != 0, "empty bytecode for " << sSource);
        CHECK_MESSAGE(k.bSame, sSource << " differs at byte " << k.uFirst << " (x86 " << k.uSize86
                                       << " bytes, x64 " << k.uSize64 << " bytes)");
        if (k.bRead && k.bSame && k.uSize86 != 0)
        {
            ++uSame;
        }
    }
    printf("luac-compare: %u of %u scripts identical\n", (unsigned int)uSame,
           (unsigned int)vSources.size());
    CHECK(uSame == vSources.size());
}

TEST_CASE("luac x86 and x64 write the same lvls" * doctest::test_suite("regression"))
{
    std::string sX86;
    std::string sX64;
    FindLuacPair(sX86, sX64);
    REQUIRE_GAMEDATA();
    std::string sGameData = FindGameDataRoot();
    while (!sGameData.empty() && strchr("\\/", sGameData[sGameData.size() - 1]))
    {
        sGameData.erase(sGameData.size() - 1);
    }
    LuacScratch kDir("lvl");
    std::string sArgs =
        "lvl --index \"data\\scripts\\index.json\" --gamedata \"" + sGameData + "\" --out-dir \"";
    REQUIRE(RunLuac(sX86, sArgs + kDir.mX86 + "\"") == 0);
    REQUIRE(RunLuac(sX64, sArgs + kDir.mX64 + "\"") == 0);
    const char* apcLvls[] = { "shell", "common", "ingame", "mission" };
    for (int i = 0; i < 4; ++i)
    {
        std::string sName = std::string(apcLvls[i]) + ".lvl";
        FileCompare k = CompareFiles(kDir.mX86 + "\\" + sName, kDir.mX64 + "\\" + sName);
        CHECK_MESSAGE(k.bRead, "cannot read both copies of " << sName);
        CHECK_MESSAGE(k.uSize86 != 0, "x86 wrote an empty " << sName);
        CHECK_MESSAGE(k.bSame, sName << " differs at byte " << k.uFirst << " (x86 " << k.uSize86
                                     << " bytes, x64 " << k.uSize64 << " bytes)");
    }
}
