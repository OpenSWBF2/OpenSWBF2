#include "x87/x87_disasm.h"

#include <algorithm>
#include <deque>

#include "gate/gate_keys.h"
#include "hardening/hardening_dumpbin.h"
#include "hardening/hardening_regex.h"
#include "hardening/hardening_xml.h"

namespace X87
{

namespace
{

bool IsDotNetSpace(char c)
{
    return c == ' ' || (c >= '\t' && c <= '\r');
}

int HexValue(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    return -1;
}

bool ParseHex(const std::string& sText, size_t uAt, size_t uLen, long long& iOut)
{
    if (uLen == 0 || uAt + uLen > sText.size())
    {
        return false;
    }
    long long v = 0;
    for (size_t i = uAt; i < uAt + uLen; ++i)
    {
        const int d = HexValue(sText[i]);
        if (d < 0)
        {
            return false;
        }
        v = v * 16 + d;
    }
    iOut = v;
    return true;
}

long long LineAddress(const std::string& sLine)
{
    long long a = 0;
    ParseHex(sLine, 2, 8, a);
    return a;
}

std::string ParentDir(const std::string& sPath)
{
    const size_t u = sPath.find_last_of("\\/");
    return u == std::string::npos ? std::string() : sPath.substr(0, u);
}

std::string FileName(const std::string& sPath)
{
    const size_t u = sPath.find_last_of("\\/");
    return u == std::string::npos ? sPath : sPath.substr(u + 1);
}

std::string FileNameWithoutExtension(const std::string& sPath)
{
    const std::string sName = FileName(sPath);
    const size_t u = sName.find_last_of('.');
    return u == std::string::npos ? sName : sName.substr(0, u);
}

const Hardening::RegexPattern& MapPattern()
{
    static const Hardening::RegexPattern k(
        R"rx(^\s*0001:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{8})\s+(f?)\s*(?:i\s+)?(\S+)\s*$)rx", false);
    return k;
}

const Hardening::RegexPattern& CauseLinePattern()
{
    static const Hardening::RegexPattern k(R"rx(^- `[^`]+` \| (C1|C2|C3|C-X64-\S+) \| )rx", false);
    return k;
}

const Hardening::RegexPattern& CauseFilePattern()
{
    static const Hardening::RegexPattern k(R"rx([A-Za-z0-9_]+\.(?:cpp|c)\b)rx", false);
    return k;
}

const Hardening::RegexPattern& DataPattern()
{
    static const Hardening::RegexPattern k(R"rx(([0-9A-F]{6,8})h\])rx", false);
    return k;
}

bool PatternReady(const Hardening::RegexPattern& kPattern, std::string& sError)
{
    if (!kPattern.Valid())
    {
        sError = kPattern.Error();
        return false;
    }
    return true;
}

}

LineSplitter::LineSplitter() : mbCr(false)
{
}

void LineSplitter::Feed(const char* pcData, size_t uSize, const LineFn& fnLine)
{
    for (size_t i = 0; i < uSize; ++i)
    {
        const char c = pcData[i];
        if (mbCr)
        {
            mbCr = false;
            if (c == '\n')
            {
                continue;
            }
        }
        if (c == '\r' || c == '\n')
        {
            fnLine(msPending);
            msPending.clear();
            mbCr = c == '\r';
            continue;
        }
        msPending += c;
    }
}

void LineSplitter::End(const LineFn& fnLine)
{
    if (!msPending.empty())
    {
        fnLine(msPending);
    }
    msPending.clear();
    mbCr = false;
}

std::vector<std::string> TextLines(const std::string& sText)
{
    std::vector<std::string> kLines;
    LineSplitter kSplit;
    const LineFn fnAdd = [&kLines](const std::string& s) { kLines.push_back(s); };
    kSplit.Feed(sText.data(), sText.size(), fnAdd);
    kSplit.End(fnAdd);
    return kLines;
}

std::string TrimSpace(const std::string& sText)
{
    size_t b = 0;
    size_t e = sText.size();
    while (b < e && IsDotNetSpace(sText[b]))
    {
        ++b;
    }
    while (e > b && IsDotNetSpace(sText[e - 1]))
    {
        --e;
    }
    return sText.substr(b, e - b);
}

bool IsHex8Upper(const std::string& sText)
{
    if (sText.size() != 8)
    {
        return false;
    }
    for (size_t i = 0; i < 8; ++i)
    {
        const char c = sText[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')))
        {
            return false;
        }
    }
    return true;
}

std::string Hex8(long long iValue)
{
    char acBuf[32];
    _snprintf(acBuf, sizeof(acBuf), "%08llX", (unsigned long long)iValue);
    acBuf[sizeof(acBuf) - 1] = 0;
    return acBuf;
}

bool ReadMapObjects(const std::string& sText, MapObjects& kOut, std::string& sError)
{
    kOut.clear();
    if (!PatternReady(MapPattern(), sError))
    {
        return false;
    }
    const std::vector<std::string> kLines = TextLines(sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        Hardening::RegexMatcher m(MapPattern(), kLines[i]);
        if (!m.FindNext())
        {
            if (m.Failed())
            {
                sError = m.Error();
                return false;
            }
            continue;
        }
        long long a = 0;
        ParseHex(m.GetCaptureGroup(2), 0, 8, a);
        MapEntry e;
        e.sName = m.GetCaptureGroup(1);
        e.sObj = m.GetCaptureGroup(4);
        e.bFunc = m.GetCaptureGroup(3) == "f";
        kOut[(unsigned int)a].push_back(e);
    }
    return true;
}

bool FunctionObj(const MapObjects& kMap, long long iStart, const std::string& sName, std::string& sObj,
                 std::string& sError)
{
    if (iStart < 0 || iStart > 0xFFFFFFFFLL)
    {
        sError = "x87-inventory: function " + sName + " has no decoded instruction, so it has no map address";
        return false;
    }
    MapObjects::const_iterator it = kMap.find((unsigned int)iStart);
    if (it == kMap.end())
    {
        sObj = "?";
        return true;
    }
    const std::vector<MapEntry>& kList = it->second;
    for (size_t i = 0; i < kList.size(); ++i)
    {
        if (kList[i].sName == sName)
        {
            sObj = kList[i].sObj;
            return true;
        }
    }
    std::vector<std::string> kObjs;
    for (size_t i = 0; i < kList.size(); ++i)
    {
        kObjs.push_back(kList[i].sObj);
    }
    std::sort(kObjs.begin(), kObjs.end());
    sObj = kObjs[0];
    return true;
}

DisasmReader::DisasmReader(std::set<long long>* pBounds) : mpBounds(pBounds), mbOpen(false)
{
    mkCur.iStart = -1;
    mkCur.bHasF = false;
}

void DisasmReader::Close()
{
    if (mbOpen)
    {
        if (!mkCur.bHasF)
        {
            mkCur.kLines.clear();
        }
        mkFuncs.push_back(mkCur);
    }
    mbOpen = false;
    mkCur = DisasmFunc();
    mkCur.iStart = -1;
    mkCur.bHasF = false;
}

bool DisasmReader::Line(const std::string& sLine, std::string& sError)
{
    if (sLine.empty())
    {
        return true;
    }
    if (sLine[0] != ' ')
    {
        Close();
        if (sLine[sLine.size() - 1] == ':' && sLine.compare(0, 7, "SECTION") != 0)
        {
            mbOpen = true;
            mkCur.sName = sLine.substr(0, sLine.size() - 1);
        }
        return true;
    }
    if (!mbOpen || sLine.size() < 13 || sLine[10] != ':')
    {
        return true;
    }
    long long a = 0;
    if (!ParseHex(sLine, 2, 8, a))
    {
        sError = "x87-inventory: no instruction address in line: " + sLine;
        return false;
    }
    if (mpBounds)
    {
        mpBounds->insert(a);
    }
    if (mkCur.iStart < 0)
    {
        mkCur.iStart = a;
    }
    mkCur.kLines.push_back(sLine);
    if (sLine[12] == 'f')
    {
        mkCur.bHasF = true;
    }
    else if (sLine.size() > 24 && (sLine.compare(12, 5, "call ") == 0 || sLine.compare(12, 4, "jmp ") == 0))
    {
        const std::string t = TrimSpace(sLine.substr(24));
        if (!t.empty() && t.find('[') == std::string::npos && !IsHex8Upper(t))
        {
            mkCur.kCalls.push_back(t);
        }
    }
    return true;
}

void DisasmReader::Finish(std::vector<DisasmFunc>& kOut)
{
    Close();
    kOut.swap(mkFuncs);
    mkFuncs.clear();
}

bool ReadDisasmFunctions(const std::string& sText, std::set<long long>* pBounds, std::vector<DisasmFunc>& kOut,
                         std::string& sError)
{
    DisasmReader kReader(pBounds);
    const std::vector<std::string> kLines = TextLines(sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (!kReader.Line(kLines[i], sError))
        {
            return false;
        }
    }
    kReader.Finish(kOut);
    return true;
}

bool ItemCondition(const std::string& sCondition, bool& bIn, std::string& sError)
{
    bIn = true;
    if (sCondition.empty())
    {
        return true;
    }
    std::string c;
    for (size_t i = 0; i < sCondition.size(); ++i)
    {
        if (!IsDotNetSpace(sCondition[i]))
        {
            c += sCondition[i];
        }
    }
    if (c == "'$(Platform)'=='Win32'")
    {
        return true;
    }
    if (c == "'$(Platform)'=='x64'")
    {
        bIn = false;
        return true;
    }
    sError = "x87-inventory: unknown ClCompile condition " + sCondition;
    return false;
}

bool ProjectSources(const Hardening::BuildFiles& kFiles, const std::string& sRoot, const std::string& sProject,
                    SourceMap& kOut, std::string& sError)
{
    kOut.clear();
    std::string sRootTrim = sRoot;
    while (!sRootTrim.empty() && sRootTrim[sRootTrim.size() - 1] == '\\')
    {
        sRootTrim.erase(sRootTrim.size() - 1);
    }
    std::set<std::string> kSeen;
    std::deque<std::string> kQueue;
    kQueue.push_back(Gate::FullPath(sProject));
    while (!kQueue.empty())
    {
        const std::string p = kQueue.front();
        kQueue.pop_front();
        if (!kSeen.insert(Gate::LowerText(p)).second)
        {
            continue;
        }
        const std::string sDir = ParentDir(p);
        std::string sBytes;
        if (!kFiles.Read(p, sBytes))
        {
            sError = "x87-inventory: cannot read " + p;
            return false;
        }
        Hardening::XmlNode kRoot;
        if (!Hardening::ParseXml(Gate::DecodeText(std::vector<unsigned char>(sBytes.begin(), sBytes.end())), p, kRoot,
                                 sError))
        {
            return false;
        }
        for (size_t g = 0; g < kRoot.kChildren.size(); ++g)
        {
            const Hardening::XmlNode& kGroup = kRoot.kChildren[g];
            if (!kGroup.bElement || kGroup.sLocal != "ItemGroup")
            {
                continue;
            }
            bool bIn = true;
            if (!ItemCondition(Hardening::Attr(kGroup, "Condition"), bIn, sError))
            {
                return false;
            }
            if (!bIn)
            {
                continue;
            }
            for (size_t i = 0; i < kGroup.kChildren.size(); ++i)
            {
                const Hardening::XmlNode& kItem = kGroup.kChildren[i];
                if (!kItem.bElement)
                {
                    continue;
                }
                if (kItem.sLocal == "ClCompile")
                {
                    if (!ItemCondition(Hardening::Attr(kItem, "Condition"), bIn, sError))
                    {
                        return false;
                    }
                    if (!bIn)
                    {
                        continue;
                    }
                    const std::string sFull = Gate::FullPath(Gate::JoinPath(sDir, Hardening::Attr(kItem, "Include")));
                    if (sFull.size() <= sRootTrim.size() + 1)
                    {
                        sError = "x87-inventory: " + sFull + " is not under " + sRootTrim;
                        return false;
                    }
                    const std::string sRel = sFull.substr(sRootTrim.size() + 1);
                    const std::string sObj = FileNameWithoutExtension(sFull) + ".obj";
                    const std::string sKey = Gate::LowerText(sObj);
                    SourceMap::const_iterator it = kOut.find(sKey);
                    if (it != kOut.end() && it->second != sRel)
                    {
                        sError = "x87-inventory: " + sObj + " is built from " + it->second + " and " + sRel;
                        return false;
                    }
                    kOut[sKey] = sRel;
                }
                else if (kItem.sLocal == "ProjectReference")
                {
                    std::string sLink;
                    bool bLink = false;
                    for (size_t m = 0; m < kItem.kChildren.size(); ++m)
                    {
                        if (kItem.kChildren[m].bElement && kItem.kChildren[m].sLocal == "LinkLibraryDependencies")
                        {
                            sLink = TrimSpace(Hardening::InnerText(kItem.kChildren[m]));
                            bLink = true;
                        }
                    }
                    if (bLink && sLink == "true")
                    {
                        kQueue.push_back(Gate::FullPath(Gate::JoinPath(sDir, Hardening::Attr(kItem, "Include"))));
                    }
                }
            }
        }
    }
    return true;
}

bool CauseFiles(const std::string& sText, const SourceMap& kSources, std::set<std::string>& kOut, std::string& sError)
{
    kOut.clear();
    if (!PatternReady(CauseLinePattern(), sError) || !PatternReady(CauseFilePattern(), sError))
    {
        return false;
    }
    std::map<std::string, std::string> kByName;
    for (SourceMap::const_iterator it = kSources.begin(); it != kSources.end(); ++it)
    {
        kByName[Gate::LowerText(FileName(it->second))] = it->second;
    }
    const std::vector<std::string> kLines = TextLines(sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (!Hardening::RegexIsMatch(CauseLinePattern(), kLines[i], sError))
        {
            if (!sError.empty())
            {
                return false;
            }
            continue;
        }
        Hardening::RegexMatcher m(CauseFilePattern(), kLines[i]);
        while (m.FindNext())
        {
            const std::string sName = kLines[i].substr(m.GetMatchBeginning(), m.GetMatchLength());
            std::map<std::string, std::string>::const_iterator f = kByName.find(Gate::LowerText(sName));
            if (f != kByName.end())
            {
                kOut.insert(Gate::LowerText(f->second));
            }
        }
        if (m.Failed())
        {
            sError = m.Error();
            return false;
        }
    }
    return true;
}

bool ConvertFunction(const DisasmFunc& kF, long long iEnd, Function& kOut)
{
    long long iDataStart = iEnd;
    std::vector<Ins> kParsed;
    for (size_t i = 0; i < kF.kLines.size(); ++i)
    {
        const std::string& sLine = kF.kLines[i];
        const long long iAddr = LineAddress(sLine);
        if (iAddr >= iDataStart)
        {
            break;
        }
        const std::string sRest = sLine.substr(12);
        const size_t uSp = sRest.find(' ');
        Ins p;
        p.iA = iAddr;
        if (uSp == std::string::npos)
        {
            p.sM = sRest;
        }
        else
        {
            p.sM = sRest.substr(0, uSp);
            p.sO = TrimSpace(sRest.substr(uSp));
        }
        if (p.sM.empty() || p.sM[0] < 'a' || p.sM[0] > 'z')
        {
            continue;
        }
        if (p.sO.find("h]") != std::string::npos)
        {
            Hardening::RegexMatcher m(DataPattern(), p.sO);
            while (m.FindNext())
            {
                long long t = 0;
                const std::string sHex = m.GetCaptureGroup(1);
                ParseHex(sHex, 0, sHex.size(), t);
                if (t >= kF.iStart && t < iEnd && t < iDataStart)
                {
                    iDataStart = t;
                }
            }
        }
        kParsed.push_back(p);
    }
    kOut = Function();
    kOut.sName = kF.sName;
    kOut.iStart = kF.iStart;
    bool bHasF = false;
    for (size_t i = 0; i < kParsed.size(); ++i)
    {
        const Ins& p = kParsed[i];
        if (p.iA >= iDataStart)
        {
            break;
        }
        const char c0 = p.sM[0];
        if (c0 == 'f')
        {
            bHasF = true;
            kOut.kIns.push_back(p);
            continue;
        }
        if (p.sM == "call" || p.sM == "ret")
        {
            kOut.kIns.push_back(p);
            continue;
        }
        if (c0 == 'j' || p.sM.compare(0, 4, "loop") == 0)
        {
            if (IsHex8Upper(p.sO))
            {
                long long t = 0;
                ParseHex(p.sO, 0, 8, t);
                if (t >= kF.iStart && t < iDataStart)
                {
                    kOut.kLabels.insert(t);
                }
            }
            kOut.kIns.push_back(p);
        }
    }
    return bHasF;
}

bool AddLostFunctions(std::vector<DisasmFunc>& kFuncs, const MapObjects& kMap, const std::set<long long>& kBounds,
                      const DecodeFn& fnDecode, std::string& sError)
{
    std::vector<long long> kStarts;
    for (size_t i = 0; i < kFuncs.size(); ++i)
    {
        if (kFuncs[i].iStart >= 0)
        {
            kStarts.push_back(kFuncs[i].iStart);
        }
    }
    if (kStarts.empty() || kBounds.empty())
    {
        return true;
    }
    const long long iLo = kStarts[0];
    const long long iHi = *kBounds.rbegin();
    std::vector<long long> kAddrs;
    for (MapObjects::const_iterator it = kMap.begin(); it != kMap.end(); ++it)
    {
        bool bFunc = false;
        for (size_t i = 0; i < it->second.size(); ++i)
        {
            bFunc = bFunc || it->second[i].bFunc;
        }
        const long long a = (long long)it->first;
        if (bFunc && a >= iLo && a <= iHi)
        {
            kAddrs.push_back(a);
        }
    }
    std::vector<DisasmFunc> kAdded;
    for (size_t i = 0; i < kAddrs.size(); ++i)
    {
        const long long a = kAddrs[i];
        if (kBounds.count(a))
        {
            continue;
        }
        const std::vector<MapEntry>& kList = kMap.find((unsigned int)a)->second;
        std::string sName;
        for (size_t e = 0; e < kList.size(); ++e)
        {
            if (kList[e].bFunc)
            {
                sName = kList[e].sName;
                break;
            }
        }
        if (!fnDecode)
        {
            sError = "x87-inventory: map function " + sName + " at " + Hex8(a) +
                     " starts inside a decoded instruction, and -Disasm cannot decode it again";
            return false;
        }
        const long long iEnd = i + 1 < kAddrs.size() ? kAddrs[i + 1] : iHi + 1;
        std::vector<DisasmFunc> kDecoded;
        if (!fnDecode(a, iEnd - 1, kDecoded, sError))
        {
            return false;
        }
        std::vector<DisasmFunc> kGot;
        for (size_t g = 0; g < kDecoded.size(); ++g)
        {
            if (kDecoded[g].iStart >= 0)
            {
                kGot.push_back(kDecoded[g]);
            }
        }
        if (kGot.empty() || kGot[0].iStart != a)
        {
            sError = "x87-inventory: dumpbin did not decode map function " + sName + " at " + Hex8(a);
            return false;
        }
        for (size_t g = 0; g < kGot.size(); ++g)
        {
            if (kGot[g].iStart < iEnd)
            {
                kAdded.push_back(kGot[g]);
            }
        }
    }
    if (kAdded.empty())
    {
        return true;
    }
    std::vector<DisasmFunc> kAll;
    for (size_t i = 0; i < kFuncs.size(); ++i)
    {
        if (kFuncs[i].iStart >= 0)
        {
            kAll.push_back(kFuncs[i]);
        }
    }
    kAll.insert(kAll.end(), kAdded.begin(), kAdded.end());
    std::stable_sort(kAll.begin(), kAll.end(),
                     [](const DisasmFunc& a, const DisasmFunc& b) { return a.iStart < b.iStart; });
    kFuncs.swap(kAll);
    return true;
}

bool RunDisasm(const std::string& sPlatform, const std::vector<std::string>& kArgs, std::set<long long>* pBounds,
               std::vector<DisasmFunc>& kOut, std::string& sError)
{
    DisasmReader kReader(pBounds);
    LineSplitter kSplit;
    std::string sLineError;
    bool bLinesOk = true;
    const LineFn fnLine = [&](const std::string& sLine) {
        if (bLinesOk && !kReader.Line(sLine, sLineError))
        {
            bLinesOk = false;
        }
    };
    Hardening::DumpbinRun kRun;
    std::string sRunError;
    const bool bRan = Hardening::Dumpbin(sPlatform, kArgs, kRun, sRunError,
                                         [&](const char* pcData, size_t uSize) { kSplit.Feed(pcData, uSize, fnLine); });
    if (!bRan)
    {
        sError = "x87-inventory: " + sRunError;
        return false;
    }
    kSplit.End(fnLine);
    if (!bLinesOk)
    {
        sError = sLineError;
        return false;
    }
    kReader.Finish(kOut);
    if (kRun.uCode != 0)
    {
        sError = "x87-inventory: dumpbin exited " + std::to_string((long)(int)kRun.uCode) + ": " + kRun.sErr;
        return false;
    }
    return true;
}

}
