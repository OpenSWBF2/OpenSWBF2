#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include "gate/gate_evidence.h"

namespace Gate
{

const char* const kAsanCoverage = "ASan covers stack, globals, CRT heap and the game heaps and pools";

namespace
{

struct JsonValue
{
    char cType;
    std::string sText;
    double fNumber;
    std::vector<JsonValue> kItems;
    std::vector<std::pair<std::string, JsonValue> > kMembers;
};

struct JsonReader
{
    const std::string& sText;
    size_t uAt;
    explicit JsonReader(const std::string& s) : sText(s), uAt(0)
    {
    }

    void Space()
    {
        while (uAt < sText.size() &&
               (sText[uAt] == ' ' || sText[uAt] == '\t' || sText[uAt] == '\n' || sText[uAt] == '\r'))
        {
            ++uAt;
        }
    }

    bool Hex4(unsigned int& uOut)
    {
        if (uAt + 4 > sText.size())
        {
            return false;
        }
        uOut = 0;
        for (size_t i = 0; i < 4; ++i)
        {
            const char c = sText[uAt + i];
            uOut <<= 4;
            if (c >= '0' && c <= '9')
            {
                uOut |= (unsigned int)(c - '0');
            }
            else if (c >= 'a' && c <= 'f')
            {
                uOut |= (unsigned int)(c - 'a' + 10);
            }
            else if (c >= 'A' && c <= 'F')
            {
                uOut |= (unsigned int)(c - 'A' + 10);
            }
            else
            {
                return false;
            }
        }
        uAt += 4;
        return true;
    }

    bool String(std::string& sOut)
    {
        if (uAt >= sText.size() || sText[uAt] != '"')
        {
            return false;
        }
        ++uAt;
        std::wstring sWide;
        while (uAt < sText.size())
        {
            const unsigned char c = (unsigned char)sText[uAt];
            if (c == '"')
            {
                ++uAt;
                sOut += WideToUtf8(sWide);
                return true;
            }
            if (c < 0x20)
            {
                return false;
            }
            if (c != '\\')
            {
                if (!sWide.empty())
                {
                    sOut += WideToUtf8(sWide);
                    sWide.clear();
                }
                sOut += (char)c;
                ++uAt;
                continue;
            }
            if (++uAt >= sText.size())
            {
                return false;
            }
            const char e = sText[uAt++];
            unsigned int uCode = 0;
            switch (e)
            {
            case '"':
            case '\\':
            case '/':
                sWide += (wchar_t)e;
                break;
            case 'b':
                sWide += L'\b';
                break;
            case 'f':
                sWide += L'\f';
                break;
            case 'n':
                sWide += L'\n';
                break;
            case 'r':
                sWide += L'\r';
                break;
            case 't':
                sWide += L'\t';
                break;
            case 'u':
                if (!Hex4(uCode))
                {
                    return false;
                }
                sWide += (wchar_t)uCode;
                break;
            default:
                return false;
            }
        }
        return false;
    }

    bool Number(JsonValue& k)
    {
        const size_t uStart = uAt;
        if (uAt < sText.size() && sText[uAt] == '-')
        {
            ++uAt;
        }
        if (uAt >= sText.size() || !isdigit((unsigned char)sText[uAt]))
        {
            return false;
        }
        if (sText[uAt] == '0')
        {
            ++uAt;
        }
        else
        {
            while (uAt < sText.size() && isdigit((unsigned char)sText[uAt]))
            {
                ++uAt;
            }
        }
        if (uAt < sText.size() && sText[uAt] == '.')
        {
            ++uAt;
            if (uAt >= sText.size() || !isdigit((unsigned char)sText[uAt]))
            {
                return false;
            }
            while (uAt < sText.size() && isdigit((unsigned char)sText[uAt]))
            {
                ++uAt;
            }
        }
        if (uAt < sText.size() && (sText[uAt] == 'e' || sText[uAt] == 'E'))
        {
            ++uAt;
            if (uAt < sText.size() && (sText[uAt] == '+' || sText[uAt] == '-'))
            {
                ++uAt;
            }
            if (uAt >= sText.size() || !isdigit((unsigned char)sText[uAt]))
            {
                return false;
            }
            while (uAt < sText.size() && isdigit((unsigned char)sText[uAt]))
            {
                ++uAt;
            }
        }
        k.cType = 'n';
        k.sText = sText.substr(uStart, uAt - uStart);
        k.fNumber = atof(k.sText.c_str());
        return true;
    }

    bool Word(const char* pcWord)
    {
        const size_t uLen = strlen(pcWord);
        if (sText.compare(uAt, uLen, pcWord) != 0)
        {
            return false;
        }
        uAt += uLen;
        return true;
    }

    bool Value(JsonValue& k, int iDepth)
    {
        if (iDepth > 64)
        {
            return false;
        }
        Space();
        if (uAt >= sText.size())
        {
            return false;
        }
        const char c = sText[uAt];
        if (c == '{')
        {
            k.cType = 'o';
            ++uAt;
            Space();
            if (uAt < sText.size() && sText[uAt] == '}')
            {
                ++uAt;
                return true;
            }
            for (;;)
            {
                Space();
                std::string sName;
                if (!String(sName))
                {
                    return false;
                }
                Space();
                if (uAt >= sText.size() || sText[uAt] != ':')
                {
                    return false;
                }
                ++uAt;
                k.kMembers.push_back(std::make_pair(sName, JsonValue()));
                if (!Value(k.kMembers.back().second, iDepth + 1))
                {
                    return false;
                }
                Space();
                if (uAt < sText.size() && sText[uAt] == ',')
                {
                    ++uAt;
                    continue;
                }
                if (uAt < sText.size() && sText[uAt] == '}')
                {
                    ++uAt;
                    return true;
                }
                return false;
            }
        }
        if (c == '[')
        {
            k.cType = 'a';
            ++uAt;
            Space();
            if (uAt < sText.size() && sText[uAt] == ']')
            {
                ++uAt;
                return true;
            }
            for (;;)
            {
                k.kItems.push_back(JsonValue());
                if (!Value(k.kItems.back(), iDepth + 1))
                {
                    return false;
                }
                Space();
                if (uAt < sText.size() && sText[uAt] == ',')
                {
                    ++uAt;
                    continue;
                }
                if (uAt < sText.size() && sText[uAt] == ']')
                {
                    ++uAt;
                    return true;
                }
                return false;
            }
        }
        if (c == '"')
        {
            k.cType = 's';
            return String(k.sText);
        }
        if (c == 't')
        {
            k.cType = 't';
            return Word("true");
        }
        if (c == 'f')
        {
            k.cType = 'f';
            return Word("false");
        }
        if (c == 'n' && sText.compare(uAt, 4, "null") == 0)
        {
            k.cType = 'z';
            return Word("null");
        }
        return Number(k);
    }
};

const JsonValue* Member(const JsonValue& k, const char* pcName)
{
    for (size_t i = 0; i < k.kMembers.size(); ++i)
    {
        if (k.kMembers[i].first == pcName)
        {
            return &k.kMembers[i].second;
        }
    }
    return 0;
}

void HashLines(const std::string& sDir, const std::string& sRel, std::vector<std::string>& kLines)
{
    WIN32_FIND_DATAW kData;
    HANDLE hFind = FindFirstFileExW(Utf8ToWide(JoinPath(sDir, "*")).c_str(), FindExInfoBasic, &kData,
                                    FindExSearchNameMatch, NULL, 0);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        if (wcscmp(kData.cFileName, L".") == 0 || wcscmp(kData.cFileName, L"..") == 0)
        {
            continue;
        }
        const std::string sName = WideToUtf8(kData.cFileName);
        const std::string sPath = JoinPath(sDir, sName);
        const std::string sChild = sRel.empty() ? sName : sRel + "\\" + sName;
        if (kData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!(kData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            {
                HashLines(sPath, sChild, kLines);
            }
            continue;
        }
        std::string sHash;
        FileSha(sPath, sHash);
        kLines.push_back(sChild + "\t" + sHash);
    } while (FindNextFileW(hFind, &kData));
    FindClose(hFind);
}

bool LineStartsWith(const std::string& sText, const std::string& sPrefix, std::string& sMatch)
{
    size_t uAt = sText.find(sPrefix);
    while (uAt != std::string::npos)
    {
        if (uAt == 0 || sText[uAt - 1] == '\n')
        {
            sMatch = sPrefix;
            return true;
        }
        uAt = sText.find(sPrefix, uAt + 1);
    }
    return false;
}

bool VerdictToken(const std::string& sLine, const char* const* ppcTokens, size_t uCount, std::string& sToken)
{
    for (size_t i = 0; i < uCount; ++i)
    {
        const std::string sHead = std::string("[") + ppcTokens[i] + "] ";
        if (sLine.compare(0, sHead.size(), sHead) == 0)
        {
            sToken = ppcTokens[i];
            return true;
        }
    }
    return false;
}

std::string NewStageName()
{
    GUID kGuid;
    CoCreateGuid(&kGuid);
    char acName[64];
    _snprintf(acName, sizeof(acName), "stage-%08lx%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x", kGuid.Data1,
              kGuid.Data2, kGuid.Data3, kGuid.Data4[0], kGuid.Data4[1], kGuid.Data4[2], kGuid.Data4[3],
              kGuid.Data4[4], kGuid.Data4[5], kGuid.Data4[6], kGuid.Data4[7]);
    acName[sizeof(acName) - 1] = 0;
    return acName;
}

std::string CountText(size_t uCount)
{
    char acText[32];
    _snprintf(acText, sizeof(acText), "%u", (unsigned int)uCount);
    acText[sizeof(acText) - 1] = 0;
    return acText;
}

std::string FaultCount(HostFaultCountFn pCount, bool bSince, unsigned long long uStart)
{
    long long iCount = 0;
    if (!pCount(bSince, uStart, iCount))
    {
        return "unavailable";
    }
    char acText[32];
    _snprintf(acText, sizeof(acText), "%lld", iCount);
    acText[sizeof(acText) - 1] = 0;
    return acText;
}

bool FailStage(const std::string& sStage, const std::string& sWhy, std::string& sError)
{
    std::string sIgnored;
    if (DirExists(sStage))
    {
        RemoveTree(sStage, sIgnored);
    }
    sError = sWhy;
    return false;
}

}

bool ReadIndexJson(const std::string& sText, IndexJson& kIndex, std::string& sError)
{
    kIndex.bHasFailed = false;
    kIndex.fFailed = 0;
    kIndex.sFailed.clear();
    kIndex.bHasTests = false;
    kIndex.kTests.clear();
    JsonReader kReader(sText);
    JsonValue kRoot;
    if (!kReader.Value(kRoot, 0))
    {
        sError = "does not parse";
        return false;
    }
    kReader.Space();
    if (kReader.uAt != sText.size() || kRoot.cType != 'o')
    {
        sError = "does not parse";
        return false;
    }
    const JsonValue* pFailed = Member(kRoot, "failed");
    if (pFailed && pFailed->cType == 'n')
    {
        kIndex.bHasFailed = true;
        kIndex.sFailed = pFailed->sText;
        kIndex.fFailed = pFailed->fNumber;
    }
    const JsonValue* pTests = Member(kRoot, "tests");
    if (pTests && pTests->cType == 'a')
    {
        kIndex.bHasTests = true;
        for (size_t i = 0; i < pTests->kItems.size(); ++i)
        {
            const JsonValue& kTest = pTests->kItems[i];
            IndexTest k;
            const JsonValue* pPath = Member(kTest, "fullTestPath");
            const JsonValue* pState = Member(kTest, "state");
            k.sFullPath = pPath && pPath->cType == 's' ? pPath->sText : std::string();
            k.sState = pState && pState->cType == 's' ? pState->sText : std::string();
            kIndex.kTests.push_back(k);
        }
    }
    return true;
}

std::string EvidenceHash(const std::string& sDir)
{
    if (sDir.empty() || !DirExists(sDir))
    {
        return "-";
    }
    std::string sFull = FullPath(sDir);
    while (!sFull.empty() && sFull[sFull.size() - 1] == '\\')
    {
        sFull.erase(sFull.size() - 1);
    }
    std::vector<std::string> kLines;
    HashLines(sFull, std::string(), kLines);
    if (kLines.empty())
    {
        return "-";
    }
    std::sort(kLines.begin(), kLines.end());
    return TextSha(JoinText(kLines, "\n"));
}

bool IsEvidenceName(const std::string& sHash)
{
    if (sHash.size() != 64)
    {
        return false;
    }
    for (size_t i = 0; i < sHash.size(); ++i)
    {
        const char c = sHash[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
        {
            return false;
        }
    }
    return true;
}

std::string EvidenceIntact(const std::string& sEvidenceRoot, const std::string& sHash)
{
    if (!IsEvidenceName(sHash))
    {
        return "record names no evidence (" + sHash + ")";
    }
    const std::string sDir = JoinPath(sEvidenceRoot, sHash);
    if (!DirExists(sDir))
    {
        return "evidence " + sHash + " is missing";
    }
    if (EvidenceHash(sDir) != sHash)
    {
        return "evidence " + sHash + " no longer hashes to its name";
    }
    return std::string();
}

bool SaveEvidence(const std::string& sEvidenceRoot, const std::string& sWork, unsigned long uCode, bool bRun,
                  const KeyPairs& kRuns, const std::vector<std::string>& kOutputs, std::string& sHash,
                  std::string& sError)
{
    const std::string sEv = JoinPath(sWork, "evidence");
    char acCode[32];
    _snprintf(acCode, sizeof(acCode), "%ld\r\n", (long)uCode);
    acCode[sizeof(acCode) - 1] = 0;
    if (!MakeDirs(sEv) || !WriteTextFile(JoinPath(sEv, "exit.txt"), acCode) || !MakeDirs(sEvidenceRoot))
    {
        sError = "cannot write the evidence of " + sWork;
        return false;
    }
    const std::string sStage = JoinPath(sEvidenceRoot, NewStageName());
    std::string sWhy;
    if (!CopyTree(sEv, sStage, sWhy))
    {
        return FailStage(sStage, sWhy, sError);
    }
    if (bRun)
    {
        std::vector<std::string> kPaths;
        for (size_t i = 0; i < kRuns.size(); ++i)
        {
            std::vector<std::string> kFound;
            if (!FindReparsePoints(kRuns[i].second, kFound, sWhy) || !kFound.empty())
            {
                return FailStage(sStage, "run folder " + kRuns[i].second + " holds a reparse point", sError);
            }
            if (!CopyTree(kRuns[i].second, JoinPath(sStage, "runs\\" + kRuns[i].first), sWhy))
            {
                return FailStage(sStage, sWhy, sError);
            }
            kPaths.push_back(kRuns[i].second);
        }
        WriteTextFile(JoinPath(sWork, "runs.txt"), JoinText(kPaths, "\r\n") + "\r\n");
    }
    for (size_t i = 0; i < kOutputs.size(); ++i)
    {
        if (!DirExists(kOutputs[i]))
        {
            continue;
        }
        std::vector<std::string> kFound;
        if (!FindReparsePoints(kOutputs[i], kFound, sWhy) || !kFound.empty())
        {
            return FailStage(sStage, "output folder " + kOutputs[i] + " holds a reparse point", sError);
        }
        if (!CopyTree(kOutputs[i], JoinPath(sStage, "out\\" + CountText(i)), sWhy))
        {
            return FailStage(sStage, sWhy, sError);
        }
    }
    sHash = EvidenceHash(sStage);
    const std::string sDest = JoinPath(sEvidenceRoot, sHash);
    if (DirExists(sDest) && EvidenceHash(sDest) != sHash && !RemoveTree(sDest, sWhy))
    {
        return FailStage(sStage, sWhy, sError);
    }
    if (!DirExists(sDest) && !MoveFileExW(Utf8ToWide(sStage).c_str(), Utf8ToWide(sDest).c_str(), 0))
    {
        return FailStage(sStage, "cannot move " + sStage + " to " + sDest, sError);
    }
    if (DirExists(sStage))
    {
        RemoveTree(sStage, sWhy);
    }
    if (EvidenceHash(sDest) != sHash)
    {
        sError = "evidence " + sDest + " does not hash to " + sHash + " after the copy";
        return false;
    }
    return true;
}

std::string JudgeRunRecord(const std::string& sDir, const JudgeSpec& kSpec)
{
    std::string sResults;
    if (!FileExists(JoinPath(sDir, "results.txt")) || !ReadTextFile(JoinPath(sDir, "results.txt"), sResults))
    {
        return "no results.txt";
    }
    const std::vector<std::string> kLines = SplitLines(sResults);
    static const char* const kBad[] = {"FAIL", "STALL", "CRASH"};
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::string sToken;
        if (VerdictToken(kLines[i], kBad, 3, sToken))
        {
            return "results.txt has a failure line: " + kLines[i];
        }
    }
    const char* const kIndexes[] = {"index.json", "unit\\index.json"};
    for (size_t r = 0; r < 2; ++r)
    {
        const std::string sFile = JoinPath(sDir, kIndexes[r]);
        if (!FileExists(sFile))
        {
            if (r == 0 && kSpec.bDirect)
            {
                return "no index.json";
            }
            continue;
        }
        std::string sText;
        IndexJson kIndex;
        std::string sError;
        if (!ReadTextFile(sFile, sText) || !ReadIndexJson(sText, kIndex, sError))
        {
            return std::string(kIndexes[r]) + " does not parse";
        }
        if (!kIndex.bHasFailed || !kIndex.bHasTests)
        {
            return std::string(kIndexes[r]) + " has no failed count or test list";
        }
        if ((int)kIndex.fFailed != 0)
        {
            return std::string(kIndexes[r]) + " counts " + kIndex.sFailed + " failed";
        }
        for (size_t i = 0; i < kIndex.kTests.size(); ++i)
        {
            if (kIndex.kTests[i].sState == "Fail")
            {
                return std::string(kIndexes[r]) + " test " + kIndex.kTests[i].sFullPath + " is Fail";
            }
        }
        if (r == 0 && kSpec.bDirect)
        {
            if (kIndex.kTests.empty())
            {
                return "index.json lists no test";
            }
            for (size_t m = 0; m < kSpec.kMustRun.size(); ++m)
            {
                size_t uSeen = 0;
                std::string sState;
                for (size_t i = 0; i < kIndex.kTests.size(); ++i)
                {
                    if (kIndex.kTests[i].sFullPath == kSpec.kMustRun[m])
                    {
                        ++uSeen;
                        sState = kIndex.kTests[i].sState;
                    }
                }
                if (uSeen != 1)
                {
                    return "index.json runs " + kSpec.kMustRun[m] + " " + CountText(uSeen) + " times";
                }
                if (sState != "Success")
                {
                    return "index.json runs " + kSpec.kMustRun[m] + " with state " + sState;
                }
            }
        }
    }
    return std::string();
}

std::string JudgeEvidenceFolder(const std::string& sDir, const JudgeSpec& kSpec)
{
    std::string sExit;
    if (!FileExists(JoinPath(sDir, "exit.txt")) || !ReadTextFile(JoinPath(sDir, "exit.txt"), sExit))
    {
        return "no exit.txt";
    }
    const std::string sCode = TrimText(sExit);
    if (sCode != "0")
    {
        return "exit.txt is " + sCode;
    }
    std::string sStdout;
    if (!FileExists(JoinPath(sDir, "stdout.txt")) || !ReadTextFile(JoinPath(sDir, "stdout.txt"), sStdout))
    {
        return "no stdout.txt";
    }
    std::string sMatch;
    if ((kSpec.sKind == "script" && LineStartsWith(sStdout, "FAILED", sMatch)) ||
        (kSpec.sKind == "capture" && LineStartsWith(sStdout, "[FAIL] ", sMatch)))
    {
        return "stdout.txt has a failure line: " + sMatch;
    }
    if (kSpec.sKind == "run")
    {
        const std::string sRuns = JoinPath(sDir, "runs");
        std::vector<std::string> kNames;
        WIN32_FIND_DATAW kData;
        HANDLE hFind = FindFirstFileExW(Utf8ToWide(JoinPath(sRuns, "*")).c_str(), FindExInfoBasic, &kData,
                                        FindExSearchNameMatch, NULL, 0);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((kData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(kData.cFileName, L".") != 0 &&
                    wcscmp(kData.cFileName, L"..") != 0)
                {
                    kNames.push_back(WideToUtf8(kData.cFileName));
                }
            } while (FindNextFileW(hFind, &kData));
            FindClose(hFind);
        }
        if (kNames.empty())
        {
            return "no archived run folder";
        }
        for (size_t i = 0; i < kNames.size(); ++i)
        {
            const std::string sWhy = JudgeRunRecord(JoinPath(sRuns, kNames[i]), kSpec);
            if (!sWhy.empty())
            {
                return "run " + kNames[i] + ": " + sWhy;
            }
        }
    }
    return std::string();
}

std::string JudgeArchived(const std::string& sEvidenceRoot, const std::string& sHash, const std::string& sCopy,
                          const JudgeSpec& kSpec)
{
    std::string sError;
    if (!CopyTree(JoinPath(sEvidenceRoot, sHash), sCopy, sError))
    {
        return "evidence could not be copied: " + sError;
    }
    if (EvidenceHash(sCopy) != sHash)
    {
        return "evidence changed while it was copied";
    }
    return JudgeEvidenceFolder(sCopy, kSpec);
}

std::vector<std::string> VerdictLines(const std::string& sOutput)
{
    static const char* const kTokens[] = {"PASS", "FAIL", "SKIP", "STALL", "CRASH", "N/A"};
    std::vector<std::string> kOut;
    const std::vector<std::string> kLines = SplitLines(sOutput);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::string sToken;
        if (VerdictToken(kLines[i], kTokens, 6, sToken))
        {
            kOut.push_back(kLines[i]);
        }
    }
    return kOut;
}

std::string LinesText(const std::vector<std::string>& kLines)
{
    std::string sOut;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        sOut += kLines[i] + "\r\n";
    }
    return sOut;
}

std::string ResultsText(const std::vector<std::string>& kLines, bool bAsan, const std::string& sNote)
{
    static const char* const kFinal[] = {"PASS", "FAIL", "SKIP", "STALL", "CRASH"};
    size_t auCount[5] = {0, 0, 0, 0, 0};
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        std::string sToken = "SKIP";
        VerdictToken(kLines[i], kFinal, 5, sToken);
        for (size_t t = 0; t < 5; ++t)
        {
            if (sToken == kFinal[t])
            {
                ++auCount[t];
            }
        }
    }
    std::vector<std::string> kOut = kLines;
    kOut.push_back("");
    kOut.push_back("Results");
    for (size_t t = 0; t < 5; ++t)
    {
        char acLine[64];
        _snprintf(acLine, sizeof(acLine), "  %-8s %u", kFinal[t], (unsigned int)auCount[t]);
        acLine[sizeof(acLine) - 1] = 0;
        kOut.push_back(acLine);
    }
    if (bAsan)
    {
        kOut.push_back(kAsanCoverage);
    }
    if (!sNote.empty())
    {
        kOut.push_back(sNote);
    }
    return LinesText(kOut);
}

std::vector<std::string> RunParamsLines(const RunParamsInput& kIn)
{
    std::vector<std::string> k;
    for (size_t i = 0; i < kIn.kArgs.size(); ++i)
    {
        k.push_back("arg=" + kIn.kArgs[i]);
    }
    k.push_back("platform=" + kIn.sPlatform);
    k.push_back("configuration=" + kIn.sConfiguration);
    k.push_back("smoke_configuration=" + kIn.sSmokeConfiguration);
    k.push_back(std::string("wer_localdumps=") + (kIn.bWerLocalDumps ? "1" : "0"));
    k.push_back("steam=" + kIn.sSteam);
    k.push_back("tests_exe=" + kIn.sTestsExe);
    for (size_t i = 0; i < kIn.kExes.size(); ++i)
    {
        k.push_back("exe." + kIn.kExes[i].first + "=" + kIn.kExes[i].second);
    }
    for (size_t i = 0; i < kIn.kDlls.size(); ++i)
    {
        k.push_back("dll." + kIn.kDlls[i].first + "=" + kIn.kDlls[i].second);
    }
    return k;
}

bool HostFaultLines(const std::string& sStage, HostFaultCountFn pCount, unsigned long long uStart,
                    std::vector<std::string>& kLines)
{
    if (sStage == "start")
    {
        kLines.push_back("whea_before=" + FaultCount(pCount, false, 0));
        return true;
    }
    if (sStage == "end")
    {
        kLines.push_back("whea_after=" + FaultCount(pCount, false, 0));
        kLines.push_back("whea_delta=" + FaultCount(pCount, true, uStart));
        return true;
    }
    return false;
}

std::map<std::string, std::string> ReadKeyValueText(const std::string& sText)
{
    std::map<std::string, std::string> k;
    const std::vector<std::string> kLines = SplitLines(sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const size_t uEq = kLines[i].find('=');
        if (uEq == std::string::npos || uEq < 1)
        {
            continue;
        }
        k[kLines[i].substr(0, uEq)] = kLines[i].substr(uEq + 1);
    }
    return k;
}

std::string HostFaultNote(const std::map<std::string, std::string>& kParams, int iBad)
{
    if (iBad <= 0)
    {
        return std::string();
    }
    std::map<std::string, std::string>::const_iterator it = kParams.find("whea_delta");
    const std::string sDelta = it != kParams.end() ? it->second : std::string();
    if (sDelta.empty() || sDelta.find_first_not_of("0123456789") != std::string::npos ||
        sDelta.find_first_not_of('0') == std::string::npos)
    {
        return std::string();
    }
    std::map<std::string, std::string>::const_iterator b = kParams.find("whea_before");
    std::map<std::string, std::string>::const_iterator a = kParams.find("whea_after");
    return "Suspected hardware: " + sDelta +
           " host fault event(s) (WHEA-Logger, Kernel-Power 41, EventLog 6008) in this run window, whea_before=" +
           (b != kParams.end() ? b->second : std::string()) +
           " whea_after=" + (a != kParams.end() ? a->second : std::string()) + "; the failures above stay FAIL";
}

}
