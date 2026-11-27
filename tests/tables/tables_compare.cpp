#include "tables/tables_compare.h"

#include <algorithm>
#include <regex>
#include <set>

#include "harness/compare.h"

namespace Tables
{

namespace
{

TableVerdict Thrown(const std::string& sWhy)
{
    TableVerdict k;
    k.bOk = false;
    k.bThrown = true;
    k.sLine = sWhy;
    return k;
}

TableVerdict Verdict(bool bOk, const std::string& sLine, const std::vector<std::string>& kProblems)
{
    TableVerdict k;
    k.bOk = bOk;
    k.bThrown = false;
    k.sLine = sLine;
    k.kProblems = kProblems;
    return k;
}

std::string KeyOf(const std::string& sLine, int iKeyCols)
{
    const std::vector<std::string> kCols = SplitTabs(sLine);
    std::vector<std::string> kKey;
    for (int i = 0; i < iKeyCols && (size_t)i < kCols.size(); ++i)
    {
        kKey.push_back(kCols[(size_t)i]);
    }
    return JoinText(kKey, "\t");
}

std::vector<std::string> SortedKeys(const KeyedRows& kRows)
{
    std::vector<std::string> k;
    for (KeyedRows::const_iterator it = kRows.begin(); it != kRows.end(); ++it)
    {
        k.push_back(it->first);
    }
    CultureSort(k, true);
    return k;
}

std::vector<std::string> SortUniqueNoCase(std::vector<std::string> k)
{
    CultureSort(k, true);
    std::vector<std::string> kOut;
    for (size_t i = 0; i < k.size(); ++i)
    {
        if (kOut.empty() || CultureCompare(kOut.back(), k[i], true) != 0)
        {
            kOut.push_back(k[i]);
        }
    }
    return kOut;
}

bool ParseCount(const std::string& s, unsigned long long& ull)
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

struct HeapPeaks
{
    std::map<std::string, unsigned long long> kPeak;
    std::map<std::string, std::vector<std::string> > kCaps;
};

bool ReadHeapPeaks(const std::string& sPath, const FileText& kText, HeapPeaks& kOut, std::string& sWhy)
{
    if (!kText.bFound)
    {
        sWhy = sPath + " not found";
        return false;
    }
    const std::vector<std::string> kLines = ReadAllLines(kText.sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string& sLine = kLines[i];
        if (sLine.empty() || StartsWith(sLine, "#") || StartsWith(sLine, "mission\t"))
        {
            continue;
        }
        const std::vector<std::string> c = SplitTabs(sLine);
        if (c.size() != 10 || c[2] != "heap")
        {
            continue;
        }
        if (!AllDigits(c[4]) || !AllDigits(c[6]))
        {
            sWhy = sPath + " heap row without a cap and peak: " + sLine;
            return false;
        }
        unsigned long long ullPeak = 0;
        if (!ParseCount(c[6], ullPeak))
        {
            sWhy = sPath + " heap row peak out of range: " + sLine;
            return false;
        }
        std::map<std::string, unsigned long long>::iterator it = kOut.kPeak.find(c[3]);
        if (it == kOut.kPeak.end() || ullPeak > it->second)
        {
            kOut.kPeak[c[3]] = ullPeak;
        }
        std::vector<std::string>& kCaps = kOut.kCaps[c[3]];
        if (std::find(kCaps.begin(), kCaps.end(), c[4]) == kCaps.end())
        {
            kCaps.push_back(c[4]);
        }
    }
    return true;
}

bool CeilDiv(unsigned long long a, unsigned long long b, unsigned long long& q)
{
    q = a / b + (a % b != 0 ? 1ULL : 0ULL);
    return true;
}

bool DerivedCap(unsigned long long ullPeak64, unsigned long long ullCap86, unsigned long long ullPeak86,
                unsigned long long& ullOut)
{
    if (ullCap86 != 0 && ullPeak64 > ~0ULL / ullCap86)
    {
        return false;
    }
    unsigned long long q = 0;
    CeilDiv(ullPeak64 * ullCap86, ullPeak86, q);
    unsigned long long uBlocks = 0;
    CeilDiv(q, 65536ULL, uBlocks);
    if (uBlocks > ~0ULL / 65536ULL)
    {
        return false;
    }
    ullOut = uBlocks * 65536ULL;
    return true;
}

}

std::string VerdictText(const TableVerdict& kVerdict)
{
    std::string s = kVerdict.sLine + "\n";
    for (size_t i = 0; i < kVerdict.kProblems.size(); ++i)
    {
        s += "    " + kVerdict.kProblems[i] + "\n";
    }
    return s;
}

bool ReadKeyed(const FileText& kText, const std::string& sName, const std::string& sDir, int iKeyCols, bool bOffThread,
               const std::string& sSkipRx, KeyedRows& kRows, std::string& sWhy)
{
    if (!kText.bFound)
    {
        sWhy = sName + " not found in " + sDir;
        return false;
    }
    std::regex kSkip;
    if (!sSkipRx.empty())
    {
        kSkip = std::regex(sSkipRx);
    }
    int iCount = 0;
    const std::vector<std::string> kLines = ReadAllLines(kText.sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::string sLine = kLines[i];
        if (bOffThread)
        {
            if (sLine == "## sound-on")
            {
                break;
            }
            if (StartsWith(sLine, "case\t"))
            {
                continue;
            }
            std::vector<std::string> kCols = SplitTabs(sLine);
            if (kCols.size() > 4)
            {
                kCols.erase(kCols.begin() + 1);
                sLine = JoinText(kCols, "\t");
            }
        }
        if (sLine.empty() || StartsWith(sLine, "#") || StartsWith(sLine, "mission\t"))
        {
            continue;
        }
        ++iCount;
        const std::string sKey = KeyOf(sLine, iKeyCols);
        if (!sSkipRx.empty() && std::regex_search(sKey, kSkip))
        {
            continue;
        }
        kRows[sKey].push_back(sLine);
    }
    if (iCount == 0)
    {
        sWhy = sName + " in " + sDir + " has no rows";
        return false;
    }
    return true;
}

TableVerdict CompareTable(const std::string& sLabel, const std::string& sName, int iKeyCols, bool bOffThread,
                          const std::string& sSkipRx, const std::string& sBaseDir, const FileText& kBase,
                          const std::string& sTablesDir, const FileText& kNew)
{
    KeyedRows kB;
    KeyedRows kT;
    std::string sWhy;
    if (!ReadKeyed(kBase, sName, sBaseDir, iKeyCols, bOffThread, sSkipRx, kB, sWhy) ||
        !ReadKeyed(kNew, sName, sTablesDir, iKeyCols, bOffThread, sSkipRx, kT, sWhy))
    {
        return Thrown(sWhy);
    }
    std::vector<std::string> kBad;
    if (kB.empty())
    {
        kBad.push_back("no baseline rows to compare");
    }
    const std::vector<std::string> kBaseKeys = SortedKeys(kB);
    for (size_t i = 0; i < kBaseKeys.size(); ++i)
    {
        const std::string& k = kBaseKeys[i];
        KeyedRows::const_iterator it = kT.find(k);
        if (it == kT.end())
        {
            kBad.push_back(k + " missing from tables");
            continue;
        }
        const std::vector<std::string>& b = kB[k];
        const std::vector<std::string>& t = it->second;
        const size_t n = b.size() > t.size() ? b.size() : t.size();
        for (size_t j = 0; j < n; ++j)
        {
            const std::string bl = j < b.size() ? b[j] : std::string("<none>");
            const std::string tl = j < t.size() ? t[j] : std::string("<none>");
            if (tl != bl)
            {
                kBad.push_back("tables '" + tl + "' baseline '" + bl + "'");
                break;
            }
        }
    }
    const std::vector<std::string> kNewKeys = SortedKeys(kT);
    for (size_t i = 0; i < kNewKeys.size(); ++i)
    {
        if (!kB.count(kNewKeys[i]))
        {
            kBad.push_back(kNewKeys[i] + " not in baseline");
        }
    }
    const std::string sKeys = " keys=" + std::to_string(kB.size());
    return Verdict(kBad.empty(), (kBad.empty() ? "[PASS] " : "[FAIL] ") + sLabel + sKeys, kBad);
}

TableVerdict CompareMemPeaksKeys(const std::string& sLabel, const std::string& sName, const std::string& sPair,
                                 const std::string& sBins, const std::string& sBaseX86, const FileText& kPair,
                                 const std::string& sTablesDir, const FileText& kNew, const std::string& sBaseX64,
                                 const FileText& kBins)
{
    KeyedRows kBase;
    KeyedRows kN;
    KeyedRows kBinRows;
    std::string sWhy;
    if (!ReadKeyed(kPair, sPair, sBaseX86, 4, false, "", kBase, sWhy) ||
        !ReadKeyed(kNew, sName, sTablesDir, 4, false, "", kN, sWhy) ||
        !ReadKeyed(kBins, sBins, sBaseX64, 3, false, "", kBinRows, sWhy))
    {
        return Thrown(sWhy);
    }
    const std::string sTn = "x64 " + sName;
    const std::string sBn = "x86 " + sPair;
    std::vector<std::string> kBad;
    const std::regex kLuaRx("^[^\t]+\t[^\t]+\tpool\tLua\\d+$");
    std::set<std::string> kWant;
    for (KeyedRows::const_iterator it = kBinRows.begin(); it != kBinRows.end(); ++it)
    {
        const std::vector<std::string> c = SplitTabs(it->first);
        if (c.size() > 2 && AllDigits(c[2]))
        {
            kWant.insert(c[0] + "\t" + c[1] + "\tpool\tLua" + c[2]);
        }
    }
    if (kWant.empty())
    {
        kBad.push_back("x64 " + sBins + " has no bins");
    }
    for (KeyedRows::iterator it = kBase.begin(); it != kBase.end();)
    {
        if (std::regex_search(it->first, kLuaRx))
        {
            it = kBase.erase(it);
        }
        else
        {
            ++it;
        }
    }
    std::vector<std::string> kLuaKeys;
    for (KeyedRows::const_iterator it = kN.begin(); it != kN.end(); ++it)
    {
        if (std::regex_search(it->first, kLuaRx))
        {
            kLuaKeys.push_back(it->first);
        }
    }
    CultureSort(kLuaKeys, true);
    for (size_t i = 0; i < kLuaKeys.size(); ++i)
    {
        const std::string& k = kLuaKeys[i];
        if (!kWant.count(k))
        {
            kBad.push_back(k + " not a bin of x64 " + sBins);
        }
        else if (kN[k].size() != 1)
        {
            kBad.push_back(k + " has " + std::to_string(kN[k].size()) + " rows in " + sTn);
        }
        kWant.erase(k);
        kN.erase(k);
    }
    std::vector<std::string> kLeft(kWant.begin(), kWant.end());
    CultureSort(kLeft, true);
    for (size_t i = 0; i < kLeft.size(); ++i)
    {
        kBad.push_back(kLeft[i] + " missing from " + sTn);
    }
    const std::vector<std::string> kNewKeys = SortedKeys(kN);
    for (size_t i = 0; i < kNewKeys.size(); ++i)
    {
        const std::vector<std::string>& kLines = kN[kNewKeys[i]];
        for (size_t j = 0; j < kLines.size(); ++j)
        {
            const std::vector<std::string> c = SplitTabs(kLines[j]);
            if (c.size() != 10)
            {
                kBad.push_back(sTn + " row has " + std::to_string(c.size()) + " columns: " + kLines[j]);
            }
            else if (c[2] == "heap" && (!AllDigits(c[4]) || !AllDigits(c[6])))
            {
                kBad.push_back(sTn + " heap row without a cap and peak: " + kLines[j]);
            }
        }
    }
    std::vector<std::string> kBm;
    for (KeyedRows::const_iterator it = kBase.begin(); it != kBase.end(); ++it)
    {
        kBm.push_back(SplitTabs(it->first)[0]);
    }
    std::vector<std::string> kNm;
    for (KeyedRows::const_iterator it = kN.begin(); it != kN.end(); ++it)
    {
        kNm.push_back(SplitTabs(it->first)[0]);
    }
    kBm = SortUniqueNoCase(kBm);
    kNm = SortUniqueNoCase(kNm);
    if (JoinText(kBm, " ") != JoinText(kNm, " "))
    {
        kBad.push_back(sTn + " covers " + std::to_string(kNm.size()) + " missions and " + sBn + " " +
                       std::to_string(kBm.size()));
    }
    const std::vector<std::string> kBaseKeys = SortedKeys(kBase);
    for (size_t i = 0; i < kBaseKeys.size(); ++i)
    {
        const std::string& k = kBaseKeys[i];
        KeyedRows::const_iterator it = kN.find(k);
        if (it == kN.end())
        {
            kBad.push_back(k + " missing from " + sTn);
        }
        else if (kBase[k].size() != it->second.size())
        {
            kBad.push_back(k + " has " + std::to_string(it->second.size()) + " rows in " + sTn + " and " +
                           std::to_string(kBase[k].size()) + " in " + sBn);
        }
    }
    for (size_t i = 0; i < kNewKeys.size(); ++i)
    {
        if (!kBase.count(kNewKeys[i]))
        {
            kBad.push_back(kNewKeys[i] + " not in " + sBn);
        }
    }
    if (!kBad.empty())
    {
        return Verdict(false, "[FAIL] " + sLabel + " keys=" + std::to_string(kN.size()), kBad);
    }
    return Verdict(true,
                   "[PASS] " + sLabel + " keys=" + std::to_string(kN.size()) + " missions=" + std::to_string(kNm.size()),
                   kBad);
}

TableVerdict CompareHeapCapsX64(const std::string& sLabel, const std::string& sX64Path, const FileText& kX64,
                                const std::string& sX86Path, const FileText& kX86, const std::string& sRunLog,
                                const FileText& kRunLog)
{
    HeapPeaks k64;
    HeapPeaks k86;
    std::string sWhy;
    if (!ReadHeapPeaks(sX64Path, kX64, k64, sWhy) || !ReadHeapPeaks(sX86Path, kX86, k86, sWhy))
    {
        return Thrown(sWhy);
    }
    std::vector<std::string> kBad;
    std::map<std::string, unsigned long long> kWant;
    std::vector<std::string> kHeaps86;
    for (std::map<std::string, std::vector<std::string> >::const_iterator it = k86.kCaps.begin(); it != k86.kCaps.end();
         ++it)
    {
        kHeaps86.push_back(it->first);
    }
    CultureSort(kHeaps86, true);
    for (size_t i = 0; i < kHeaps86.size(); ++i)
    {
        const std::string& h = kHeaps86[i];
        const std::vector<std::string>& kCaps = k86.kCaps[h];
        if (kCaps.size() != 1)
        {
            kBad.push_back(h + " has " + std::to_string(kCaps.size()) + " caps in " + sX86Path + ": " +
                           JoinText(kCaps, ", "));
            continue;
        }
        if (!k64.kPeak.count(h))
        {
            kBad.push_back(h + " has no heap rows in " + sX64Path);
            continue;
        }
        unsigned long long ullCap86 = 0;
        if (!ParseCount(kCaps[0], ullCap86))
        {
            kBad.push_back(h + " cap " + kCaps[0] + " in " + sX86Path + " is out of range");
            continue;
        }
        const unsigned long long ullPeak64 = k64.kPeak[h];
        const unsigned long long ullPeak86 = k86.kPeak[h];
        unsigned long long w = ullCap86;
        if (ullPeak64 > 0 && ullPeak86 > 0)
        {
            unsigned long long d = 0;
            if (!DerivedCap(ullPeak64, ullCap86, ullPeak86, d))
            {
                kBad.push_back(h + " derived cap is out of range");
                continue;
            }
            if (d > ullCap86)
            {
                w = d;
            }
        }
        kWant[h] = w;
    }
    std::vector<std::string> kHeaps64;
    for (std::map<std::string, unsigned long long>::const_iterator it = k64.kPeak.begin(); it != k64.kPeak.end(); ++it)
    {
        kHeaps64.push_back(it->first);
    }
    CultureSort(kHeaps64, true);
    for (size_t i = 0; i < kHeaps64.size(); ++i)
    {
        if (!k86.kCaps.count(kHeaps64[i]))
        {
            kBad.push_back(kHeaps64[i] + " has no heap rows in " + sX86Path);
        }
    }
    std::vector<std::string> kWantKeys;
    for (std::map<std::string, unsigned long long>::const_iterator it = kWant.begin(); it != kWant.end(); ++it)
    {
        kWantKeys.push_back(it->first);
    }
    CultureSort(kWantKeys, true);
    std::vector<std::string> kDerived;
    for (size_t i = 0; i < kWantKeys.size(); ++i)
    {
        kDerived.push_back(kWantKeys[i] + "=" + std::to_string(kWant[kWantKeys[i]]));
    }
    if (!sRunLog.empty())
    {
        if (!kRunLog.bFound)
        {
            kBad.push_back(sRunLog + " not found");
        }
        else
        {
            std::set<std::string> kSeen;
            const std::regex kRx("Memory report heap=(\\S+) cap=(\\d+) ");
            size_t uCount = 0;
            for (std::sregex_iterator it(kRunLog.sText.begin(), kRunLog.sText.end(), kRx), e; it != e; ++it)
            {
                ++uCount;
                const std::string h = (*it)[1].str();
                kSeen.insert(h);
                unsigned long long c = 0;
                if (!ParseCount((*it)[2].str(), c))
                {
                    kBad.push_back(h + " logged cap " + (*it)[2].str() + " is out of range");
                    continue;
                }
                if (!kWant.count(h))
                {
                    kBad.push_back(h + " logged cap " + std::to_string(c) + " has no derived cap");
                }
                else if (c != kWant[h])
                {
                    kBad.push_back(h + " logged cap " + std::to_string(c) + " derived " + std::to_string(kWant[h]));
                }
            }
            if (uCount == 0)
            {
                kBad.push_back(sRunLog + " has no MEMREPORT heap lines");
            }
            else
            {
                for (size_t i = 0; i < kWantKeys.size(); ++i)
                {
                    if (!kSeen.count(kWantKeys[i]))
                    {
                        kBad.push_back(kWantKeys[i] + " derived " + std::to_string(kWant[kWantKeys[i]]) + " not logged");
                    }
                }
            }
        }
    }
    if (!kBad.empty())
    {
        std::vector<std::string> kUnique;
        for (size_t i = 0; i < kBad.size(); ++i)
        {
            if (std::find(kUnique.begin(), kUnique.end(), kBad[i]) == kUnique.end())
            {
                kUnique.push_back(kBad[i]);
            }
        }
        return Verdict(false, "[FAIL] " + sLabel + " heaps=" + std::to_string(kWant.size()), kUnique);
    }
    return Verdict(true, "[PASS] " + sLabel + " " + JoinText(kDerived, " "), kBad);
}

TableVerdict CompareTableBytes(const std::string& sLabel, const std::string& sName, const std::string& sBaseDir,
                               const FileText& kBase, const std::string& sTablesDir, const FileText& kNew)
{
    if (!kBase.bFound)
    {
        return Thrown(sName + " not found in " + sBaseDir);
    }
    std::vector<std::string> kBad;
    if (!kNew.bFound)
    {
        kBad.push_back(sName + " not found in " + sTablesDir);
        return Verdict(false, "[FAIL] " + sLabel, kBad);
    }
    const std::vector<std::string> kBl = ReadAllLines(kBase.sText);
    if (kBase.sText.empty())
    {
        kBad.push_back(sName + " in " + sBaseDir + " is empty");
        return Verdict(false, "[FAIL] " + sLabel, kBad);
    }
    const std::string sLines = " lines=" + std::to_string(kBl.size());
    std::string sBytes;
    if (CompareBytes(kBase.sText, kNew.sText, sBytes))
    {
        return Verdict(true, "[PASS] " + sLabel + sLines, kBad);
    }
    const std::vector<std::string> kTl = ReadAllLines(kNew.sText);
    const size_t n = kBl.size() > kTl.size() ? kBl.size() : kTl.size();
    std::string sMsg = "bytes differ with equal lines (line endings or final newline): tables " +
                       std::to_string(kNew.sText.size()) + " bytes, baseline " + std::to_string(kBase.sText.size()) +
                       " bytes";
    for (size_t i = 0; i < n; ++i)
    {
        const std::string x = i < kBl.size() ? kBl[i] : std::string("<none>");
        const std::string y = i < kTl.size() ? kTl[i] : std::string("<none>");
        if (y != x)
        {
            sMsg = "line " + std::to_string(i + 1) + ": tables '" + y + "' baseline '" + x + "'";
            break;
        }
    }
    kBad.push_back(sMsg);
    return Verdict(false, "[FAIL] " + sLabel + sLines, kBad);
}

std::string BaselineDirFor(const std::string& sName, const CompareInput& kIn)
{
    if (kIn.sPlatform == "x64" && kIn.fnRead(JoinPath(kIn.sBaseX64, sName)).bFound)
    {
        return kIn.sBaseX64;
    }
    return kIn.sBaseX86;
}

std::vector<TableVerdict> CompareMode(ReaderKind eKind, const CompareInput& kIn)
{
    std::vector<TableVerdict> kOut;
    const std::string sMode = KindName(eKind);
    if (eKind == kReadCommit || eKind == kReadX87Inventory || eKind == kReadUnit || eKind == kReadVerify)
    {
        kOut.push_back(Verdict(false, "[FAIL] tables:" + sMode + " - not a table compare", std::vector<std::string>()));
        return kOut;
    }
    const std::vector<Reader> kEntries = SelectReaders(eKind, kIn.sPlatform);
    if (kEntries.empty())
    {
        kOut.push_back(Verdict(false, "[FAIL] tables:" + sMode + " - no reader map entry for -" + sMode + " on " +
                                          kIn.sPlatform,
                               std::vector<std::string>()));
        return kOut;
    }
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        const Reader& e = kEntries[i];
        kOut.push_back(CompareEntry(e, kIn));
        if (kOut.back().bThrown)
        {
            return kOut;
        }
        if (e.pcPair && *e.pcPair)
        {
            kOut.push_back(CompareHeapCaps(e, kIn));
            if (kOut.back().bThrown)
            {
                return kOut;
            }
        }
    }
    return kOut;
}

TableVerdict CompareEntry(const Reader& e, const CompareInput& kIn)
{
    const std::string sFile = e.pcFile;
    if (e.pcPair && *e.pcPair)
    {
        return CompareMemPeaksKeys(e.pcLabel, sFile, e.pcPair, e.pcBins, kIn.sBaseX86,
                                   kIn.fnRead(JoinPath(kIn.sBaseX86, e.pcPair)), kIn.sTables,
                                   kIn.fnRead(JoinPath(kIn.sTables, sFile)), kIn.sBaseX64,
                                   kIn.fnRead(JoinPath(kIn.sBaseX64, e.pcBins)));
    }
    const std::string sDir = BaselineDirFor(sFile, kIn);
    if (e.iKeyCols > 0)
    {
        return CompareTable(e.pcLabel, sFile, e.iKeyCols, e.bOffThread, e.pcSkipRx ? e.pcSkipRx : "", sDir,
                            kIn.fnRead(JoinPath(sDir, sFile)), kIn.sTables, kIn.fnRead(JoinPath(kIn.sTables, sFile)));
    }
    return CompareTableBytes(e.pcLabel, sFile, sDir, kIn.fnRead(JoinPath(sDir, sFile)), kIn.sTables,
                             kIn.fnRead(JoinPath(kIn.sTables, sFile)));
}

TableVerdict CompareHeapCaps(const Reader& e, const CompareInput& kIn)
{
    const std::string sX64 = JoinPath(kIn.sTables, e.pcFile);
    const std::string sX86 = JoinPath(kIn.sBaseX86, e.pcPair ? e.pcPair : "");
    return CompareHeapCapsX64("tables:heap-caps-x64", sX64, kIn.fnRead(sX64), sX86, kIn.fnRead(sX86), kIn.sRunLog,
                              kIn.sRunLog.empty() ? Absent() : kIn.fnRead(kIn.sRunLog));
}

bool ModeFailed(const std::vector<TableVerdict>& kVerdicts)
{
    for (size_t i = 0; i < kVerdicts.size(); ++i)
    {
        if (!kVerdicts[i].bOk)
        {
            return true;
        }
    }
    return false;
}

}
