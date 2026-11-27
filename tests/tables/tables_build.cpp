#include "tables/tables_build.h"

#include <windows.h>

#include <algorithm>
#include <regex>
#include <set>

#include "harness/compare.h"

namespace Tables
{

namespace
{

bool ParseHex(const std::string& s, unsigned long long& ull)
{
    if (s.empty() || s.size() > 16)
    {
        return false;
    }
    ull = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        const char c = s[i];
        unsigned int u = 0;
        if (c >= '0' && c <= '9')
        {
            u = (unsigned int)(c - '0');
        }
        else if (c >= 'a' && c <= 'f')
        {
            u = (unsigned int)(c - 'a' + 10);
        }
        else if (c >= 'A' && c <= 'F')
        {
            u = (unsigned int)(c - 'A' + 10);
        }
        else
        {
            return false;
        }
        ull = (ull << 4) | u;
    }
    return true;
}

bool ParseDecimal(const std::string& s, unsigned long long& ull)
{
    if (!AllDigits(s))
    {
        return false;
    }
    ull = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        const unsigned long long uDigit = (unsigned long long)(s[i] - '0');
        if (ull > (~0ULL - uDigit) / 10ULL)
        {
            return false;
        }
        ull = ull * 10ULL + uDigit;
    }
    return true;
}

std::string UpperHex(unsigned long long ull)
{
    static const char* const kDigits = "0123456789ABCDEF";
    std::string s;
    do
    {
        s.insert(s.begin(), kDigits[ull & 15]);
        ull >>= 4;
    } while (ull);
    return s;
}

std::string UpperAscii(const std::string& sText)
{
    std::string s = sText;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] >= 'a' && s[i] <= 'z')
        {
            s[i] = (char)(s[i] - 'a' + 'A');
        }
    }
    return s;
}

bool IsSpace(char c)
{
    return c == ' ' || (c >= '\t' && c <= '\r');
}

std::string TrimSpace(const std::string& s)
{
    size_t uStart = 0;
    while (uStart < s.size() && IsSpace(s[uStart]))
    {
        ++uStart;
    }
    size_t uEnd = s.size();
    while (uEnd > uStart && IsSpace(s[uEnd - 1]))
    {
        --uEnd;
    }
    return s.substr(uStart, uEnd - uStart);
}

std::string ReplaceNoCase(const std::string& sText, const std::string& sFrom, const std::string& sTo)
{
    if (sFrom.empty())
    {
        return sText;
    }
    const std::string sLower = LowerAscii(sText);
    const std::string sNeedle = LowerAscii(sFrom);
    std::string sOut;
    size_t uAt = 0;
    for (;;)
    {
        const size_t uHit = sLower.find(sNeedle, uAt);
        if (uHit == std::string::npos)
        {
            sOut += sText.substr(uAt);
            return sOut;
        }
        sOut += sText.substr(uAt, uHit - uAt) + sTo;
        uAt = uHit + sFrom.size();
    }
}

std::string AsciiText(const std::string& s)
{
    bool bWide = false;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if ((unsigned char)s[i] >= 0x80)
        {
            bWide = true;
            break;
        }
    }
    if (!bWide)
    {
        return s;
    }
    const int iLen = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), 0, 0);
    std::wstring w((size_t)(iLen > 0 ? iLen : 0), L'\0');
    if (iLen > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], iLen);
    }
    std::string sOut;
    for (size_t i = 0; i < w.size(); ++i)
    {
        sOut += w[i] < 0x80 ? (char)w[i] : '?';
    }
    return sOut;
}

std::string Dashes(int iCount)
{
    std::string s;
    for (int i = 0; i < iCount; ++i)
    {
        s += "\t-";
    }
    return s;
}

BuildResult Ok()
{
    BuildResult k;
    k.bOk = true;
    return k;
}

BuildResult& Fail(BuildResult& k, const std::string& sWhy)
{
    k.bOk = false;
    k.sWhy = sWhy;
    return k;
}

void AddTable(BuildResult& k, const std::string& sName, const std::vector<std::string>& kLines)
{
    BuiltTable t;
    t.sName = sName;
    t.kLines = kLines;
    k.kTables.push_back(t);
}

FileText RunLogOf(const std::vector<CaseLog>& kCases, const std::string& sName)
{
    const CaseLog* p = FindCase(kCases, sName);
    return p ? p->kRunLog : Absent();
}

bool IsHex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int RoundHalfEven(unsigned long long ullNum, unsigned long long ullDen)
{
    unsigned long long q = ullNum / ullDen;
    const unsigned long long r2 = (ullNum % ullDen) * 2ULL;
    if (r2 > ullDen || (r2 == ullDen && (q & 1ULL)))
    {
        ++q;
    }
    return (int)q;
}

bool LessMapRow(const MapRow& a, const MapRow& b)
{
    if (a.ullOffset != b.ullOffset)
    {
        return a.ullOffset < b.ullOffset;
    }
    return a.uLine < b.uLine;
}

bool LessDiRow(const MapRow& a, const MapRow& b)
{
    if (a.iSec != b.iSec)
    {
        return a.iSec < b.iSec;
    }
    return LessMapRow(a, b);
}

}

std::string ScrubText(const std::string& sText, const Scrub& kScrub)
{
    std::vector<std::pair<std::string, std::string> > kSubs;
    kSubs.push_back(std::make_pair(kScrub.sRoot, std::string(".")));
    if (!kScrub.sProfile.empty())
    {
        std::string sProfile = kScrub.sProfile;
        while (!sProfile.empty() && sProfile[sProfile.size() - 1] == '\\')
        {
            sProfile.erase(sProfile.size() - 1);
        }
        kSubs.push_back(std::make_pair(sProfile, std::string("%USERPROFILE%")));
    }
    std::stable_sort(kSubs.begin(), kSubs.end(),
                     [](const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b) {
                         return a.first.size() > b.first.size();
                     });
    std::string s = sText;
    for (size_t i = 0; i < kSubs.size(); ++i)
    {
        std::string sSlash = kSubs[i].first;
        std::replace(sSlash.begin(), sSlash.end(), '\\', '/');
        s = ReplaceNoCase(s, kSubs[i].first, kSubs[i].second);
        s = ReplaceNoCase(s, sSlash, kSubs[i].second);
    }
    return s;
}

bool BaselineText(const std::vector<std::string>& kLines, const Scrub& kScrub, const std::string& sLeaf,
                  std::string& sText, std::string& sWhy)
{
    std::string s;
    if (!kLines.empty())
    {
        s = JoinText(kLines, "\r\n") + "\r\n";
    }
    s = ScrubText(s, kScrub);
    if (!kScrub.sUser.empty() && FindNoCase(s, kScrub.sUser) != std::string::npos)
    {
        sWhy = "user name left in " + sLeaf + " after scrubbing";
        return false;
    }
    sText = AsciiText(s);
    return true;
}

const CaseLog* FindCase(const std::vector<CaseLog>& kCases, const std::string& sName)
{
    for (size_t i = 0; i < kCases.size(); ++i)
    {
        if (EqualNoCase(kCases[i].sName, sName))
        {
            return &kCases[i];
        }
    }
    return 0;
}

std::vector<std::string> LogLines(const FileText& kRunLog, const std::string& sMarker, bool bBeforeSoak)
{
    std::vector<std::string> kOut;
    if (!kRunLog.bFound)
    {
        return kOut;
    }
    const std::vector<std::string> kLines = ReadAllLines(kRunLog.sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (bBeforeSoak && kLines[i].find("Soak turns=") != std::string::npos)
        {
            break;
        }
        const size_t uAt = kLines[i].find(sMarker);
        if (uAt != std::string::npos)
        {
            kOut.push_back(TrimEndSpace(kLines[i].substr(uAt)));
        }
    }
    return kOut;
}

std::vector<std::string> FactoryLines(const FileText& kRunLog)
{
    std::vector<std::string> kOut;
    if (!kRunLog.bFound)
    {
        return kOut;
    }
    const std::vector<std::string> kLines = ReadAllLines(kRunLog.sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string& s = kLines[i];
        size_t uAt = s.find("Factory ");
        while (uAt != std::string::npos && (uAt < 2 || s.compare(uAt - 2, 2, ": ") != 0))
        {
            uAt = s.find("Factory ", uAt + 1);
        }
        if (uAt != std::string::npos)
        {
            kOut.push_back(TrimEndSpace(s.substr(uAt)));
        }
    }
    return kOut;
}

std::vector<std::string> BuildIdLines(const FileText& kRunLog)
{
    std::vector<std::string> kOut;
    if (!kRunLog.bFound)
    {
        return kOut;
    }
    const std::vector<std::string> kLines = ReadAllLines(kRunLog.sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string& s = kLines[i];
        const size_t p = s.find("log start ");
        if (p == std::string::npos)
        {
            continue;
        }
        size_t uBest = std::string::npos;
        size_t q = s.find(" build ", p + 10);
        while (q != std::string::npos)
        {
            if (q + 7 < s.size() && IsHex(s[q + 7]))
            {
                uBest = q;
            }
            q = s.find(" build ", q + 1);
        }
        if (uBest == std::string::npos)
        {
            continue;
        }
        size_t uEnd = uBest + 7;
        while (uEnd < s.size() && IsHex(s[uEnd]))
        {
            ++uEnd;
        }
        kOut.push_back(TrimEndSpace(s.substr(p, uEnd - p)));
    }
    return kOut;
}

std::string DigestKey(const FileText& kRunLog)
{
    std::string sLast;
    if (!kRunLog.bFound)
    {
        return sLast;
    }
    const std::vector<std::string> kLines = ReadAllLines(kRunLog.sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const size_t uAt = FindNoCase(kLines[i], "Sim digest ");
        if (uAt != std::string::npos)
        {
            sLast = kLines[i].substr(uAt);
        }
    }
    return sLast;
}

std::vector<std::string> MissionNames(const std::string& sText)
{
    std::vector<std::string> kOut;
    const std::vector<std::string> kLines = ReadAllLines(sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string s = TrimSpace(kLines[i]);
        size_t uEnd = 0;
        while (uEnd < s.size() && !IsSpace(s[uEnd]))
        {
            ++uEnd;
        }
        if (uEnd > 0)
        {
            kOut.push_back(s.substr(0, uEnd));
        }
    }
    return kOut;
}

std::vector<CaseLog> SortedCases(const std::vector<CaseLog>& kCases)
{
    std::vector<CaseLog> k = kCases;
    std::stable_sort(k.begin(), k.end(),
                     [](const CaseLog& a, const CaseLog& b) { return CultureCompare(a.sName, b.sName, true) < 0; });
    return k;
}

BuildResult BuildSmokeAllTables(const std::vector<CaseLog>& kCases, const std::string& sMissionsText,
                                const std::string& sCommit)
{
    BuildResult k = Ok();
    const std::vector<CaseLog> kSorted = SortedCases(kCases);
    std::vector<std::string> kRows;
    for (size_t i = 0; i < kSorted.size(); ++i)
    {
        std::string sDigest = DigestKey(kSorted[i].kRunLog);
        std::string sScore =
            kSorted[i].kFinalScore.bFound ? FinalScoreKey(kSorted[i].kFinalScore.sText) : std::string();
        if (sDigest.empty())
        {
            sDigest = "none";
        }
        if (sScore.empty())
        {
            sScore = "none";
        }
        kRows.push_back(kSorted[i].sName + "\t" + sDigest + "\t" + sScore);
    }
    if (kRows.empty())
    {
        return Fail(k, "no case dirs under -SmokeAllDir\\smoke");
    }
    AddTable(k, "smoke-all-digests.txt", kRows);
    const std::vector<std::string> kMissions = MissionNames(sMissionsText);
    const std::string sCommitLine = "# commit " + sCommit;
    std::vector<std::string> kPool;
    std::vector<std::string> kPeak;
    std::vector<std::string> kCloth;
    kPool.push_back(sCommitLine);
    kPool.push_back("mission\tentry\tlabel\trequested\tapplied\tpool");
    kPeak.push_back(sCommitLine);
    kPeak.push_back("mission\tsource\tkind\tname\tcap_or_count\tsize\tpeak\tused\tallocs_or_grown\tfailed");
    kCloth.push_back(sCommitLine);
    kCloth.push_back("mission\tfree\tenabled\tlow\thigh");
    const std::regex kScriptRx("^Script memory entry=(\\S+) label=(.*) requested=(-?\\d+) applied=(\\d+) pool=(.*)$");
    const std::regex kHeapRx("^Memory report heap=(.*) cap=(\\d+) peak=(\\d+) used=(\\d+) allocs=(\\d+) failed=(\\d+)$");
    const std::regex kPoolRx("^Memory report pool=(.*) count=(\\d+) size=(\\d+) peak=(\\d+) grown=(\\d+)$");
    const std::regex kClothRx("^Cloth free=(-?\\d+) enabled=([01]) low=(-?\\d+) high=(-?\\d+)$");
    for (size_t i = 0; i < kMissions.size(); ++i)
    {
        const std::string& m = kMissions[i];
        const FileText kLog = RunLogOf(kCases, m);
        const std::vector<std::string> kSm = LogLines(kLog, "Script memory ", false);
        if (kSm.empty())
        {
            kPool.push_back(m + Dashes(5));
        }
        for (size_t j = 0; j < kSm.size(); ++j)
        {
            std::smatch f;
            if (!std::regex_match(kSm[j], f, kScriptRx))
            {
                return Fail(k, "bad SCRIPTMEM line in " + m + ": " + kSm[j]);
            }
            kPool.push_back(m + "\t" + f[1].str() + "\t" + f[2].str() + "\t" + f[3].str() + "\t" + f[4].str() + "\t" +
                            f[5].str());
        }
        const std::vector<std::string> kMr = LogLines(kLog, "Memory report ", true);
        std::string sSrc;
        bool bSrc = false;
        std::vector<std::string> kOrder;
        std::map<std::string, std::string> kLatest;
        for (size_t j = 0; j < kMr.size(); ++j)
        {
            const std::string& l = kMr[j];
            if (StartsWith(l, "Memory report source="))
            {
                sSrc = l.substr(21);
                bSrc = !sSrc.empty();
                continue;
            }
            if (StartsWith(l, "Memory report offthread ") || StartsWith(l, "Memory report luaspill ") ||
                StartsWith(l, "Memory report luahist "))
            {
                continue;
            }
            if (!bSrc)
            {
                return Fail(k, "MEMREPORT line before source= in " + m + ": " + l);
            }
            std::smatch h;
            std::smatch p;
            std::string sKey;
            std::string sRow;
            if (std::regex_match(l, h, kHeapRx))
            {
                sKey = "heap\t" + h[1].str();
                sRow = m + "\t" + sSrc + "\t" + sKey + "\t" + h[2].str() + "\t-\t" + h[3].str() + "\t" + h[4].str() +
                       "\t" + h[5].str() + "\t" + h[6].str();
            }
            else if (std::regex_match(l, p, kPoolRx))
            {
                sKey = "pool\t" + p[1].str();
                sRow = m + "\t" + sSrc + "\t" + sKey + "\t" + p[2].str() + "\t" + p[3].str() + "\t" + p[4].str() +
                       "\t-\t" + p[5].str() + "\t-";
            }
            else
            {
                return Fail(k, "bad MEMREPORT line in " + m + ": " + l);
            }
            if (!kLatest.count(sKey))
            {
                kOrder.push_back(sKey);
            }
            kLatest[sKey] = sRow;
        }
        if (kOrder.empty())
        {
            kPeak.push_back(m + Dashes(9));
        }
        for (size_t j = 0; j < kOrder.size(); ++j)
        {
            kPeak.push_back(kLatest[kOrder[j]]);
        }
        const std::vector<std::string> kCl = LogLines(kLog, "Cloth free=", false);
        std::smatch c;
        if (!kCl.empty() && std::regex_match(kCl[0], c, kClothRx))
        {
            kCloth.push_back(m + "\t" + c[1].str() + "\t" + c[2].str() + "\t" + c[3].str() + "\t" + c[4].str());
        }
        else if (!kCl.empty())
        {
            return Fail(k, "bad CLOTH line in " + m + ": " + kCl[0]);
        }
        else
        {
            kCloth.push_back(m + Dashes(4));
        }
    }
    AddTable(k, "pool-sizes.txt", kPool);
    AddTable(k, "mem-peaks.txt", kPeak);
    AddTable(k, "cloth-headroom.txt", kCloth);
    return k;
}

BuildResult BuildLuaTables(const std::vector<CaseLog>& kCases, const std::string& sMissionsText,
                           const std::string& sCommit)
{
    BuildResult k = Ok();
    const std::vector<std::string> kMissions = MissionNames(sMissionsText);
    std::vector<std::string> kMissing;
    for (size_t i = 0; i < kMissions.size(); ++i)
    {
        if (!RunLogOf(kCases, kMissions[i]).bFound)
        {
            kMissing.push_back(kMissions[i]);
        }
    }
    if (!kMissing.empty())
    {
        std::vector<std::string> kFirst(kMissing.begin(), kMissing.begin() + (kMissing.size() < 5 ? kMissing.size() : 5));
        return Fail(k, "no run.log under -LuaBinsDir for " + std::to_string(kMissing.size()) +
                           " missions: " + JoinText(kFirst, ", "));
    }
    std::vector<std::string> kRows;
    kRows.push_back("# commit " + sCommit);
    kRows.push_back("mission\tsource\tbin\tcount\tsize\tpeak\tgrown\tspill");
    std::vector<std::string> kHist;
    kHist.push_back(kRows[0]);
    kHist.push_back("mission\tbin\tpeak\tcalls");
    const std::regex kPoolRx("^Memory report pool=Lua(\\d+) count=(\\d+) size=(\\d+) peak=(\\d+) grown=(\\d+)$");
    const std::regex kSpillRx("^Memory report luaspill pool=Lua(\\d+) count=(\\d+)$");
    const std::regex kHistRx("^Memory report luahist bin=(\\d+|over) peak=(\\d+) calls=(\\d+)$");
    struct Block
    {
        std::string sSource;
        std::vector<std::vector<std::string> > kPools;
        std::map<std::string, std::string> kSpill;
        std::map<std::string, std::string> kHist;
    };
    for (size_t i = 0; i < kMissions.size(); ++i)
    {
        const std::string& m = kMissions[i];
        std::map<std::string, Block> kBlocks;
        Block* pCur = 0;
        const std::vector<std::string> kLines = LogLines(RunLogOf(kCases, m), "Memory report ", true);
        for (size_t j = 0; j < kLines.size(); ++j)
        {
            const std::string& l = kLines[j];
            if (StartsWith(l, "Memory report source="))
            {
                Block b;
                b.sSource = l.substr(21);
                kBlocks[b.sSource] = b;
                pCur = &kBlocks[b.sSource];
                continue;
            }
            if (!pCur)
            {
                continue;
            }
            std::smatch p;
            std::smatch s;
            std::smatch h;
            if (std::regex_match(l, p, kPoolRx))
            {
                std::vector<std::string> kGroups;
                for (size_t g = 1; g <= 5; ++g)
                {
                    kGroups.push_back(p[g].str());
                }
                pCur->kPools.push_back(kGroups);
            }
            else if (std::regex_match(l, s, kSpillRx))
            {
                pCur->kSpill[s[1].str()] = s[2].str();
            }
            else if (StartsWith(l, "Memory report luaspill "))
            {
                return Fail(k, "bad luaspill line in " + m + ": " + l);
            }
            else if (std::regex_match(l, h, kHistRx))
            {
                if (pCur->kHist.count(h[1].str()))
                {
                    return Fail(k, "duplicate luahist bin " + h[1].str() + " in " + m + " " + pCur->sSource);
                }
                pCur->kHist[h[1].str()] = h[2].str() + "\t" + h[3].str();
            }
            else if (StartsWith(l, "Memory report luahist "))
            {
                return Fail(k, "bad luahist line in " + m + ": " + l);
            }
        }
        const Block* b = 0;
        if (kBlocks.count("turn1500"))
        {
            b = &kBlocks["turn1500"];
        }
        else if (kBlocks.count("exit"))
        {
            b = &kBlocks["exit"];
        }
        if (!b || b->kHist.empty())
        {
            kHist.push_back(m + "\t-\t-\t-");
        }
        else
        {
            std::vector<std::string> kBins;
            for (std::map<std::string, std::string>::const_iterator it = b->kHist.begin(); it != b->kHist.end(); ++it)
            {
                kBins.push_back(it->first);
            }
            std::stable_sort(kBins.begin(), kBins.end(), [](const std::string& x, const std::string& y) {
                unsigned long long a = ~0ULL;
                unsigned long long c = ~0ULL;
                if (x != "over")
                {
                    ParseDecimal(x, a);
                }
                if (y != "over")
                {
                    ParseDecimal(y, c);
                }
                return a < c;
            });
            for (size_t j = 0; j < kBins.size(); ++j)
            {
                kHist.push_back(m + "\t" + kBins[j] + "\t" + b->kHist.find(kBins[j])->second);
            }
        }
        if (!b || b->kPools.empty())
        {
            kRows.push_back(m + "\t" + (b ? b->sSource : std::string("-")) + Dashes(6));
            continue;
        }
        for (size_t j = 0; j < b->kPools.size(); ++j)
        {
            const std::vector<std::string>& g = b->kPools[j];
            std::map<std::string, std::string>::const_iterator it = b->kSpill.find(g[0]);
            const std::string sSpill = it != b->kSpill.end() ? it->second : std::string("-");
            kRows.push_back(m + "\t" + b->sSource + "\t" + g[0] + "\t" + g[1] + "\t" + g[2] + "\t" + g[3] + "\t" + g[4] +
                            "\t" + sSpill);
        }
    }
    AddTable(k, "lua-bins.txt", kRows);
    AddTable(k, "lua-hist.txt", kHist);
    return k;
}

BuildResult BuildDebugDigests(const std::vector<CaseLog>& kCases)
{
    BuildResult k = Ok();
    static const char* const sapcCases[] = {"twice-digest-1", "digest-hot1g_con", "digest-spa3c_ass"};
    std::vector<std::string> kRows;
    std::vector<std::string> kMissing;
    for (size_t i = 0; i < 3; ++i)
    {
        const std::string sDigest = DigestKey(RunLogOf(kCases, sapcCases[i]));
        if (sDigest.empty())
        {
            kMissing.push_back(sapcCases[i]);
            continue;
        }
        kRows.push_back(std::string(sapcCases[i]) + "\t" + sDigest);
    }
    if (!kMissing.empty())
    {
        return Fail(k, "no digest line for " + JoinText(kMissing, ", "));
    }
    AddTable(k, "digests-debug.txt", kRows);
    return k;
}

BuildResult BuildCollect(const std::vector<CaseLog>& kCases, int iRunCode)
{
    BuildResult k = Ok();
    std::vector<std::string> kDigests;
    std::vector<std::string> kHogDigests;
    std::vector<std::string> kProblems;
    std::vector<std::string> kHogDiff;
    if (iRunCode != 0)
    {
        kProblems.push_back("the test runner exited " + std::to_string(iRunCode));
    }
    const std::regex kStateRx("state=(0x[0-9A-F]{8})");
    static const char* const sapcFull[] = {"geo1c_con", "nab2g_c", "hot1g_con", "spa3c_ass"};
    for (size_t i = 0; i < 4; ++i)
    {
        const std::string m = sapcFull[i];
        const FileText kLog = RunLogOf(kCases, m);
        const std::string sDigest = DigestKey(kLog);
        const std::vector<std::string> kTrace = LogLines(kLog, "Digest trace turn=", false);
        const std::vector<std::string> kFact = FactoryLines(kLog);
        k.kInfo.push_back("INFO: collect " + m + " trace=" + std::to_string(kTrace.size()) +
                          " factory=" + std::to_string(kFact.size()) + " " + sDigest);
        if (sDigest.empty())
        {
            kProblems.push_back(m + " has no SIM DIGEST line");
        }
        if (kTrace.size() != 1600)
        {
            kProblems.push_back(m + " has " + std::to_string(kTrace.size()) + " DIGEST TRACE lines, expected 1600");
        }
        if (kFact.empty())
        {
            kProblems.push_back(m + " has no FACTORY lines");
        }
        AddTable(k, "factory-" + m + ".txt", kFact);
        AddTable(k, "digest-trace-" + m + ".txt", kTrace);
        kDigests.push_back(m + "\t" + sDigest);
        const std::string sHog = DigestKey(RunLogOf(kCases, "hog-" + m));
        k.kInfo.push_back("INFO: collect hog-" + m + " " + sHog);
        if (sHog.empty())
        {
            kProblems.push_back("hog-" + m + " has no SIM DIGEST line");
        }
        kHogDigests.push_back(m + "\t" + sHog);
        std::smatch s1;
        std::smatch s2;
        const bool b1 = std::regex_search(sDigest, s1, kStateRx);
        const bool b2 = std::regex_search(sHog, s2, kStateRx);
        if (!b1 || !b2 || !EqualNoCase(s1[1].str(), s2[1].str()))
        {
            kHogDiff.push_back(m + " base='" + sDigest + "' hog='" + sHog + "'");
        }
    }
    static const char* const sapcTrace[] = {"dag1c_ctf", "kas2c_uber"};
    for (size_t i = 0; i < 2; ++i)
    {
        const std::string m = sapcTrace[i];
        const FileText kLog = RunLogOf(kCases, m);
        const std::vector<std::string> kTrace = LogLines(kLog, "Digest trace turn=", false);
        k.kInfo.push_back("INFO: collect " + m + " trace=" + std::to_string(kTrace.size()) + " " + DigestKey(kLog));
        if (kTrace.size() != 1600)
        {
            kProblems.push_back(m + " has " + std::to_string(kTrace.size()) + " DIGEST TRACE lines, expected 1600");
        }
        AddTable(k, "digest-trace-" + m + ".txt", kTrace);
    }
    AddTable(k, "digests.txt", kDigests);
    AddTable(k, "digests-hog.txt", kHogDigests);
    if (kHogDiff.empty())
    {
        k.kVerdicts.push_back("[PASS] capture:hog-digest same");
    }
    else
    {
        for (size_t i = 0; i < kHogDiff.size(); ++i)
        {
            k.kVerdicts.push_back("[FAIL] capture:hog-digest " + kHogDiff[i]);
        }
        k.bOk = false;
    }
    if (!kProblems.empty())
    {
        return Fail(k, JoinText(kProblems, "; "));
    }
    return k;
}

bool ReadMapSymbols(const std::string& sMap, MapSymbols& kSym, std::string& sWhy)
{
    const std::regex kTsRx("Timestamp is ([0-9a-fA-F]+)");
    const std::regex kBaseRx("Preferred load address is ([0-9a-fA-F]+)");
    const std::regex kSecRx("^\\s*0001:([0-9a-fA-F]{8})\\s+([0-9a-fA-F]{8})H\\s+\\S+\\s+CODE");
    const std::regex kRx("^\\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\\s+(\\S+)\\s+([0-9a-fA-F]{8}(?:[0-9a-fA-F]{8})?)\\s");
    bool bTs = false;
    bool bBase = false;
    bool bOn = false;
    unsigned long long ullTextEnd = 0;
    struct Row
    {
        unsigned long long ullVa;
        std::string sName;
        unsigned long long ullOff;
    };
    std::vector<Row> kRows;
    const std::vector<std::string> kLines = ReadAllLines(sMap);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string& l = kLines[i];
        std::smatch m;
        if (!bTs)
        {
            if (std::regex_search(l, m, kTsRx))
            {
                std::string s = UpperAscii(m[1].str());
                while (s.size() < 8)
                {
                    s = "0" + s;
                }
                kSym.sTimestamp = s;
                bTs = true;
            }
            continue;
        }
        if (!bBase)
        {
            if (std::regex_search(l, m, kBaseRx))
            {
                if (!ParseHex(m[1].str(), kSym.ullBase))
                {
                    sWhy = "preferred load address out of range in the map";
                    return false;
                }
                bBase = true;
            }
            continue;
        }
        if (FindNoCase(l, "Publics by Value") != std::string::npos || EqualNoCase(TrimSpace(l), "Static symbols"))
        {
            bOn = true;
            continue;
        }
        if (!bOn)
        {
            if (std::regex_search(l, m, kSecRx))
            {
                unsigned long long a = 0;
                unsigned long long b = 0;
                ParseHex(m[1].str(), a);
                ParseHex(m[2].str(), b);
                if (a + b > ullTextEnd)
                {
                    ullTextEnd = a + b;
                }
            }
            continue;
        }
        if (!std::regex_search(l, m, kRx) || m[1].str() != "0001")
        {
            continue;
        }
        Row r;
        ParseHex(m[4].str(), r.ullVa);
        r.sName = m[3].str();
        ParseHex(m[2].str(), r.ullOff);
        kRows.push_back(r);
    }
    if (!bTs)
    {
        sWhy = "no Timestamp line in the map";
        return false;
    }
    if (!bBase)
    {
        sWhy = "no Preferred load address line in the map";
        return false;
    }
    if (kRows.empty())
    {
        sWhy = "no code symbols in the map";
        return false;
    }
    if (ullTextEnd == 0)
    {
        sWhy = "no 0001 CODE section lines in the map";
        return false;
    }
    std::vector<Row> kSorted = kRows;
    std::stable_sort(kSorted.begin(), kSorted.end(), [](const Row& a, const Row& b) {
        if (a.ullVa != b.ullVa)
        {
            return a.ullVa < b.ullVa;
        }
        return CultureCompare(a.sName, b.sName, false) < 0;
    });
    kSym.kAddrs.clear();
    kSym.kNames.clear();
    for (size_t i = 0; i < kSorted.size(); ++i)
    {
        if (kSym.kAddrs.empty() || kSym.kAddrs.back() != kSorted[i].ullVa)
        {
            kSym.kAddrs.push_back(kSorted[i].ullVa);
            kSym.kNames.push_back(kSorted[i].sName);
        }
    }
    kSym.ullTextEnd = kRows[0].ullVa - kRows[0].ullOff + ullTextEnd;
    return true;
}

bool FindMapSymbol(const MapSymbols& kSym, const std::string& sFrame, Frame& kFrame, std::string& sWhy)
{
    const size_t uPlus = sFrame.rfind("+0x");
    std::string sHex = uPlus == std::string::npos ? std::string() : sFrame.substr(uPlus + 3);
    bool bHex = !sHex.empty();
    for (size_t i = 0; i < sHex.size(); ++i)
    {
        bHex = bHex && IsHex(sHex[i]);
    }
    if (uPlus == std::string::npos || uPlus == 0 || !bHex)
    {
        sWhy = "bad stack frame '" + sFrame + "'";
        return false;
    }
    kFrame.sFrame = sFrame;
    kFrame.ullOffset = 0;
    kFrame.bSym = false;
    if (!EqualNoCase(sFrame.substr(0, uPlus), "BattlefrontII.exe"))
    {
        return true;
    }
    unsigned long long ullRva = 0;
    if (!ParseHex(sHex, ullRva))
    {
        sWhy = "frame '" + sFrame + "' offset is out of range";
        return false;
    }
    const unsigned long long a = kSym.ullBase + ullRva;
    std::vector<unsigned long long>::const_iterator it = std::upper_bound(kSym.kAddrs.begin(), kSym.kAddrs.end(), a);
    if (it == kSym.kAddrs.begin())
    {
        sWhy = "frame '" + sFrame + "' is below the first map symbol";
        return false;
    }
    if (a >= kSym.ullTextEnd)
    {
        sWhy = "frame '" + sFrame + "' is past the end of .text";
        return false;
    }
    const size_t i = (size_t)(it - kSym.kAddrs.begin()) - 1;
    kFrame.bSym = true;
    kFrame.sName = kSym.kNames[i];
    kFrame.ullOffset = a - kSym.kAddrs[i];
    return true;
}

std::vector<OffRow> OffThreadHeader(const std::string& sCommit)
{
    std::vector<OffRow> k(3);
    k[0].bText = true;
    k[0].sText = "# commit " + sCommit;
    k[1].bText = true;
    k[1].sText = "case\tcount\tsources\tdropped";
    k[2].bText = true;
    k[2].sText = "case\ttid\theap\tsize\tcount\tframe0..frame7";
    return k;
}

bool OffThreadRows(const std::vector<CaseLog>& kCases, const std::string& sSmokeDir, const MapSymbols& kSym,
                   std::vector<OffRow>& kRows, std::string& sWhy)
{
    const std::vector<CaseLog> kSorted = SortedCases(kCases);
    if (kSorted.empty())
    {
        sWhy = "no case dirs under " + sSmokeDir;
        return false;
    }
    const std::regex kTotalRx("^Memory report offthread count=(\\d+) sources=(\\d+) dropped=(\\d+)$");
    const std::regex kSourceRx("^Memory report offthread tid=(\\d+) heap=(\\S+) size=(\\d+) count=(\\d+) stack=(\\S+)$");
    const std::regex kIdRx("build ([0-9A-Fa-f]+)$");
    for (size_t d = 0; d < kSorted.size(); ++d)
    {
        const std::string& c = kSorted[d].sName;
        const std::vector<std::string> kBl = BuildIdLines(kSorted[d].kRunLog);
        if (!kBl.empty())
        {
            std::smatch m;
            std::string sId;
            if (std::regex_search(kBl.back(), m, kIdRx))
            {
                sId = UpperAscii(m[1].str());
            }
            if (sId.size() < 8 || sId.substr(0, 8) != kSym.sTimestamp)
            {
                sWhy = "map timestamp mismatch in " + c + ": build " + sId + " map " + kSym.sTimestamp;
                return false;
            }
        }
        const std::vector<std::string> kOl = LogLines(kSorted[d].kRunLog, "Memory report offthread ", true);
        int iLast = -1;
        int iBlock = -1;
        std::vector<int> kBlocks;
        for (size_t i = 0; i < kOl.size(); ++i)
        {
            if (std::regex_match(kOl[i], kTotalRx))
            {
                iLast = (int)i;
                iBlock = (int)i;
                continue;
            }
            if (iBlock < 0 || !std::regex_match(kOl[i], kSourceRx))
            {
                sWhy = "bad offthread line in " + c + ": " + kOl[i];
                return false;
            }
            if (kBlocks.empty() || kBlocks.back() != iBlock)
            {
                kBlocks.push_back(iBlock);
            }
        }
        if (kBl.empty() && !kBlocks.empty())
        {
            sWhy = "no build id in " + c + "; cannot verify the map";
            return false;
        }
        OffRow kTotal;
        kTotal.bText = true;
        if (iLast < 0)
        {
            kTotal.sText = c + "\t-\t-\t-";
            kRows.push_back(kTotal);
            continue;
        }
        std::smatch t;
        std::regex_match(kOl[(size_t)iLast], t, kTotalRx);
        kTotal.sText = c + "\t" + t[1].str() + "\t" + t[2].str() + "\t" + t[3].str();
        kRows.push_back(kTotal);
        if (kBlocks.empty())
        {
            continue;
        }
        for (size_t i = (size_t)kBlocks.back() + 1; i < kOl.size() && !std::regex_match(kOl[i], kTotalRx); ++i)
        {
            std::smatch s;
            std::regex_match(kOl[i], s, kSourceRx);
            OffRow kSrc;
            kSrc.bText = false;
            kSrc.sPrefix = c + "\t" + s[1].str() + "\t" + s[2].str() + "\t" + s[3].str() + "\t" + s[4].str();
            if (s[5].str() != "-")
            {
                const std::string sStack = s[5].str();
                size_t uStart = 0;
                for (;;)
                {
                    const size_t uComma = sStack.find(',', uStart);
                    Frame kFrame;
                    if (!FindMapSymbol(kSym, sStack.substr(uStart, uComma == std::string::npos ? std::string::npos
                                                                                                : uComma - uStart),
                                       kFrame, sWhy))
                    {
                        return false;
                    }
                    kSrc.kFrames.push_back(kFrame);
                    if (uComma == std::string::npos)
                    {
                        break;
                    }
                    uStart = uComma + 1;
                }
            }
            kRows.push_back(kSrc);
        }
    }
    return true;
}

std::vector<std::string> OffThreadNames(const std::vector<OffRow>& kRows)
{
    std::vector<std::string> k;
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        for (size_t j = 0; j < kRows[i].kFrames.size(); ++j)
        {
            if (kRows[i].kFrames[j].bSym)
            {
                k.push_back(kRows[i].kFrames[j].sName);
            }
        }
    }
    return k;
}

std::vector<std::string> FormatOffThreadRows(const std::vector<OffRow>& kRows,
                                             const std::map<std::string, std::string>& kDem)
{
    std::vector<std::string> kOut;
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        if (kRows[i].bText)
        {
            kOut.push_back(kRows[i].sText);
            continue;
        }
        std::vector<std::string> kCols;
        kCols.push_back(kRows[i].sPrefix);
        for (size_t j = 0; j < kRows[i].kFrames.size(); ++j)
        {
            const Frame& f = kRows[i].kFrames[j];
            if (!f.bSym)
            {
                kCols.push_back(f.sFrame);
                continue;
            }
            std::map<std::string, std::string>::const_iterator it = kDem.find(f.sName);
            std::string n = it != kDem.end() ? it->second : f.sName;
            if (f.ullOffset != 0)
            {
                n += "+0x" + UpperHex(f.ullOffset);
            }
            kCols.push_back(n);
        }
        kOut.push_back(JoinText(kCols, "\t"));
    }
    return kOut;
}

bool MapInitializers(const std::string& sMap, MapTables& kTables, std::string& sWhy)
{
    const std::vector<std::string> kLines = ReadAllLines(sMap);
    const std::regex kSecRx("^\\s*0002:([0-9a-fA-F]{8})\\s+([0-9a-fA-F]{8})H\\s+\\.CRT\\$(XCA|XCU|XCZ)\\s");
    std::map<std::string, std::pair<unsigned long long, unsigned long long> > kSec;
    size_t uStart = 0;
    bool bStart = false;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (FindNoCase(kLines[i], "Publics by Value") != std::string::npos)
        {
            uStart = i + 1;
            bStart = true;
            break;
        }
        std::smatch m;
        if (std::regex_search(kLines[i], m, kSecRx))
        {
            unsigned long long a = 0;
            unsigned long long b = 0;
            ParseHex(m[1].str(), a);
            ParseHex(m[2].str(), b);
            kSec[m[3].str()] = std::make_pair(a, b);
        }
    }
    if (!bStart)
    {
        sWhy = "no Publics by Value table in the map";
        return false;
    }
    static const char* const sapcNames[] = {"XCA", "XCU", "XCZ"};
    for (size_t i = 0; i < 3; ++i)
    {
        if (!kSec.count(sapcNames[i]))
        {
            sWhy = std::string(".CRT$") + sapcNames[i] + " missing from the map section table";
            return false;
        }
    }
    const unsigned long long ullLo = kSec["XCA"].first;
    const unsigned long long ullHi = kSec["XCZ"].first + kSec["XCZ"].second;
    const std::regex kRx("^\\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\\s+(\\S+)\\s+[0-9a-fA-F]{8}\\s+(?:f\\s+)?(?:i\\s+)?(\\S.*?)\\s*$");
    kTables.kInRange.clear();
    kTables.kDiscarded.clear();
    kTables.kDi.clear();
    for (size_t i = uStart; i < kLines.size(); ++i)
    {
        std::smatch m;
        if (!std::regex_search(kLines[i], m, kRx))
        {
            continue;
        }
        const std::string sSecNo = m[1].str();
        const std::string sSym = m[3].str();
        const bool bDi = StartsWith(sSym, "??__E");
        const bool bInit = sSym.find("$initializer$") != std::string::npos;
        if (sSecNo != "0002" && !bDi)
        {
            continue;
        }
        unsigned long long ullOff = 0;
        ParseHex(m[2].str(), ullOff);
        const bool bIn = sSecNo == "0002" && ullOff >= ullLo && ullOff < ullHi;
        if (!(bIn || bDi || bInit))
        {
            continue;
        }
        unsigned long long ullSec = 0;
        ParseHex(sSecNo, ullSec);
        MapRow r;
        r.iSec = (int)ullSec;
        r.ullOffset = ullOff;
        r.sSymbol = sSym;
        r.sObj = m[4].str();
        r.uLine = i;
        if (bIn)
        {
            kTables.kInRange.push_back(r);
        }
        else if (bInit && sSecNo == "0002")
        {
            kTables.kDiscarded.push_back(r);
        }
        if (bDi)
        {
            kTables.kDi.push_back(r);
        }
    }
    std::stable_sort(kTables.kInRange.begin(), kTables.kInRange.end(), LessMapRow);
    std::stable_sort(kTables.kDiscarded.begin(), kTables.kDiscarded.end(), LessMapRow);
    std::stable_sort(kTables.kDi.begin(), kTables.kDi.end(), LessDiRow);
    if (kTables.kInRange.empty())
    {
        sWhy = "no symbols inside the .CRT$XCA..XCZ range";
        return false;
    }
    kTables.ullXcuSize = kSec["XCU"].second;
    return true;
}

std::vector<std::string> MapNames(const MapTables& kTables)
{
    std::vector<std::string> k;
    const std::vector<MapRow>* apLists[] = {&kTables.kInRange, &kTables.kDiscarded, &kTables.kDi};
    for (size_t l = 0; l < 3; ++l)
    {
        for (size_t i = 0; i < apLists[l]->size(); ++i)
        {
            k.push_back((*apLists[l])[i].sSymbol);
        }
    }
    return k;
}

std::vector<std::string> FormatMapRows(const std::vector<MapRow>& kRows, const std::map<std::string, std::string>& kDem)
{
    std::vector<std::string> kOut;
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        std::map<std::string, std::string>::const_iterator it = kDem.find(kRows[i].sSymbol);
        const std::string d = it != kDem.end() ? it->second : kRows[i].sSymbol;
        kOut.push_back(std::to_string(i) + "\t" + kRows[i].sSymbol + "\t" + d + "\t" + kRows[i].sObj);
    }
    return kOut;
}

std::string MapInfoLine(const MapTables& kTables, size_t uDemangled)
{
    return "INFO: map XCU entries=" + std::to_string(RoundHalfEven(kTables.ullXcuSize, 4)) +
           " initializers=" + std::to_string(kTables.kInRange.size()) +
           " discarded=" + std::to_string(kTables.kDiscarded.size()) + " di=" + std::to_string(kTables.kDi.size()) +
           " demangled=" + std::to_string(uDemangled);
}

std::vector<std::string> UndnameArgLines(const std::vector<std::string>& kNames)
{
    std::vector<std::string> kList;
    std::set<std::string> kSeen;
    for (size_t i = 0; i < kNames.size(); ++i)
    {
        if (kSeen.insert(kNames[i]).second)
        {
            kList.push_back(kNames[i]);
        }
    }
    std::vector<std::string> kOut;
    for (size_t s = 0; s < kList.size(); s += 100)
    {
        std::vector<std::string> kChunk;
        for (size_t i = s; i < kList.size() && i < s + 100; ++i)
        {
            kChunk.push_back("\"" + kList[i] + "\"");
        }
        kOut.push_back(JoinText(kChunk, " "));
    }
    return kOut;
}

void ParseUndname(const std::string& sOutput, std::map<std::string, std::string>& kDem)
{
    const std::regex kRx("Undecoration of :- \"([^\\n]*)\"\\r?\\nis :- \"([^\\n]*)\"");
    for (std::sregex_iterator it(sOutput.begin(), sOutput.end(), kRx), e; it != e; ++it)
    {
        kDem[(*it)[1].str()] = (*it)[2].str();
    }
}

}
