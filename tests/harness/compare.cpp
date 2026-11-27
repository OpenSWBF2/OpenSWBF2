#include "harness/compare.h"

#include <algorithm>
#include <ctype.h>
#include <map>
#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static std::vector<std::string> LogLines(const std::string& sText)
{
    std::vector<std::string> kLines;
    size_t uStart = 0;
    while (uStart < sText.size())
    {
        size_t uEnd = sText.find('\n', uStart);
        if (uEnd == std::string::npos)
        {
            uEnd = sText.size();
        }
        std::string sLine = sText.substr(uStart, uEnd - uStart);
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        kLines.push_back(sLine);
        uStart = uEnd + 1;
    }
    return kLines;
}

static std::string Trim(const std::string& s)
{
    const size_t uFirst = s.find_first_not_of(" \t");
    if (uFirst == std::string::npos)
    {
        return std::string();
    }
    return s.substr(uFirst, s.find_last_not_of(" \t") - uFirst + 1);
}

std::vector<std::string> MarkerLines(const std::string& sLog, const char* pcMarker)
{
    std::vector<std::string> kOut;
    const std::vector<std::string> kLines = LogLines(sLog);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const size_t uAt = kLines[i].find(pcMarker);
        if (uAt != std::string::npos)
        {
            kOut.push_back(kLines[i].substr(uAt));
        }
    }
    return kOut;
}

std::string LastMarkerLine(const std::string& sLog, const char* pcMarker)
{
    const std::vector<std::string> kLines = MarkerLines(sLog, pcMarker);
    return kLines.empty() ? std::string() : kLines.back();
}

bool FindTableRow(const std::string& sTable, const char* pcSection, const std::string& sName, std::string& sRow)
{
    const std::vector<std::string> kLines = LogLines(sTable);
    std::string sSection;
    int iFound = 0;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (pcSection && kLines[i].compare(0, 3, "## ") == 0)
        {
            sSection = kLines[i].substr(3);
            continue;
        }
        if (pcSection && sSection != pcSection)
        {
            continue;
        }
        const std::string sLine = pcSection ? kLines[i] : Trim(kLines[i]);
        if (sLine.empty() || sLine[0] == '#')
        {
            continue;
        }
        const size_t uTab = sLine.find('\t');
        if (uTab != std::string::npos && uTab > 0 && sLine.compare(0, uTab, sName) == 0 && uTab == sName.size())
        {
            sRow = sLine.substr(uTab + 1);
            ++iFound;
        }
    }
    return iFound == 1;
}

bool CompareDigestRow(const std::string& sLog, const std::string& sRow, std::string& sWhy)
{
    const std::string sGot = LastMarkerLine(sLog, "Sim digest ");
    if (sRow.empty() || sGot != sRow)
    {
        sWhy = "digest expected '" + sRow + "', actual '" + sGot + "'";
        return false;
    }
    return true;
}

bool HasDigestTrace(const std::string& sLog, unsigned int uTurn)
{
    char acPattern[96];
    _snprintf(acPattern, sizeof(acPattern) - 1, "Digest trace turn=%u state=0x[0-9A-F]{8} rand=\\d+/\\d+$", uTurn);
    acPattern[sizeof(acPattern) - 1] = 0;
    const std::regex kRx(acPattern);
    const std::vector<std::string> kLines = MarkerLines(sLog, "Digest trace ");
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (std::regex_match(kLines[i], kRx))
        {
            return true;
        }
    }
    return false;
}

bool CompareSameRun(const std::string& sLog, const std::string& sRefLog, std::string& sWhy)
{
    const std::string sDigest = LastMarkerLine(sLog, "Sim digest ");
    const std::string sRefDigest = LastMarkerLine(sRefLog, "Sim digest ");
    if (sDigest.empty() || sDigest != sRefDigest)
    {
        sWhy = "digest differs: '" + sDigest + "' reference '" + sRefDigest + "'";
        return false;
    }
    const std::string sCloth = LastMarkerLine(sLog, "Cloth free=");
    const std::string sRefCloth = LastMarkerLine(sRefLog, "Cloth free=");
    if (sCloth.empty() || sRefCloth.empty())
    {
        sWhy = "CLOTH line missing";
        return false;
    }
    if (sCloth != sRefCloth)
    {
        sWhy = "CLOTH differs: '" + sCloth + "' reference '" + sRefCloth + "'";
        return false;
    }
    return true;
}

bool CompareEqualDigests(const std::string& sLog, int iCount, std::string& sWhy)
{
    const std::vector<std::string> kLines = MarkerLines(sLog, "Sim digest ");
    char acWhy[96];
    if ((int)kLines.size() != iCount)
    {
        _snprintf(acWhy, sizeof(acWhy) - 1, "expected %d equal SIM DIGEST lines, got %d", iCount, (int)kLines.size());
        acWhy[sizeof(acWhy) - 1] = 0;
        sWhy = acWhy;
        return false;
    }
    for (size_t i = 1; i < kLines.size(); ++i)
    {
        if (kLines[i] != kLines[0])
        {
            sWhy = "SIM DIGEST lines differ: '" + kLines[0] + "' and '" + kLines[i] + "'";
            return false;
        }
    }
    return true;
}

static std::vector<std::string> Turn1500Runtime(const std::string& sLog)
{
    std::vector<std::string> kOut;
    const std::regex kRx("Memory report heap=Runtime cap=\\d+ (peak=\\d+ used=\\d+) ");
    const std::vector<std::string> kLines = LogLines(sLog);
    bool bAfter = false;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string& sLine = kLines[i];
        const size_t uAt = sLine.find("Memory report source=");
        if (uAt != std::string::npos)
        {
            bAfter = sLine.compare(uAt, std::string::npos, "Memory report source=turn1500") == 0;
            continue;
        }
        std::smatch kMatch;
        if (bAfter && std::regex_search(sLine, kMatch, kRx))
        {
            kOut.push_back(kMatch[1].str());
            bAfter = false;
        }
    }
    return kOut;
}

bool CompareTurn1500Runtime(const std::string& sLog, const std::string& sRefLog, std::string& sWhy)
{
    const std::vector<std::string> kRef = Turn1500Runtime(sRefLog);
    const std::vector<std::string> kGot = Turn1500Runtime(sLog);
    if (kRef.empty())
    {
        sWhy = "the reference run has no Runtime heap line after the turn-1500 memory report";
        return false;
    }
    if (kGot.empty() || kGot.back() != kRef.front())
    {
        sWhy = "Runtime at turn 1500 '" + (kGot.empty() ? std::string() : kGot.back()) + "' differs from the reference '" +
               kRef.front() + "'";
        return false;
    }
    return true;
}

struct CommitPart
{
    std::string sState;
    long long llArena;
    long long llRest;
};

static std::vector<CommitPart> CommitParts(const std::string& sLog)
{
    std::vector<CommitPart> kParts;
    const std::regex kRx("Commit parts state=(\\w+) private=\\d+ gamecrt=(\\d+) heaps=\\d+ threads=\\d+ "
                         "threadcount=\\d+ arena=(\\d+) crtbusy=\\d+ crtcommitted=\\d+ crtlarge=\\d+ gamevm=(\\d+)");
    const std::vector<std::string> kLines = LogLines(sLog);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::smatch kMatch;
        if (std::regex_search(kLines[i], kMatch, kRx))
        {
            CommitPart kPart;
            kPart.sState = kMatch[1].str();
            kPart.llArena = _strtoi64(kMatch[3].str().c_str(), 0, 10);
            kPart.llRest = _strtoi64(kMatch[4].str().c_str(), 0, 10) + _strtoi64(kMatch[2].str().c_str(), 0, 10);
            kParts.push_back(kPart);
        }
    }
    return kParts;
}

static std::string Join(const std::vector<std::string>& kItems)
{
    std::string sOut;
    for (size_t i = 0; i < kItems.size(); ++i)
    {
        sOut += (i ? "," : "") + kItems[i];
    }
    return sOut;
}

bool CompareCommitFlat(const std::string& sLog, long long llBound, int iCycles, int iSettled, std::string& sWhy)
{
    if (iCycles < 1 || iSettled < 1)
    {
        sWhy = "cycles and settled cycles are required";
        return false;
    }
    if (iSettled >= iCycles)
    {
        sWhy = "settled cycles must be below cycles";
        return false;
    }
    std::vector<std::string> kStates;
    const std::regex kRx("Commit state=(\\w+) bytes=(\\d+)");
    const std::vector<std::string> kLines = LogLines(sLog);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::smatch kMatch;
        if (std::regex_search(kLines[i], kMatch, kRx))
        {
            kStates.push_back(kMatch[1].str());
        }
    }
    std::vector<std::string> kWant;
    for (int i = 0; i < 2 * iCycles; ++i)
    {
        kWant.push_back(i % 2 ? "shell" : "mission");
    }
    if (kStates != kWant)
    {
        sWhy = "expected " + std::to_string(2 * iCycles) + " states starting with mission and alternating, got '" +
               Join(kStates) + "'";
        return false;
    }
    const std::vector<CommitPart> kParts = CommitParts(sLog);
    std::vector<std::string> kPartStates;
    for (size_t i = 0; i < kParts.size(); ++i)
    {
        kPartStates.push_back(kParts[i].sState);
    }
    if (kPartStates != kWant)
    {
        sWhy = "expected a COMMITPARTS row after each COMMIT row, got '" + Join(kPartStates) + "'";
        return false;
    }
    static const char* const sapcStates[] = {"mission", "shell"};
    for (int iState = 0; iState < 2; ++iState)
    {
        std::vector<long long> kArena;
        std::vector<long long> kRest;
        for (size_t i = 0; i < kParts.size(); ++i)
        {
            if (kParts[i].sState == sapcStates[iState])
            {
                kArena.push_back(kParts[i].llArena);
                kRest.push_back(kParts[i].llRest);
            }
        }
        const std::vector<long long>* apValues[2] = {&kArena, &kRest};
        static const char* const sapcParts[] = {"arena", "rest"};
        for (int iPart = 0; iPart < 2; ++iPart)
        {
            const std::vector<long long>& kV = *apValues[iPart];
            const long long llPeak = *std::max_element(kV.begin(), kV.begin() + iSettled);
            for (size_t i = (size_t)iSettled; i < kV.size(); ++i)
            {
                if (kV[i] - llPeak > llBound)
                {
                    sWhy = std::string(sapcStates[iState]) + " " + std::to_string(i + 1) + " " + sapcParts[iPart] + " = " +
                           std::to_string(kV[i]) + " is " + std::to_string(kV[i] - llPeak) + " above the peak " +
                           std::to_string(llPeak) + " of cycles 1-" + std::to_string(iSettled) + ", bound " +
                           std::to_string(llBound);
                    return false;
                }
            }
        }
    }
    return true;
}

bool CompareCommitRows(const std::string& sLog, const std::string& sTable, bool bSteam, bool bHog, int iSettled,
                       std::string& sWhy)
{
    const std::vector<std::string> kTable = LogLines(sTable);
    std::vector<std::string> kBody;
    for (size_t i = 0; i < kTable.size(); ++i)
    {
        if (!kTable[i].empty() && kTable[i][0] != '#')
        {
            kBody.push_back(kTable[i]);
        }
    }
    if (kTable.empty() || !std::regex_match(kTable[0], std::regex("# commit [0-9a-f]{7,40}")))
    {
        sWhy = "commit table does not start with '# commit <sha>'";
        return false;
    }
    if (kBody.empty() || kBody[0] != "steam\thog\tstate\tarena\trest")
    {
        sWhy = "commit table header is not steam, hog, state, arena, rest separated by tabs";
        return false;
    }
    const std::regex kRowRx("(yes|no)\t([01])\t(mission|shell)\t(\\d+)\t(\\d+)");
    std::map<std::string, std::pair<long long, long long> > kRows;
    for (size_t i = 1; i < kBody.size(); ++i)
    {
        std::smatch kMatch;
        if (!std::regex_match(kBody[i], kMatch, kRowRx))
        {
            sWhy = "commit table row is not steam, hog, state, arena, rest: " + kBody[i];
            return false;
        }
        const std::string sKey = kMatch[1].str() + " " + kMatch[2].str() + " " + kMatch[3].str();
        if (kRows.count(sKey))
        {
            sWhy = "commit table has two rows for " + sKey;
            return false;
        }
        kRows[sKey] = std::make_pair(_strtoi64(kMatch[4].str().c_str(), 0, 10), _strtoi64(kMatch[5].str().c_str(), 0, 10));
    }
    if (iSettled < 1)
    {
        sWhy = "settled cycles must be 1 or more";
        return false;
    }
    const std::vector<CommitPart> kParts = CommitParts(sLog);
    static const char* const sapcStates[] = {"mission", "shell"};
    for (int iState = 0; iState < 2; ++iState)
    {
        const std::string sKey = std::string(bSteam ? "yes" : "no") + " " + (bHog ? "1" : "0") + " " + sapcStates[iState];
        if (!kRows.count(sKey))
        {
            sWhy = "no commit row for steam hog state " + sKey;
            return false;
        }
        std::vector<CommitPart> kV;
        for (size_t i = 0; i < kParts.size(); ++i)
        {
            if (kParts[i].sState == sapcStates[iState])
            {
                kV.push_back(kParts[i]);
            }
        }
        if ((int)kV.size() <= iSettled)
        {
            sWhy = std::string(sapcStates[iState]) + " has " + std::to_string(kV.size()) +
                   " COMMITPARTS rows, none after cycle " + std::to_string(iSettled);
            return false;
        }
        for (size_t i = (size_t)iSettled; i < kV.size(); ++i)
        {
            if (kV[i].llArena != kRows[sKey].first || kV[i].llRest != kRows[sKey].second)
            {
                sWhy = std::string(sapcStates[iState]) + " " + std::to_string(i + 1) + " arena " +
                       std::to_string(kV[i].llArena) + " rest " + std::to_string(kV[i].llRest) + ", the row for " + sKey +
                       " has arena " + std::to_string(kRows[sKey].first) + " rest " + std::to_string(kRows[sKey].second);
                return false;
            }
        }
    }
    return true;
}

bool CompareBytes(const std::string& sA, const std::string& sB, std::string& sWhy)
{
    if (sA == sB)
    {
        return true;
    }
    size_t uAt = 0;
    while (uAt < sA.size() && uAt < sB.size() && sA[uAt] == sB[uAt])
    {
        ++uAt;
    }
    sWhy = "bytes differ at offset " + std::to_string(uAt) + " (" + std::to_string(sA.size()) + " and " +
           std::to_string(sB.size()) + " bytes)";
    return false;
}

bool CompareSmokeAllRow(const std::string& sLog, const std::string& sFinalScore, const std::string& sRow,
                        std::string& sWhy)
{
    std::string sDigest = LastMarkerLine(sLog, "Sim digest ");
    if (sDigest.empty())
    {
        sDigest = "none";
    }
    const std::vector<std::string> kScore = LogLines(sFinalScore);
    std::string sScore = kScore.empty() ? std::string() : kScore[0];
    const size_t uTab = sScore.find('\t');
    if (uTab != std::string::npos)
    {
        sScore.erase(0, uTab + 1);
    }
    if (sScore.empty())
    {
        sScore = "none";
    }
    const std::string sGot = sDigest + "\t" + sScore;
    if (sRow.empty() || sGot != sRow)
    {
        sWhy = "smoke-all-digests row expected '" + sRow + "', actual '" + sGot + "'";
        return false;
    }
    return true;
}

bool CompareOffThreadRow(const std::string& sLog, const std::string& sRow, std::string& sWhy)
{
    const std::regex kTotal("^Memory report offthread count=(\\d+) sources=(\\d+) dropped=(\\d+)$");
    std::string sGot = "-\t-\t-";
    const std::vector<std::string> kLines = LogLines(sLog);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (kLines[i].find("Soak turns=") != std::string::npos)
        {
            break;
        }
        const size_t uAt = kLines[i].find("Memory report offthread ");
        if (uAt == std::string::npos)
        {
            continue;
        }
        const std::string sLine = Trim(kLines[i].substr(uAt));
        std::smatch kMatch;
        if (std::regex_match(sLine, kMatch, kTotal))
        {
            sGot = kMatch[1].str() + "\t" + kMatch[2].str() + "\t" + kMatch[3].str();
        }
    }
    if (sRow.empty() || sGot != sRow)
    {
        sWhy = "offthread row expected '" + sRow + "', actual '" + sGot + "'";
        return false;
    }
    return true;
}

static bool IniWordStart(const std::string& sIni, size_t uAt)
{
    if (uAt == 0)
    {
        return true;
    }
    const unsigned char c = (unsigned char)sIni[uAt - 1];
    return !(isalnum(c) || c == '_');
}

bool IniHasLine(const std::string& sIni, const char* pcLine)
{
    const std::vector<std::string> kLines = LogLines(sIni);
    return std::find(kLines.begin(), kLines.end(), std::string(pcLine)) != kLines.end();
}

bool IniHasKey(const std::string& sIni, const char* pcKey)
{
    const std::string sKey = std::string(pcKey) + "=";
    for (size_t uAt = sIni.find(sKey); uAt != std::string::npos; uAt = sIni.find(sKey, uAt + 1))
    {
        if (IniWordStart(sIni, uAt))
        {
            return true;
        }
    }
    return false;
}

static std::vector<std::string> Words(const std::string& sLine)
{
    std::vector<std::string> kWords;
    size_t uAt = 0;
    while (uAt < sLine.size())
    {
        const size_t uStart = sLine.find_first_not_of(" \t", uAt);
        if (uStart == std::string::npos)
        {
            break;
        }
        const size_t uEnd = sLine.find_first_of(" \t", uStart);
        kWords.push_back(sLine.substr(uStart, uEnd == std::string::npos ? std::string::npos : uEnd - uStart));
        uAt = uEnd == std::string::npos ? sLine.size() : uEnd;
    }
    return kWords;
}

bool HasArgWord(const std::string& sArgs, const std::string& sArg)
{
    const std::vector<std::string> kWords = Words(sArgs);
    return std::find(kWords.begin(), kWords.end(), sArg) != kWords.end();
}

std::string DigestRowName(const std::string& sCase)
{
    return sCase == "crash-delayload-x64" ? std::string("crash-delayload") : sCase;
}

bool CommitHogMode(const std::string& sPlatform, const std::string& sArgs, bool& bHog, std::string& sWhy)
{
    bHog = HasArgWord(sArgs, "-hogaddrspace");
    if (sPlatform == "x64" && !bHog)
    {
        sWhy = "an x64 commit-flat run needs -hogaddrspace: v143-x64\\commit.txt holds hog=1 rows only "
               "(C-X64-COMMIT-TABLE)";
        return false;
    }
    return true;
}

static bool IsMissionName(const std::string& sName)
{
    if (sName.empty())
    {
        return false;
    }
    for (size_t i = 0; i < sName.size(); ++i)
    {
        if (!isalnum((unsigned char)sName[i]) && sName[i] != '_')
        {
            return false;
        }
    }
    return true;
}

bool ParseMissions(const std::string& sText, std::vector<MissionLine>& kMissions, std::string& sError)
{
    kMissions.clear();
    const std::vector<std::string> kLines = LogLines(sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string sLineNo = std::to_string(i + 1);
        const std::vector<std::string> kWords = Words(kLines[i]);
        if (kWords.empty())
        {
            sError = "missions line " + sLineNo + " is blank";
            return false;
        }
        if (kWords.size() > 2)
        {
            sError = "missions line " + sLineNo + " has more than a mission and a class";
            return false;
        }
        MissionLine kLine;
        kLine.sName = kWords[0];
        kLine.sClass = kWords.size() > 1 ? kWords[1] : "Pass";
        if (!IsMissionName(kLine.sName))
        {
            sError = "missions line " + sLineNo + ": '" + kLine.sName + "' is not a name of letters, digits and '_'";
            return false;
        }
        if (kLine.sClass != "Pass" && kLine.sClass != "TimeOut")
        {
            sError = "missions line " + sLineNo + ": unknown class '" + kLine.sClass + "'";
            return false;
        }
        for (size_t j = 0; j < kMissions.size(); ++j)
        {
            if (kMissions[j].sName == kLine.sName)
            {
                sError = "missions line " + sLineNo + ": " + kLine.sName + " is listed twice";
                return false;
            }
        }
        kMissions.push_back(kLine);
    }
    if (kMissions.empty())
    {
        sError = "the missions file lists no mission";
        return false;
    }
    return true;
}

bool CheckMissionClass(const std::string& sClass, unsigned int uTurns, unsigned int uSoakTurns, unsigned int uCap,
                       std::string& sWhy)
{
    if (sClass == "Pass")
    {
        if (uTurns >= uCap)
        {
            sWhy = "class Pass, but the run reached the turn cap " + std::to_string(uCap);
            return false;
        }
        return true;
    }
    if (sClass == "TimeOut")
    {
        if (uTurns < uSoakTurns)
        {
            sWhy = "class TimeOut, but the run ended at progress turn " + std::to_string(uTurns) + " before -soakturns " +
                   std::to_string(uSoakTurns);
            return false;
        }
        return true;
    }
    sWhy = "unknown class '" + sClass + "'";
    return false;
}

static bool LastNumber(const std::string& sLog, const char* pcMarker, unsigned long long& ullValue)
{
    const std::string sLine = LastMarkerLine(sLog, pcMarker);
    const size_t uLen = strlen(pcMarker);
    if (sLine.size() <= uLen || !isdigit((unsigned char)sLine[uLen]))
    {
        return false;
    }
    ullValue = _strtoui64(sLine.c_str() + uLen, 0, 10);
    return true;
}

bool LastTurnsCovered(const std::string& sLog, unsigned int uTurns, std::string& sWhy)
{
    unsigned long long ullWant = 0;
    if (!LastNumber(sLog, "Soak turns=", ullWant) && !LastNumber(sLog, "Sim digest turns=", ullWant))
    {
        return true;
    }
    if ((unsigned long long)uTurns < ullWant)
    {
        sWhy = "progress turns " + std::to_string(uTurns) + " below the logged turns=" + std::to_string(ullWant);
        return false;
    }
    return true;
}

std::string FinalScoreKey(const std::string& sFinalScore)
{
    const std::vector<std::string> kLines = LogLines(sFinalScore);
    if (kLines.empty())
    {
        return std::string();
    }
    const size_t uTab = kLines[0].find('\t');
    return uTab == std::string::npos ? kLines[0] : kLines[0].substr(uTab + 1);
}

bool CompareFinalScores(const std::string& sA, const std::string& sB, std::string& sWhy)
{
    const std::string sKeyA = FinalScoreKey(sA);
    const std::string sKeyB = FinalScoreKey(sB);
    if (sKeyA.empty() || sKeyA != sKeyB)
    {
        sWhy = "final scores differ: '" + sKeyA + "' and '" + sKeyB + "'";
        return false;
    }
    return true;
}

bool CompareLastDigests(const std::string& sLogA, const std::string& sLogB, std::string& sWhy)
{
    const std::string sA = LastMarkerLine(sLogA, "Sim digest ");
    const std::string sB = LastMarkerLine(sLogB, "Sim digest ");
    if (sA.empty() || sA != sB)
    {
        sWhy = "digests differ: '" + sA + "' and '" + sB + "'";
        return false;
    }
    return true;
}

typedef std::map<unsigned int, std::string> TraceMap;

static TraceMap DigestTrace(const std::string& sLog)
{
    TraceMap kMap;
    const std::regex kRx("Digest trace turn=(\\d+) (.*)");
    const std::vector<std::string> kLines = LogLines(sLog);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::smatch kMatch;
        if (std::regex_search(kLines[i], kMatch, kRx))
        {
            kMap[(unsigned int)strtoul(kMatch[1].str().c_str(), 0, 10)] = kMatch[2].str();
        }
    }
    return kMap;
}

static std::string FirstWord(const std::string& s)
{
    return s.substr(0, s.find(' '));
}

int TraceSplitTurn(const std::string& sLogA, const std::string& sLogB)
{
    const TraceMap kA = DigestTrace(sLogA);
    const TraceMap kB = DigestTrace(sLogB);
    if (kA.empty() || kB.empty())
    {
        return -1;
    }
    const size_t uLast = kA.size() > kB.size() ? kA.size() : kB.size();
    for (unsigned int t = 1; t <= uLast; ++t)
    {
        TraceMap::const_iterator itA = kA.find(t);
        TraceMap::const_iterator itB = kB.find(t);
        if (itA == kA.end() || itB == kB.end())
        {
            return -1;
        }
        if (FirstWord(itA->second) != FirstWord(itB->second))
        {
            return (int)t;
        }
    }
    return 0;
}

bool TraceMatchesDigest(const std::string& sLog, std::string& sWhy)
{
    const std::string sDigest = LastMarkerLine(sLog, "Sim digest ");
    std::smatch kMatch;
    if (!std::regex_search(sDigest, kMatch, std::regex("turns=(\\d+) .*state=(0x[0-9A-F]{8})")))
    {
        sWhy = "no SIM DIGEST line with turns and state: '" + sDigest + "'";
        return false;
    }
    const unsigned int uTurn = (unsigned int)strtoul(kMatch[1].str().c_str(), 0, 10);
    const std::string sState = kMatch[2].str();
    const TraceMap kTrace = DigestTrace(sLog);
    TraceMap::const_iterator it = kTrace.find(uTurn);
    if (it == kTrace.end() || it->second.compare(0, 6 + sState.size() + 1, "state=" + sState + " ") != 0)
    {
        sWhy = "the digest trace at turn " + std::to_string(uTurn) + " is '" + (it == kTrace.end() ? "" : it->second) +
               "', the digest state is " + sState;
        return false;
    }
    return true;
}

static std::vector<TraceMap> TraceSegments(const std::string& sLog)
{
    std::vector<TraceMap> kSegs(1);
    const std::regex kRx("Digest trace turn=(\\d+) (state=0x[0-9A-F]{8} rand=\\d+/\\d+)$");
    const std::vector<std::string> kLines = LogLines(sLog);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::smatch kMatch;
        if (std::regex_search(kLines[i], kMatch, kRx))
        {
            kSegs.back()[(unsigned int)strtoul(kMatch[1].str().c_str(), 0, 10)] = kMatch[2].str();
        }
        else if (kLines[i].find("Repeat to_mission") != std::string::npos)
        {
            kSegs.push_back(TraceMap());
        }
    }
    return kSegs;
}

static int CompareTraces(const TraceMap& kA, const TraceMap& kB, unsigned int uMaxTurn)
{
    if (kA.empty() || kB.empty())
    {
        return -1;
    }
    const size_t uLast = uMaxTurn > 0 ? uMaxTurn : (kA.size() > kB.size() ? kA.size() : kB.size());
    for (unsigned int t = 1; t <= uLast; ++t)
    {
        TraceMap::const_iterator itA = kA.find(t);
        TraceMap::const_iterator itB = kB.find(t);
        if (itA == kA.end() || itB == kB.end())
        {
            return -1;
        }
        if (itA->second != itB->second)
        {
            return (int)t;
        }
    }
    return 0;
}

static unsigned int TraceEnd(const TraceMap& kMap)
{
    return kMap.empty() ? 0 : kMap.rbegin()->first;
}

static const unsigned int kPerturbSimTurn = 1500;

bool CheckPerturbBase(const std::string& sBase, std::string& sWhy)
{
    const std::vector<TraceMap> kSegs = TraceSegments(sBase);
    if (kSegs.size() != 1)
    {
        sWhy = "the base run has " + std::to_string(kSegs.size()) + " trace segments, not 1";
        return false;
    }
    const unsigned int uEnd = TraceEnd(kSegs[0]);
    if (uEnd < kPerturbSimTurn || CompareTraces(kSegs[0], kSegs[0], uEnd) != 0)
    {
        sWhy = "the base trace does not hold every turn from 1 to at least " + std::to_string(kPerturbSimTurn) +
               " (last turn " + std::to_string(uEnd) + ")";
        return false;
    }
    return true;
}

bool ComparePerturbRun(const std::string& sBase, const std::string& sRun, bool bRepeat, std::string& sWhy)
{
    const std::vector<TraceMap> kBaseSegs = TraceSegments(sBase);
    const TraceMap& kBase = kBaseSegs[0];
    const unsigned int uBaseEnd = TraceEnd(kBase);
    const std::vector<TraceMap> kSegs = TraceSegments(sRun);
    const size_t uWant = bRepeat ? 2 : 1;
    int iSplit = -1;
    size_t uSeg = 0;
    if (uBaseEnd > 0 && kSegs.size() == uWant)
    {
        for (uSeg = 0; uSeg < uWant; ++uSeg)
        {
            const unsigned int uSegEnd = TraceEnd(kSegs[uSeg]);
            const bool bLast = uSeg == uWant - 1;
            const unsigned int uLimit = bLast ? uBaseEnd : uSegEnd;
            if (uLimit < kPerturbSimTurn)
            {
                iSplit = -1;
                break;
            }
            iSplit = CompareTraces(kBase, kSegs[uSeg], uLimit);
            if (iSplit == 0 && bLast && uSegEnd > uBaseEnd)
            {
                iSplit = (int)uBaseEnd + 1;
            }
            if (iSplit != 0)
            {
                break;
            }
        }
    }
    if (iSplit > 0)
    {
        sWhy = "split turn=" + std::to_string(iSplit) + " in trace segment " + std::to_string(uSeg + 1);
        return false;
    }
    if (iSplit < 0)
    {
        sWhy = "incomplete: the trace has " + std::to_string(kSegs.size()) + " segments, " + std::to_string(uWant) +
               " wanted, or misses a turn";
        return false;
    }
    const std::vector<std::string> kBaseDigests = MarkerLines(sBase, "Sim digest ");
    const std::vector<std::string> kDigests = MarkerLines(sRun, "Sim digest ");
    if (kBaseDigests.empty() || (bRepeat ? kDigests.size() != 2 : kDigests.empty()))
    {
        sWhy = "incomplete: " + std::to_string(kDigests.size()) + " SIM DIGEST lines";
        return false;
    }
    for (size_t i = 0; i < kDigests.size(); ++i)
    {
        if (kDigests[i] != kBaseDigests.back())
        {
            sWhy = "split turn=" + std::to_string(kPerturbSimTurn) + ": digest '" + kDigests[i] + "', base '" +
                   kBaseDigests.back() + "'";
            return false;
        }
    }
    return true;
}

std::vector<std::string> GrownPools(const std::string& sTable, const std::string& sMission)
{
    std::vector<std::string> kPools;
    const std::vector<std::string> kLines = LogLines(sTable);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::vector<std::string> kCols;
        size_t uStart = 0;
        for (;;)
        {
            const size_t uTab = kLines[i].find('\t', uStart);
            kCols.push_back(kLines[i].substr(uStart, uTab == std::string::npos ? std::string::npos : uTab - uStart));
            if (uTab == std::string::npos)
            {
                break;
            }
            uStart = uTab + 1;
        }
        if (kCols.size() == 10 && kCols[0] == sMission && kCols[2] == "pool" && kCols[8] != "0" && kCols[8] != "-")
        {
            kPools.push_back(kCols[3]);
        }
    }
    return kPools;
}

bool FramesCount(const std::string& sLog, long long& llFrames)
{
    const std::vector<std::string> kLines = MarkerLines(sLog, "Frames count=");
    std::smatch kMatch;
    if (kLines.size() != 1 || !std::regex_match(kLines[0], kMatch, std::regex("Frames count=(\\d{1,12})")))
    {
        return false;
    }
    llFrames = _strtoi64(kMatch[1].str().c_str(), 0, 10);
    return true;
}
