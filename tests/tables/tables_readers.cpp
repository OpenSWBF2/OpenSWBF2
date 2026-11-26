#include "tables/tables_readers.h"

#include <windows.h>

#include <algorithm>
#include <set>

namespace Tables
{

namespace
{

const char* const kSkipCases = "^(crash-.+|fatal-init|pool-full|pool-full-strict|dedicated)$";

const Reader kReaders[] = {
    {"cloth-headroom.txt", 0, kReadTables, "tables:cloth", 1, 0, false, 0, 0, 0, 0, 0},
    {"cloth-headroom.txt", "v143-x64", kReadTables, "tables:cloth-x64", 1, 0, false, 0, 0, 0, 0, 0},
    {"mem-peaks.txt", 0, kReadTables, "tables:mem-peaks", 4, 0, false, 0, 0, "tests\\game\\crash.cpp", 0, 0},
    {"mem-peaks.txt", "v143-x64", kReadMemPeaksX64, "tables:mem-peaks-x64", 0, 0, false, "mem-peaks.txt", "lua-bins.txt",
     "tests\\game\\crash.cpp", 0, 0},
    {"commit.txt", "v143-x64", kReadCommit, "tables:commit", 0, 0, false, 0, 0, "tests\\game\\standard.cpp", 0, 0},
    {"pool-sizes.txt", 0, kReadTables, "tables:pool-sizes", 1, 0, false, 0, 0, 0, 0, 0},
    {"pool-sizes.txt", "v143-x64", kReadTables, "tables:pool-sizes-x64", 1, 0, false, 0, 0, 0, 0, 0},
    {"smoke-all-digests.txt", 0, kReadTables, "tables:digests", 1, kSkipCases, false, 0, 0,
     "tests\\game\\standard_case.cpp", 0, 0},
    {"lua-bins.txt", 0, kReadLuaBins, "tables:lua-bins", 3, 0, false, 0, 0, 0, 0, 0},
    {"lua-bins.txt", "v143-x64", kReadLuaBins, "tables:lua-bins-x64", 3, 0, false, 0, 0, 0, 0, 0},
    {"lua-hist.txt", 0, kReadLuaHist, "tables:lua-hist", 2, 0, false, 0, 0, 0, 0, 0},
    {"lua-hist.txt", "v143-x64", kReadLuaHist, "tables:lua-hist-x64", 2, 0, false, 0, 0, 0, 0, 0},
    {"offthread.txt", 0, kReadOffThread, "tables:offthread", 1, kSkipCases, true, 0, 0, "tests\\game\\standard_case.cpp", 0,
     0},
    {"offthread.txt", "v143-x64", kReadOffThread, "tables:offthread-x64", 1, kSkipCases, true, 0, 0,
     "tests\\game\\standard_case.cpp", 0, 0},
    {"digests.txt", 0, kReadCollect, "collect:digests", 0, 0, false, 0, 0, 0, 0, 0},
    {"digests-hog.txt", 0, kReadCollect, "collect:digests-hog", 0, 0, false, 0, 0, 0, 0, 0},
    {"digest-trace-geo1c_con.txt", 0, kReadCollect, "collect:digest-trace-geo1c_con", 0, 0, false, 0, 0, 0, 0, 0},
    {"digest-trace-hot1g_con.txt", 0, kReadCollect, "collect:digest-trace-hot1g_con", 0, 0, false, 0, 0, 0, 0, 0},
    {"digest-trace-nab2g_c.txt", 0, kReadCollect, "collect:digest-trace-nab2g_c", 0, 0, false, 0, 0, 0, 0, 0},
    {"digest-trace-dag1c_ctf.txt", 0, kReadCollect, "collect:digest-trace-dag1c_ctf", 0, 0, false, 0, 0, 0, 0, 0},
    {"digest-trace-kas2c_uber.txt", 0, kReadCollect, "collect:digest-trace-kas2c_uber", 0, 0, false, 0, 0, 0, 0, 0},
    {"digest-trace-spa3c_ass.txt", 0, kReadCollect, "collect:digest-trace-spa3c_ass", 0, 0, false, 0, 0, 0, 0, 0},
    {"factory-geo1c_con.txt", 0, kReadCollect, "collect:factory-geo1c_con", 0, 0, false, 0, 0, 0, 0, 0},
    {"factory-hot1g_con.txt", 0, kReadCollect, "collect:factory-hot1g_con", 0, 0, false, 0, 0, 0, 0, 0},
    {"factory-nab2g_c.txt", 0, kReadCollect, "collect:factory-nab2g_c", 0, 0, false, 0, 0, 0, 0, 0},
    {"factory-spa3c_ass.txt", 0, kReadCollect, "collect:factory-spa3c_ass", 0, 0, false, 0, 0, 0, 0, 0},
    {"initializers.txt", 0, kReadMap, "map:initializers", 0, 0, false, 0, 0, 0, 0, 0},
    {"initializers-di.txt", 0, kReadMap, "map:initializers-di", 0, 0, false, 0, 0, 0, 0, 0},
    {"initializers-discarded.txt", 0, kReadMap, "map:initializers-discarded", 0, 0, false, 0, 0, 0, 0, 0},
    {"digests-debug.txt", 0, kReadDebugDigests, "debug:digests-debug", 0, 0, false, 0, 0, 0, 0, 0},
    {"x87-inventory.tsv", 0, kReadX87Inventory, "tables:x87-inventory", 0, 0, false, 0, 0, "tests\\x87\\x87_cases.cpp", 0, 0},
    {"CAUSES.md", 0, kReadUnit, 0, 0, 0, false, 0, 0, 0, "tests\\unit\\memory_baseline.cpp", 0},
    {"movie-goldens.txt", 0, kReadUnit, 0, 0, 0, false, 0, 0, 0, "tests\\unit\\memory_baseline.cpp", 0},
    {"movie-frames.txt", 0, kReadUnit, 0, 0, 0, false, 0, 0, 0, "tests\\unit\\movie_retail.cpp", 0},
    {"MANIFEST.md", 0, kReadVerify, 0, 0, 0, false, 0, 0, 0, 0, "v143"},
    {"CAUSES.md", "v143-x64", kReadVerify, 0, 0, 0, false, 0, 0, 0, 0, "v143-x64"},
    {"MANIFEST.md", "v143-x64", kReadVerify, 0, 0, 0, false, 0, 0, 0, 0, "v143-x64"},
};

bool Has(const char* pc)
{
    return pc != 0 && *pc != 0;
}

std::wstring Wide(const std::string& s)
{
    if (s.empty())
    {
        return std::wstring();
    }
    const int iLen = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), 0, 0);
    std::wstring w((size_t)(iLen > 0 ? iLen : 0), L'\0');
    if (iLen > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], iLen);
    }
    return w;
}

bool ContainsOrdinal(const std::vector<std::string>& k, const std::string& s)
{
    return std::find(k.begin(), k.end(), s) != k.end();
}

}

FileText Found(const std::string& sText)
{
    FileText k;
    k.bFound = true;
    k.sText = sText;
    return k;
}

FileText Absent()
{
    FileText k;
    k.bFound = false;
    return k;
}

const std::vector<Reader>& BaselineReaders()
{
    static const std::vector<Reader> k(kReaders, kReaders + sizeof(kReaders) / sizeof(kReaders[0]));
    return k;
}

std::string ReaderFolder(const Reader& kReader)
{
    return Has(kReader.pcFolder) ? std::string(kReader.pcFolder) : std::string("v143");
}

const char* KindName(ReaderKind eKind)
{
    switch (eKind)
    {
    case kReadTables:
        return "Tables";
    case kReadMemPeaksX64:
        return "MemPeaksX64";
    case kReadCommit:
        return "Commit";
    case kReadLuaBins:
        return "LuaBins";
    case kReadLuaHist:
        return "LuaHist";
    case kReadOffThread:
        return "OffThread";
    case kReadCollect:
        return "Collect";
    case kReadMap:
        return "Map";
    case kReadDebugDigests:
        return "DebugDigests";
    case kReadX87Inventory:
        return "X87Inventory";
    case kReadUnit:
        return "t1";
    case kReadVerify:
        return "verify";
    }
    return "";
}

std::vector<Reader> SelectReaders(ReaderKind eKind, const std::string& sPlatform)
{
    std::vector<Reader> kEntries;
    const std::vector<Reader>& kAll = BaselineReaders();
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (kAll[i].eKind == eKind && (!Has(kAll[i].pcFolder) || sPlatform == "x64"))
        {
            kEntries.push_back(kAll[i]);
        }
    }
    std::vector<std::string> kScoped;
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        if (Has(kEntries[i].pcFolder))
        {
            kScoped.push_back(kEntries[i].pcFile);
        }
    }
    std::vector<Reader> kOut;
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        if (Has(kEntries[i].pcFolder) || !ContainsOrdinal(kScoped, kEntries[i].pcFile))
        {
            kOut.push_back(kEntries[i]);
        }
    }
    return kOut;
}

bool KeyColumnsFor(const std::string& sFile, int& iKeyCols)
{
    const std::vector<Reader>& kAll = BaselineReaders();
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (sFile == kAll[i].pcFile && kAll[i].iKeyCols > 0)
        {
            iKeyCols = kAll[i].iKeyCols;
            return true;
        }
    }
    return false;
}

std::vector<std::string> ReadAllLines(const std::string& sText)
{
    std::vector<std::string> k;
    size_t u = 0;
    if (sText.size() >= 3 && (unsigned char)sText[0] == 0xEF && (unsigned char)sText[1] == 0xBB &&
        (unsigned char)sText[2] == 0xBF)
    {
        u = 3;
    }
    std::string sLine;
    bool bOpen = false;
    while (u < sText.size())
    {
        const char c = sText[u];
        if (c == '\r' || c == '\n')
        {
            k.push_back(sLine);
            sLine.clear();
            bOpen = false;
            if (c == '\r' && u + 1 < sText.size() && sText[u + 1] == '\n')
            {
                ++u;
            }
        }
        else
        {
            sLine += c;
            bOpen = true;
        }
        ++u;
    }
    if (bOpen)
    {
        k.push_back(sLine);
    }
    return k;
}

std::vector<std::string> SplitTabs(const std::string& sLine)
{
    std::vector<std::string> k;
    size_t uStart = 0;
    for (;;)
    {
        const size_t uTab = sLine.find('\t', uStart);
        if (uTab == std::string::npos)
        {
            k.push_back(sLine.substr(uStart));
            return k;
        }
        k.push_back(sLine.substr(uStart, uTab - uStart));
        uStart = uTab + 1;
    }
}

std::string JoinText(const std::vector<std::string>& kItems, const std::string& sSep)
{
    std::string s;
    for (size_t i = 0; i < kItems.size(); ++i)
    {
        if (i)
        {
            s += sSep;
        }
        s += kItems[i];
    }
    return s;
}

std::string TrimEndSpace(const std::string& sText)
{
    size_t uEnd = sText.size();
    while (uEnd > 0 && (sText[uEnd - 1] == ' ' || (sText[uEnd - 1] >= '\t' && sText[uEnd - 1] <= '\r')))
    {
        --uEnd;
    }
    return sText.substr(0, uEnd);
}

std::string LowerAscii(const std::string& sText)
{
    std::string s = sText;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] >= 'A' && s[i] <= 'Z')
        {
            s[i] = (char)(s[i] - 'A' + 'a');
        }
    }
    return s;
}

bool EqualNoCase(const std::string& sA, const std::string& sB)
{
    return LowerAscii(sA) == LowerAscii(sB);
}

size_t FindNoCase(const std::string& sText, const std::string& sNeedle)
{
    return LowerAscii(sText).find(LowerAscii(sNeedle));
}

bool StartsWith(const std::string& sText, const std::string& sPrefix)
{
    return sText.compare(0, sPrefix.size(), sPrefix) == 0;
}

bool AllDigits(const std::string& sText)
{
    if (sText.empty())
    {
        return false;
    }
    for (size_t i = 0; i < sText.size(); ++i)
    {
        if (sText[i] < '0' || sText[i] > '9')
        {
            return false;
        }
    }
    return true;
}

std::string JoinPath(const std::string& sDir, const std::string& sName)
{
    if (sDir.empty())
    {
        return sName;
    }
    if (sDir[sDir.size() - 1] == '\\' || sDir[sDir.size() - 1] == '/')
    {
        return sDir + sName;
    }
    return sDir + "\\" + sName;
}

int CultureCompare(const std::string& sA, const std::string& sB, bool bIgnoreCase)
{
    const std::wstring wA = Wide(sA);
    const std::wstring wB = Wide(sB);
    const int iResult = CompareStringEx(L"en-US", bIgnoreCase ? NORM_IGNORECASE : 0, wA.c_str(), (int)wA.size(),
                                        wB.c_str(), (int)wB.size(), 0, 0, 0);
    if (iResult == CSTR_LESS_THAN)
    {
        return -1;
    }
    if (iResult == CSTR_GREATER_THAN)
    {
        return 1;
    }
    if (iResult == CSTR_EQUAL)
    {
        return 0;
    }
    return sA < sB ? -1 : (sB < sA ? 1 : 0);
}

void CultureSort(std::vector<std::string>& kItems, bool bIgnoreCase)
{
    std::stable_sort(kItems.begin(), kItems.end(), [bIgnoreCase](const std::string& a, const std::string& b) {
        return CultureCompare(a, b, bIgnoreCase) < 0;
    });
}

std::vector<std::string> BaselineReadersCheck(const std::vector<Reader>& kReaders,
                                              const std::map<std::string, std::vector<std::string> >& kFolderFiles,
                                              const RepoTextFn& fnRepoText)
{
    std::vector<std::string> kHits;
    std::vector<std::string> kFolders;
    kFolders.push_back("v143");
    kFolders.push_back("v143-x64");
    std::vector<std::string> kFiles;
    for (size_t i = 0; i < kFolders.size(); ++i)
    {
        std::map<std::string, std::vector<std::string> >::const_iterator it = kFolderFiles.find(kFolders[i]);
        if (it == kFolderFiles.end())
        {
            kHits.push_back("tests\\baseline\\" + kFolders[i] + " not found");
            continue;
        }
        for (size_t j = 0; j < it->second.size(); ++j)
        {
            kFiles.push_back(kFolders[i] + "\\" + it->second[j]);
        }
    }
    CultureSort(kFiles, true);
    std::set<std::string> kMapped;
    std::vector<std::string> kMappedList;
    for (size_t i = 0; i < kReaders.size(); ++i)
    {
        const Reader& e = kReaders[i];
        if (!Has(e.pcFile))
        {
            kHits.push_back("reader map entry without File and Reader: System.Collections.Hashtable");
            continue;
        }
        const std::string sFolder = ReaderFolder(e);
        if (!ContainsOrdinal(kFolders, sFolder))
        {
            kHits.push_back(std::string(e.pcFile) + " names folder " + sFolder + ", which is not a baseline folder");
            continue;
        }
        const std::string sKey = sFolder + "\\" + e.pcFile;
        if (!kMapped.insert(sKey).second)
        {
            kHits.push_back(sKey + " is in the reader map twice");
        }
        else
        {
            kMappedList.push_back(sKey);
        }
        if (e.eKind == kReadUnit)
        {
            const FileText kSrc = fnRepoText(Has(e.pcUnit) ? e.pcUnit : "");
            if (!kSrc.bFound || kSrc.sText.find(std::string(e.pcFile) + "\"") == std::string::npos)
            {
                kHits.push_back(sKey + " reader " + (Has(e.pcUnit) ? e.pcUnit : "") + " does not name it");
            }
        }
        if (Has(e.pcCpp))
        {
            const FileText kSrc = fnRepoText(e.pcCpp);
            if (!kSrc.bFound || kSrc.sText.find(sFolder + "\\\\" + e.pcFile + "\"") == std::string::npos)
            {
                kHits.push_back(sKey + " C++ reader " + e.pcCpp + " does not name it");
            }
        }
    }
    for (size_t i = 0; i < kFiles.size(); ++i)
    {
        if (!kMapped.count(kFiles[i]))
        {
            kHits.push_back(kFiles[i] + " has no reader");
        }
    }
    CultureSort(kMappedList, true);
    for (size_t i = 0; i < kMappedList.size(); ++i)
    {
        if (!ContainsOrdinal(kFiles, kMappedList[i]))
        {
            kHits.push_back(kMappedList[i] + " is in the reader map but not in tests\\baseline");
        }
    }
    return kHits;
}

std::vector<std::string> AllCheckpointsCheck(const RepoTextFn& fnRepoText)
{
    static const char* const sapcMissions[] = {"-digesttrace 1500\"", "HasDigestTrace(sLog, 100)",
                                               "HasDigestTrace(sLog, 1500)", "CheckDigestRow(", 0};
    static const char* const sapcStandard[] = {"\" -digesttrace 1500\"", "HasDigestTrace(sLog, 100)",
                                               "HasDigestTrace(sLog, 1500)", "CheckDigestRow(", 0};
    static const char* const sapcTwice[] = {"StandardArgs(", "CheckStandardRun(", 0};
    struct Need
    {
        const char* pcRel;
        const char* const* ppcNeedles;
    };
    static const Need skNeed[] = {{"tests\\game\\missions.cpp", sapcMissions},
                                  {"tests\\game\\standard_case.cpp", sapcStandard},
                                  {"tests\\game\\twice.cpp", sapcTwice}};
    std::vector<std::string> kHits;
    for (size_t i = 0; i < sizeof(skNeed) / sizeof(skNeed[0]); ++i)
    {
        const FileText kText = fnRepoText(skNeed[i].pcRel);
        if (!kText.bFound)
        {
            kHits.push_back(std::string(skNeed[i].pcRel) + " not found");
            continue;
        }
        for (const char* const* pp = skNeed[i].ppcNeedles; *pp; ++pp)
        {
            if (kText.sText.find(*pp) == std::string::npos)
            {
                kHits.push_back(std::string(skNeed[i].pcRel) + " lacks " + *pp);
            }
        }
    }
    return kHits;
}

}
