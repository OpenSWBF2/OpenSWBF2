#include <windows.h>

#include <algorithm>

#include "gate/gate_keys.h"
#include "gate/gate_readlist.h"

namespace Gate
{

void ReadListNote(const std::string& sPath)
{
    wchar_t acList[MAX_PATH * 4];
    const DWORD uLen = GetEnvironmentVariableW(L"OPENSWBF2_READS", acList, MAX_PATH * 4);
    if (uLen == 0 || uLen >= MAX_PATH * 4)
    {
        return;
    }
    AppendTextFile(WideToUtf8(std::wstring(acList, uLen)), FullPath(sPath) + "\r\n");
}

static std::string RepoRel(const std::string& sPath, const std::string& sRoot)
{
    std::string sOut = sPath;
    std::replace(sOut.begin(), sOut.end(), '/', '\\');
    std::string sRootSlash = sRoot;
    if (!sRootSlash.empty() && sRootSlash[sRootSlash.size() - 1] != '\\')
    {
        sRootSlash += "\\";
    }
    if (StartsWithNoCase(sOut, sRootSlash))
    {
        sOut = sOut.substr(sRootSlash.size());
    }
    return LowerText(sOut);
}

std::vector<std::string> ReadCheck(const ReadCheckInput& kIn)
{
    std::vector<std::string> kPaths = SplitLines(kIn.sReadsText);
    const std::string sExclude = "--exclude=";
    for (size_t i = 0; i < kIn.kArgs.size(); ++i)
    {
        if (kIn.kArgs[i].compare(0, sExclude.size(), sExclude) == 0)
        {
            kPaths.push_back(kIn.kArgs[i].substr(sExclude.size()));
        }
    }
    std::vector<std::string> kFails;
    std::set<std::string> kSeen;
    for (size_t i = 0; i < kPaths.size(); ++i)
    {
        const std::string sLine = TrimText(kPaths[i]);
        if (sLine.empty())
        {
            continue;
        }
        const std::string sRel = RepoRel(sLine, kIn.sRoot);
        if (!kIn.pTree || kIn.pTree->find(sRel) == kIn.pTree->end())
        {
            continue;
        }
        if (InputMatches(sRel, kIn.kReads) || kIn.kClosure.count(sRel) || !kSeen.insert(sRel).second)
        {
            continue;
        }
        kFails.push_back("harness: " + kIn.sStep + " read " + sRel + ", which is not in its Reads");
    }
    return kFails;
}

}
