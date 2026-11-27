#include "tables/tables_repin.h"

#include <map>
#include <regex>
#include <set>

#include "tables/tables_build.h"

namespace Tables
{

namespace
{

RepinResult Refuse(RepinResult& k, int iExit, const std::string& sWhy)
{
    k.iExit = iExit;
    k.kLines.push_back("[FAIL] repin - " + sWhy);
    return k;
}

std::string TrimAll(const std::string& s)
{
    size_t uStart = 0;
    while (uStart < s.size() && (s[uStart] == ' ' || (s[uStart] >= '\t' && s[uStart] <= '\r')))
    {
        ++uStart;
    }
    return TrimEndSpace(s.substr(uStart));
}

std::vector<std::string> RowKeys(const std::vector<std::string>& kLines, int iKeyCols, bool bOffThread)
{
    std::vector<std::string> kKeys;
    bool bStop = false;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string& sLine = kLines[i];
        if (bOffThread && sLine == "## sound-on")
        {
            bStop = true;
        }
        if (bStop || sLine.empty() || StartsWith(sLine, "#") || StartsWith(sLine, "mission\t") ||
            (bOffThread && StartsWith(sLine, "case\t")))
        {
            kKeys.push_back(std::string());
            continue;
        }
        const std::vector<std::string> kCols = SplitTabs(sLine);
        std::vector<std::string> kKey;
        for (int c = 0; c < iKeyCols && (size_t)c < kCols.size(); ++c)
        {
            kKey.push_back(kCols[(size_t)c]);
        }
        kKeys.push_back(JoinText(kKey, "/"));
    }
    return kKeys;
}

bool Contains(const std::vector<std::string>& k, const std::string& s)
{
    for (size_t i = 0; i < k.size(); ++i)
    {
        if (k[i] == s)
        {
            return true;
        }
    }
    return false;
}

}

const std::vector<std::string>& RepinTables()
{
    static const std::vector<std::string> k = {"smoke-all-digests", "mem-peaks", "pool-sizes", "cloth-headroom",
                                               "lua-bins",          "offthread", "digest-expected"};
    return k;
}

bool RepinTableFile(const std::string& sTable, std::string& sFile, int& iKeyCols, std::string& sWhy)
{
    sFile = sTable + ".txt";
    if (sTable == "digest-expected")
    {
        iKeyCols = 1;
        return true;
    }
    if (!KeyColumnsFor(sFile, iKeyCols))
    {
        sWhy = "the reader table has no key columns for " + sFile;
        return false;
    }
    return true;
}

std::vector<std::string> RepinKeys(const std::vector<std::string>& kArgs)
{
    std::vector<std::string> kAll;
    for (size_t i = 0; i < kArgs.size(); ++i)
    {
        size_t uStart = 0;
        for (;;)
        {
            const size_t uComma = kArgs[i].find(',', uStart);
            const std::string s =
                TrimAll(kArgs[i].substr(uStart, uComma == std::string::npos ? std::string::npos : uComma - uStart));
            if (!s.empty())
            {
                kAll.push_back(s);
            }
            if (uComma == std::string::npos)
            {
                break;
            }
            uStart = uComma + 1;
        }
    }
    CultureSort(kAll, true);
    std::vector<std::string> kOut;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (kOut.empty() || CultureCompare(kOut.back(), kAll[i], true) != 0)
        {
            kOut.push_back(kAll[i]);
        }
    }
    return kOut;
}

std::string RunParam(const std::string& sRunParams, const std::string& sName)
{
    std::string sValue;
    const std::vector<std::string> kLines = ReadAllLines(sRunParams);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const size_t uEq = kLines[i].find('=');
        if (uEq == std::string::npos || uEq < 1)
        {
            continue;
        }
        if (EqualNoCase(kLines[i].substr(0, uEq), sName))
        {
            sValue = kLines[i].substr(uEq + 1);
        }
    }
    return sValue;
}

RepinResult Repin(const RepinInput& kIn)
{
    RepinResult k;
    k.iExit = 1;
    bool bKnown = false;
    for (size_t i = 0; i < RepinTables().size(); ++i)
    {
        bKnown = bKnown || EqualNoCase(RepinTables()[i], kIn.sTable);
    }
    if (!bKnown)
    {
        return Refuse(k, 1, "-Table " + kIn.sTable + " does not belong to the set " + JoinText(RepinTables(), ","));
    }
    const std::string sBaselineDir =
        kIn.sBaselineDir.empty() ? JoinPath(kIn.sRoot, "tests\\baseline\\v143") : kIn.sBaselineDir;
    const std::string sExpectedFile =
        kIn.sExpectedFile.empty() ? JoinPath(kIn.sRoot, "tests\\smoke\\digest-expected.txt") : kIn.sExpectedFile;
    const std::vector<std::string> kKeys = RepinKeys(kIn.kKeyArgs);
    if (kKeys.empty())
    {
        return Refuse(k, 2, "no -Key given");
    }
    const FileText kParams = kIn.fnRead(JoinPath(kIn.sRunDir, "run-params.txt"));
    if (!kParams.bFound)
    {
        return Refuse(k, 2, "no run-params.txt in " + kIn.sRunDir);
    }
    std::string sExe = kIn.sExe;
    if (sExe.empty())
    {
        const std::string sPlatform = RunParam(kParams.sText, "platform");
        const std::string sConfig = RunParam(kParams.sText, "smoke_configuration");
        if (sPlatform.empty() || sConfig.empty())
        {
            return Refuse(k, 2, "run-params.txt names no platform or smoke configuration");
        }
        sExe = JoinPath(kIn.sRoot, "bin\\" + sPlatform + "\\" + sConfig + "\\BattlefrontII.exe");
    }
    const std::string sRunSha = RunParam(kParams.sText, "exe.BattlefrontII.exe");
    if (!std::regex_match(sRunSha, std::regex("[0-9a-f]{64}")))
    {
        return Refuse(k, 1, "run-params.txt has no exe hash ('" + sRunSha + "')");
    }
    std::string sCurSha;
    if (!kIn.fnExeSha(sExe, sCurSha))
    {
        return Refuse(k, 1, "exe not found: " + sExe);
    }
    if (sCurSha != sRunSha)
    {
        return Refuse(k, 1, "the run's exe " + sRunSha + " is not the current exe " + sCurSha + " (" + sExe + ")");
    }
    const std::string sCausesFile = JoinPath(sBaselineDir, "CAUSES.md");
    const FileText kCauses = kIn.fnRead(sCausesFile);
    if (!kCauses.bFound)
    {
        return Refuse(k, 1, sCausesFile + " not found");
    }
    std::set<std::string> kCaused;
    const std::regex kCauseRx("^- `(\\S+) (\\S+)` \\| \\S+ \\| ");
    const std::vector<std::string> kCauseLines = ReadAllLines(kCauses.sText);
    for (size_t i = 0; i < kCauseLines.size(); ++i)
    {
        std::smatch m;
        if (std::regex_search(kCauseLines[i], m, kCauseRx))
        {
            kCaused.insert(m[1].str() + " " + m[2].str());
        }
    }
    bool bUncaused = false;
    for (size_t i = 0; i < kKeys.size(); ++i)
    {
        if (!kCaused.count(kIn.sTable + " " + kKeys[i]))
        {
            k.kLines.push_back("[FAIL] repin - no cause line in " + sCausesFile + " names '" + kIn.sTable + " " + kKeys[i] +
                               "'");
            bUncaused = true;
        }
    }
    if (bUncaused)
    {
        k.iExit = 1;
        return k;
    }
    const bool bOffThread = kIn.sTable == "offthread";
    std::string sPinned;
    int iKeyCols = 1;
    std::string sSource;
    if (kIn.sTable == "digest-expected")
    {
        sPinned = sExpectedFile;
    }
    else
    {
        sPinned = JoinPath(sBaselineDir, kIn.sTable + ".txt");
        const std::string sSrcDir = (kIn.sTable == "lua-bins" || bOffThread) ? kIn.sTable : std::string("tables");
        sSource = JoinPath(kIn.sRunDir, sSrcDir + "\\" + kIn.sTable + ".txt");
        std::string sFile;
        std::string sWhy;
        if (!RepinTableFile(kIn.sTable, sFile, iKeyCols, sWhy))
        {
            return Refuse(k, 2, sWhy);
        }
    }
    std::map<std::string, std::vector<std::string> > kWant;
    std::vector<std::string> kMissing;
    if (kIn.sTable == "digest-expected")
    {
        for (size_t i = 0; i < kKeys.size(); ++i)
        {
            const std::string sCaseDir = JoinPath(kIn.sRunDir, "smoke\\" + kKeys[i]);
            const std::string sDigest = DigestKey(kIn.fnRead(JoinPath(sCaseDir, "run.log")));
            if (!sDigest.empty())
            {
                kWant[kKeys[i]].push_back(kKeys[i] + "\t" + sDigest);
            }
            else
            {
                kMissing.push_back(kKeys[i] + " has no SIM DIGEST line in " + sCaseDir);
            }
        }
    }
    else
    {
        const FileText kSource = kIn.fnRead(sSource);
        if (!kSource.bFound)
        {
            return Refuse(k, 1, sSource + " not found");
        }
        const std::vector<std::string> kSrcLines = ReadAllLines(kSource.sText);
        const std::vector<std::string> kSrcKeys = RowKeys(kSrcLines, iKeyCols, bOffThread);
        for (size_t i = 0; i < kSrcLines.size(); ++i)
        {
            if (kSrcKeys[i].empty() || !Contains(kKeys, kSrcKeys[i]))
            {
                continue;
            }
            kWant[kSrcKeys[i]].push_back(kSrcLines[i]);
        }
        for (size_t i = 0; i < kKeys.size(); ++i)
        {
            if (!kWant.count(kKeys[i]))
            {
                kMissing.push_back(kKeys[i] + " is not a row of " + sSource);
            }
        }
    }
    const FileText kPinned = kIn.fnRead(sPinned);
    if (!kPinned.bFound)
    {
        return Refuse(k, 1, sPinned + " not found");
    }
    std::string sText = kPinned.sText;
    if (sText.size() >= 3 && (unsigned char)sText[0] == 0xEF && (unsigned char)sText[1] == 0xBB &&
        (unsigned char)sText[2] == 0xBF)
    {
        sText.erase(0, 3);
    }
    const std::string sNl = sText.find("\r\n") != std::string::npos ? "\r\n" : "\n";
    std::vector<std::string> kLines;
    size_t uStart = 0;
    for (;;)
    {
        const size_t uAt = sText.find(sNl, uStart);
        if (uAt == std::string::npos)
        {
            kLines.push_back(sText.substr(uStart));
            break;
        }
        kLines.push_back(sText.substr(uStart, uAt - uStart));
        uStart = uAt + sNl.size();
    }
    const std::vector<std::string> kLineKeys = RowKeys(kLines, iKeyCols, bOffThread);
    for (size_t i = 0; i < kKeys.size(); ++i)
    {
        if (!Contains(kLineKeys, kKeys[i]))
        {
            kMissing.push_back(kKeys[i] + " is not a row of " + sPinned);
        }
    }
    if (!kMissing.empty())
    {
        for (size_t i = 0; i < kMissing.size(); ++i)
        {
            k.kLines.push_back("[FAIL] repin - " + kMissing[i]);
        }
        k.iExit = 1;
        return k;
    }
    std::vector<std::string> kOut;
    std::set<std::string> kDone;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string& sKey = kLineKeys[i];
        if (!sKey.empty() && kWant.count(sKey))
        {
            if (kDone.insert(sKey).second)
            {
                kOut.insert(kOut.end(), kWant[sKey].begin(), kWant[sKey].end());
            }
            continue;
        }
        kOut.push_back(kLines[i]);
    }
    k.sWritePath = sPinned;
    k.sWriteText = JoinText(kOut, sNl);
    for (size_t i = 0; i < kKeys.size(); ++i)
    {
        k.kLines.push_back("[PASS] repin " + kIn.sTable + " " + kKeys[i] + " (" + std::to_string(kWant[kKeys[i]].size()) +
                           " rows from exe " + sRunSha + ")");
    }
    k.iExit = 0;
    return k;
}

}
