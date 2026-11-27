#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <set>

#include "gate/gate_keys.h"
#include "gate/gate_machine.h"
#include "hardening/hardening_binary.h"
#include "hardening/hardening_dumpbin.h"
#include "hardening/hardening_regex.h"
#include "hardening/hardening_xml.h"
#include "tables/tables_readers.h"

namespace Hardening
{

namespace
{

const char* const kDelayHeader = "Section contains the following delay load imports:";
const char* const kCrtTrigHelper = "pcCrtTrigX64.obj";
const char* const kFFmpegNoticeSource = "https://github.com/OpenSWBF2/OpenSWBF2 under src/FFmpegFL";
const char* const kFFmpegChangesNeed[] = {"+++ b/openswbf2/ossf2_alloc.c", "+++ b/openswbf2-ffmpeg.def"};
const char* const kFFmpegWd4703[] = {"libavutil\\encryption_info.c", "libavformat\\url.c", "libavformat\\av1.c",
                                     "libavformat\\avio.c", "libavcodec\\raw.c"};
const char* const kFFmpegDebugO1[] = {"libavcodec\\avcodec.c"};
const char* const kFFmpegImportAllow[] = {"kernel32.dll",
                                          "vcruntime140.dll",
                                          "api-ms-win-crt-runtime-l1-1-0.dll",
                                          "api-ms-win-crt-math-l1-1-0.dll",
                                          "api-ms-win-crt-stdio-l1-1-0.dll",
                                          "api-ms-win-crt-string-l1-1-0.dll",
                                          "api-ms-win-crt-time-l1-1-0.dll",
                                          "api-ms-win-crt-convert-l1-1-0.dll",
                                          "api-ms-win-crt-environment-l1-1-0.dll",
                                          "api-ms-win-crt-utility-l1-1-0.dll"};
const char* const kFFmpegCrtHeapModule = "api-ms-win-crt-heap-l1-1-0.dll";
const char* const kFFmpegCrtHeapNameRx = "^(?:malloc|realloc|free|calloc|_aligned_\\w+)$";
const char* const kCrtFloatImports[] = {"sinf", "cosf",  "tanf",  "asinf",  "acosf", "atanf",
                                        "atan2f", "expf", "logf", "log10f", "powf",  "fmodf"};
const char* const kD3dxImportsAllow[] = {"D3DXCheckTextureRequirements", "D3DXLoadSurfaceFromSurface"};
const char* const kDpiImports[] = {"SetProcessDpiAwarenessContext", "SetProcessDpiAwareness", "SetProcessDPIAware",
                                   "GetDpiForWindow", "AdjustWindowRectExForDpi"};
const char* const kFma3OffFunctions[] = {"Startup_RedInitHeap", "?PcMathStartup@@YAXXZ"};

template <size_t N>
std::vector<std::string> NameList(const char* const (&apcItems)[N])
{
    return std::vector<std::string>(apcItems, apcItems + N);
}

bool IsSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

std::string Trim(const std::string& sText)
{
    size_t uStart = 0;
    size_t uEnd = sText.size();
    while (uStart < uEnd && IsSpace(sText[uStart]))
    {
        ++uStart;
    }
    while (uEnd > uStart && IsSpace(sText[uEnd - 1]))
    {
        --uEnd;
    }
    return sText.substr(uStart, uEnd - uStart);
}

std::vector<std::string> SplitOn(const std::string& sText, char cSep)
{
    std::vector<std::string> kOut;
    size_t uFrom = 0;
    for (;;)
    {
        const size_t uAt = sText.find(cSep, uFrom);
        kOut.push_back(sText.substr(uFrom, uAt == std::string::npos ? std::string::npos : uAt - uFrom));
        if (uAt == std::string::npos)
        {
            return kOut;
        }
        uFrom = uAt + 1;
    }
}

std::vector<std::string> SplitCrLf(const std::string& sText)
{
    std::vector<std::string> kOut = SplitOn(sText, '\n');
    for (size_t i = 0; i < kOut.size(); ++i)
    {
        if (!kOut[i].empty() && kOut[i][kOut[i].size() - 1] == '\r')
        {
            kOut[i].erase(kOut[i].size() - 1);
        }
    }
    return kOut;
}

std::string Join(const std::vector<std::string>& kItems, const std::string& sSep)
{
    return Gate::JoinText(kItems, sSep);
}

std::vector<std::string> Sorted(std::vector<std::string> kItems)
{
    Tables::CultureSort(kItems, true);
    return kItems;
}

std::vector<std::string> SortUnique(std::vector<std::string> kItems)
{
    Tables::CultureSort(kItems, true);
    std::vector<std::string> kOut;
    for (size_t i = 0; i < kItems.size(); ++i)
    {
        if (kOut.empty() || Tables::CultureCompare(kOut.back(), kItems[i], true) != 0)
        {
            kOut.push_back(kItems[i]);
        }
    }
    return kOut;
}

bool ContainsNoCase(const std::vector<std::string>& kItems, const std::string& sItem)
{
    for (size_t i = 0; i < kItems.size(); ++i)
    {
        if (Gate::EqualNoCase(kItems[i], sItem))
        {
            return true;
        }
    }
    return false;
}

bool ContainsCase(const std::vector<std::string>& kItems, const std::string& sItem)
{
    return std::find(kItems.begin(), kItems.end(), sItem) != kItems.end();
}

bool EndsWithNoCase(const std::string& sText, const std::string& sTail)
{
    return sText.size() >= sTail.size() && Gate::EqualNoCase(sText.substr(sText.size() - sTail.size()), sTail);
}

std::string LowerText(const std::string& sText)
{
    return Gate::LowerText(sText);
}

std::string Hex(unsigned long long uValue, int iWidth)
{
    char acBuf[32];
    snprintf(acBuf, sizeof(acBuf), "%0*llX", iWidth, uValue);
    return acBuf;
}

std::string BoolText(bool b)
{
    return b ? "True" : "False";
}

std::string FileName(const std::string& sPath)
{
    const size_t uAt = sPath.find_last_of("\\/");
    return uAt == std::string::npos ? sPath : sPath.substr(uAt + 1);
}

std::string ParentDir(const std::string& sPath)
{
    const size_t uAt = sPath.find_last_of("\\/");
    return uAt == std::string::npos ? std::string() : sPath.substr(0, uAt);
}

std::string BaseName(const std::string& sPath)
{
    const std::string sName = FileName(sPath);
    const size_t uDot = sName.find_last_of('.');
    return uDot == std::string::npos ? sName : sName.substr(0, uDot);
}

std::string LeafName(const std::string& sPath)
{
    std::string s = sPath;
    while (!s.empty() && (s[s.size() - 1] == '\\' || s[s.size() - 1] == '/'))
    {
        s.erase(s.size() - 1);
    }
    return FileName(s);
}

std::string StripBom(const std::string& sLine)
{
    std::string s = sLine;
    while (s.compare(0, 3, "\xEF\xBB\xBF") == 0)
    {
        s.erase(0, 3);
    }
    return s;
}

std::string DecodeBytes(const std::string& sBytes)
{
    return Gate::DecodeText(std::vector<unsigned char>(sBytes.begin(), sBytes.end()));
}

std::string RegexEscape(const std::string& sText)
{
    std::string sOut;
    for (size_t i = 0; i < sText.size(); ++i)
    {
        if (strchr("\\*+?|{[()^$.# \t\n\r\f\v", sText[i]))
        {
            sOut += '\\';
        }
        sOut += sText[i];
    }
    return sOut;
}

BinaryRow Row(const std::string& sName, bool bOk, const std::string& sDetail)
{
    BinaryRow k;
    k.sName = sName;
    k.eStatus = bOk ? kRowPass : kRowFail;
    k.sDetail = sDetail;
    return k;
}

BinaryRow SkipRow(const std::string& sName, const std::string& sWhy)
{
    BinaryRow k;
    k.sName = sName;
    k.eStatus = kRowSkip;
    k.sDetail = sWhy;
    return k;
}

class Scan
{
public:
    explicit Scan(std::string& sError) : msError(sError)
    {
    }

    bool Has(const RegexPattern& kPattern, const std::string& sText)
    {
        RegexMatcher kMatcher(kPattern, sText);
        const bool bFound = kMatcher.FindNext();
        return Check(kMatcher) && bFound;
    }

    bool First(const RegexPattern& kPattern, const std::string& sText, std::vector<std::string>& kGroups,
               size_t uGroups)
    {
        kGroups.clear();
        RegexMatcher kMatcher(kPattern, sText);
        const bool bFound = kMatcher.FindNext();
        if (!Check(kMatcher) || !bFound)
        {
            return false;
        }
        for (size_t i = 0; i <= uGroups; ++i)
        {
            kGroups.push_back(kMatcher.GetCaptureGroup(i));
        }
        return true;
    }

    std::vector<std::string> All(const RegexPattern& kPattern, const std::string& sText, size_t uGroup)
    {
        std::vector<std::string> kOut;
        RegexMatcher kMatcher(kPattern, sText);
        while (kMatcher.FindNext())
        {
            kOut.push_back(kMatcher.GetCaptureGroup(uGroup));
        }
        Check(kMatcher);
        return kOut;
    }

    std::vector<std::string> Split(const RegexPattern& kPattern, const std::string& sText)
    {
        std::vector<std::string> kOut;
        RegexMatcher kMatcher(kPattern, sText);
        size_t uFrom = 0;
        while (kMatcher.FindNext())
        {
            kOut.push_back(sText.substr(uFrom, kMatcher.GetMatchBeginning() - uFrom));
            uFrom = kMatcher.GetMatchBeginning() + kMatcher.GetMatchLength();
        }
        Check(kMatcher);
        kOut.push_back(sText.substr(uFrom));
        return kOut;
    }

    bool Failed() const
    {
        return !msError.empty();
    }

private:
    bool Check(const RegexMatcher& kMatcher)
    {
        if (kMatcher.Failed())
        {
            if (msError.empty())
            {
                msError = kMatcher.Error();
            }
            return false;
        }
        return true;
    }

    std::string& msError;
};

long long HexValue(const std::string& sText)
{
    return static_cast<long long>(strtoull(sText.c_str(), 0, 16));
}

bool WildFrom(const std::string& sText, size_t t, const std::string& sPattern, size_t p)
{
    while (p < sPattern.size())
    {
        if (sPattern[p] == '*')
        {
            for (size_t k = t; k <= sText.size(); ++k)
            {
                if (WildFrom(sText, k, sPattern, p + 1))
                {
                    return true;
                }
            }
            return false;
        }
        if (t >= sText.size())
        {
            return false;
        }
        if (sPattern[p] != '?' && tolower(static_cast<unsigned char>(sPattern[p])) !=
                                      tolower(static_cast<unsigned char>(sText[t])))
        {
            return false;
        }
        ++p;
        ++t;
    }
    return t == sText.size();
}

void ListFiles(const BuildFiles& kFiles, const std::string& sDir, const char* pcPattern, bool bRecurse,
               std::vector<std::string>& kOut)
{
    std::vector<RepoEntry> kEntries;
    if (!kFiles.List(sDir, kEntries))
    {
        return;
    }
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        if (!kEntries[i].bDir && WildcardNoCase(kEntries[i].sName, pcPattern))
        {
            kOut.push_back(sDir + "\\" + kEntries[i].sName);
        }
    }
    if (!bRecurse)
    {
        return;
    }
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        if (kEntries[i].bDir)
        {
            ListFiles(kFiles, sDir + "\\" + kEntries[i].sName, pcPattern, true, kOut);
        }
    }
}

bool ReadText(const BuildFiles& kFiles, const std::string& sPath, std::string& sText)
{
    std::string sBytes;
    if (!kFiles.Read(sPath, sBytes))
    {
        return false;
    }
    sText = DecodeBytes(sBytes);
    return true;
}

bool ItemIncluded(const XmlNode& kItem, const std::string& sPlatform, const std::string& sConfig, bool& bIn,
                  std::string& sError)
{
    bIn = true;
    for (const XmlNode* n = &kItem; n && n->bElement; n = n->pParent)
    {
        bool bOk = true;
        if (!MsBuildCondition(Attr(*n, "Condition"), sPlatform, sConfig, bOk, sError))
        {
            return false;
        }
        if (!bOk)
        {
            bIn = false;
            return true;
        }
        if (Gate::EqualNoCase(n->sLocal, "ItemGroup"))
        {
            break;
        }
    }
    return true;
}

bool HasAttr(const XmlNode& kNode, const char* pcName)
{
    for (size_t i = 0; i < kNode.kAttrs.size(); ++i)
    {
        if (kNode.kAttrs[i].first == pcName)
        {
            return true;
        }
    }
    return false;
}

const BinaryRow* FindRow(const std::vector<BinaryRow>& kRows, const std::string& sName)
{
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        if (kRows[i].sName == sName)
        {
            return &kRows[i];
        }
    }
    return 0;
}

std::map<std::string, std::string> VendorPinValues(const std::string& sText, std::string& sError, bool& bOk)
{
    std::map<std::string, std::string> kPins;
    XmlNode kRoot;
    bOk = ParseXml(sText, "lib\\vendor-pins.props", kRoot, sError);
    if (!bOk)
    {
        return kPins;
    }
    for (size_t g = 0; g < kRoot.kChildren.size(); ++g)
    {
        const XmlNode& kGroup = kRoot.kChildren[g];
        if (!kGroup.bElement || !Gate::EqualNoCase(kGroup.sLocal, "PropertyGroup"))
        {
            continue;
        }
        for (size_t p = 0; p < kGroup.kChildren.size(); ++p)
        {
            if (kGroup.kChildren[p].bElement)
            {
                kPins[LowerText(kGroup.kChildren[p].sLocal)] = Trim(InnerText(kGroup.kChildren[p]));
            }
        }
    }
    return kPins;
}

}

bool DiskFiles::Read(const std::string& sPath, std::string& sBytes) const
{
    const std::string sRoot = RepoRoot() + "\\";
    if (Gate::StartsWithNoCase(sPath, sRoot))
    {
        return ReadRepoBytes(sPath.substr(sRoot.size()), sBytes);
    }
    std::vector<unsigned char> kBytes;
    if (!Gate::ReadFileBytes(sPath, kBytes))
    {
        return false;
    }
    sBytes.assign(kBytes.begin(), kBytes.end());
    return true;
}

bool DiskFiles::IsFile(const std::string& sPath) const
{
    return Gate::FileExists(sPath);
}

bool DiskFiles::IsDir(const std::string& sPath) const
{
    return Gate::DirExists(sPath);
}

bool DiskFiles::List(const std::string& sDir, std::vector<RepoEntry>& kOut) const
{
    kOut.clear();
    const std::string sRoot = RepoRoot() + "\\";
    if (!Gate::StartsWithNoCase(sDir, sRoot) || !Gate::DirExists(sDir))
    {
        return false;
    }
    return ListRepoFolder(sDir.substr(sRoot.size()), "*", kOut);
}

void MemoryFiles::Add(const std::string& sPath, const std::string& sBytes)
{
    mkFiles[LowerText(sPath)] = std::make_pair(sPath, sBytes);
}

bool MemoryFiles::Read(const std::string& sPath, std::string& sBytes) const
{
    std::map<std::string, std::pair<std::string, std::string> >::const_iterator it = mkFiles.find(LowerText(sPath));
    if (it == mkFiles.end())
    {
        return false;
    }
    sBytes = it->second.second;
    return true;
}

bool MemoryFiles::IsFile(const std::string& sPath) const
{
    return mkFiles.count(LowerText(sPath)) != 0;
}

bool MemoryFiles::IsDir(const std::string& sPath) const
{
    const std::string sPrefix = LowerText(sPath) + "\\";
    std::map<std::string, std::pair<std::string, std::string> >::const_iterator it = mkFiles.lower_bound(sPrefix);
    return it != mkFiles.end() && it->first.compare(0, sPrefix.size(), sPrefix) == 0;
}

bool MemoryFiles::List(const std::string& sDir, std::vector<RepoEntry>& kOut) const
{
    kOut.clear();
    if (!IsDir(sDir))
    {
        return false;
    }
    const std::string sPrefix = LowerText(sDir) + "\\";
    std::set<std::string> kSeen;
    for (std::map<std::string, std::pair<std::string, std::string> >::const_iterator it = mkFiles.lower_bound(sPrefix);
         it != mkFiles.end() && it->first.compare(0, sPrefix.size(), sPrefix) == 0; ++it)
    {
        const std::string sRest = it->second.first.substr(sPrefix.size());
        const size_t uSlash = sRest.find('\\');
        RepoEntry k;
        k.sName = uSlash == std::string::npos ? sRest : sRest.substr(0, uSlash);
        k.bDir = uSlash != std::string::npos;
        if (kSeen.insert(LowerText(k.sName)).second)
        {
            kOut.push_back(k);
        }
    }
    return true;
}

std::string JoinedLines(const std::string& sText)
{
    return Gate::JoinText(Gate::SplitLines(sText), "\n");
}

std::string DelayHalf(const std::string& sImports, std::string& sStatic)
{
    const size_t uAt = sImports.find(kDelayHeader);
    if (uAt == std::string::npos)
    {
        sStatic = sImports;
        return std::string();
    }
    sStatic = sImports.substr(0, uAt);
    return sImports.substr(uAt);
}

bool WildcardNoCase(const std::string& sText, const std::string& sPattern)
{
    return WildFrom(sText, 0, sPattern, 0);
}

std::vector<BinaryRow> HardeningChecks(const std::string& sHeaders, const std::string& sLoadConfig, int iClTotal,
                                       int iClGsMinus, int iClSdl, std::string& sError)
{
    static const RegexPattern kRelocDir("(?m)^\\s+[0-9A-F]+ \\[\\s*([0-9A-F]+)\\] RVA \\[size\\] of Base Relocation Directory",
                                        true);
    static const RegexPattern kSeh("(?m)^\\s+([0-9A-F]+) Safe Exception Handler Count", true);
    static const RegexPattern kCookie("(?m)^\\s+([0-9A-F]+) Security Cookie", true);
    static const RegexPattern kRelocName("(?m)^\\s+\\.reloc name\\s*$", true);
    static const RegexPattern kStripped("(?m)^\\s+Relocations stripped\\s*$", true);
    static const RegexPattern kDynamic("(?m)^\\s+Dynamic base\\s*$", true);
    static const RegexPattern kNx("(?m)^\\s+NX compatible\\s*$", true);
    static const RegexPattern kLaa("(?m)^\\s+Application can handle large \\(>2GB\\) addresses\\s*$", true);
    Scan k(sError);
    std::vector<std::string> g;
    long long iRelocSize = 0;
    if (k.First(kRelocDir, sHeaders, g, 1))
    {
        iRelocSize = HexValue(g[1]);
    }
    long long iSeh = 0;
    if (k.First(kSeh, sLoadConfig, g, 1))
    {
        iSeh = HexValue(g[1]);
    }
    long long iCookie = 0;
    if (k.First(kCookie, sLoadConfig, g, 1))
    {
        iCookie = HexValue(g[1]);
    }
    const bool bHasReloc = k.Has(kRelocName, sHeaders);
    const bool bStripped = k.Has(kStripped, sHeaders);
    std::vector<BinaryRow> kRows;
    kRows.push_back(Row("dynamicbase", k.Has(kDynamic, sHeaders), "Dynamic base flag missing"));
    kRows.push_back(Row("nxcompat", k.Has(kNx, sHeaders), "NX compatible flag missing"));
    kRows.push_back(Row("laa", k.Has(kLaa, sHeaders),
                        "Application can handle large (>2GB) addresses missing from dumpbin /headers"));
    kRows.push_back(Row("reloc", bHasReloc && iRelocSize > 0 && !bStripped,
                        ".reloc=" + BoolText(bHasReloc) + " reloc-dir-size=" + std::to_string(iRelocSize) +
                            " stripped=" + BoolText(bStripped)));
    kRows.push_back(Row("safeseh", iSeh > 0, "Safe Exception Handler Count=" + std::to_string(iSeh)));
    kRows.push_back(Row("gs", iClTotal > 0 && iClGsMinus == 0 && iCookie != 0,
                        "cl entries=" + std::to_string(iClTotal) + " /GS-=" + std::to_string(iClGsMinus) +
                            " security-cookie=0x" + Hex(static_cast<unsigned long long>(iCookie), 8)));
    kRows.push_back(Row("sdl", iClTotal > 0 && iClSdl == iClTotal,
                        "cl entries=" + std::to_string(iClTotal) + " /sdl=" + std::to_string(iClSdl)));
    return kRows;
}

bool CountClText(const std::string& sText, ClCounts& kCounts, std::string& sError)
{
    static const RegexPattern kWe4311("(?:^| )/we\"?4311\"?(?= |$)", false);
    static const RegexPattern kWe4312("(?:^| )/we\"?4312\"?(?= |$)", false);
    static const RegexPattern kWe4302("(?:^| )/we\"?4302\"?(?= |$)", false);
    static const RegexPattern kWe4477("(?:^| )/we\"?4477\"?(?= |$)", false);
    static const RegexPattern kGsMinus(" /GS-( |$)", true);
    static const RegexPattern kSdl(" /sdl( |$)", true);
    static const RegexPattern kAlignedNewOff(" /Zc:alignedNew-( |$)", false);
    static const RegexPattern kInlineOff(" /Zc:inline-( |$)", false);
    static const RegexPattern kInlineOn(" /Zc:inline( |$)", false);
    static const RegexPattern kThrowingNew(" /Zc:throwingNew( |$)", false);
    static const RegexPattern kGl(" /GL( |$)", false);
    static const RegexPattern kFpFast(" /fp:fast( |$)", false);
    static const RegexPattern kFpContract(" /fp:contract( |$)", false);
    Scan k(sError);
    const std::vector<std::string> kLines = Gate::SplitLines(sText);
    bool bMi = false;
    std::string sUnit;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string& l = kLines[i];
        if (!l.empty() && l[0] == '^')
        {
            const std::vector<std::string> kParts = SplitOn(l.substr(1), '|');
            sUnit = kParts[0];
            bMi = false;
            for (size_t p = 0; p < kParts.size(); ++p)
            {
                bMi = bMi || EndsWithNoCase(kParts[p], "\\SRC\\MIMALLOC\\SRC\\STATIC.C");
            }
        }
        else if (!l.empty())
        {
            ++kCounts.iTotal;
            if (bMi)
            {
                kCounts.kMiLines.push_back(l);
            }
            int iWeHits = 0;
            if (k.Has(kWe4311, l))
            {
                ++kCounts.iWe4311;
                ++iWeHits;
            }
            if (k.Has(kWe4312, l))
            {
                ++kCounts.iWe4312;
                ++iWeHits;
            }
            if (k.Has(kWe4302, l))
            {
                ++kCounts.iWe4302;
                ++iWeHits;
            }
            if (k.Has(kWe4477, l))
            {
                ++kCounts.iWe4477;
                ++iWeHits;
            }
            if (iWeHits == 4)
            {
                ++kCounts.iWeAll;
            }
            else
            {
                kCounts.kWeMissing.push_back(sUnit);
            }
            kCounts.iGsMinus += k.Has(kGsMinus, l) ? 1 : 0;
            kCounts.iSdl += k.Has(kSdl, l) ? 1 : 0;
            kCounts.iAlignedNewOff += k.Has(kAlignedNewOff, l) ? 1 : 0;
            kCounts.iInlineOff += k.Has(kInlineOff, l) ? 1 : 0;
            kCounts.iInlineOn += k.Has(kInlineOn, l) ? 1 : 0;
            kCounts.iThrowingNew += k.Has(kThrowingNew, l) ? 1 : 0;
            kCounts.iGl += k.Has(kGl, l) ? 1 : 0;
            kCounts.iFpFast += k.Has(kFpFast, l) ? 1 : 0;
            kCounts.iFpContract += k.Has(kFpContract, l) ? 1 : 0;
        }
    }
    return !k.Failed();
}

bool CountClTlogs(const BuildFiles& kFiles, const std::vector<std::string>& kDirs, ClCounts& kCounts,
                  std::string& sError)
{
    std::vector<std::string> kAll;
    for (size_t d = 0; d < kDirs.size(); ++d)
    {
        std::vector<std::string> kFound;
        ListFiles(kFiles, kDirs[d], "cl.command*.tlog", true, kFound);
        if (kFound.empty())
        {
            kCounts.kMissing.push_back(LeafName(kDirs[d]));
        }
        kAll.insert(kAll.end(), kFound.begin(), kFound.end());
    }
    for (size_t f = 0; f < kAll.size(); ++f)
    {
        std::string sText;
        if (!ReadText(kFiles, kAll[f], sText))
        {
            sError = "cannot read " + kAll[f];
            return false;
        }
        if (!CountClText(sText, kCounts, sError))
        {
            return false;
        }
    }
    return true;
}

std::vector<BinaryRow> FlagChecks(const ClCounts& kGame, const ClCounts* pTests, const std::string& sGameConfig,
                                  const std::string& sTestsConfig, const ClCounts* pLuac, const std::string& sPlatform,
                                  const AllowFile& kAllow, std::string& sError)
{
    std::vector<BinaryRow> kRows;
    std::vector<std::string> kGameMissing;
    for (size_t i = 0; i < kGame.kMissing.size(); ++i)
    {
        if (!kGame.kMissing[i].empty())
        {
            kGameMissing.push_back(kGame.kMissing[i]);
        }
    }
    const std::string sMissTail = kGameMissing.empty() ? std::string() : "; no cl tlog in " + Join(kGameMissing, " ");
    const std::string sTotal = std::to_string(kGame.iTotal);
    kRows.push_back(Row("flag-alignednew",
                        kGame.iTotal > 0 && kGame.iAlignedNewOff == kGame.iTotal && kGameMissing.empty(),
                        "cl entries=" + sTotal + " /Zc:alignedNew-=" + std::to_string(kGame.iAlignedNewOff) +
                            sMissTail));
    kRows.push_back(
        Row("flag-inline",
            kGame.iTotal > 0 && kGame.iInlineOn == kGame.iTotal && kGame.iInlineOff == 0 && kGameMissing.empty(),
            "cl entries=" + sTotal + " /Zc:inline=" + std::to_string(kGame.iInlineOn) +
                " /Zc:inline-=" + std::to_string(kGame.iInlineOff) + sMissTail));
    std::vector<const ClCounts*> kBags(1, &kGame);
    if (pTests)
    {
        kRows.push_back(Row("flag-alignednew-tests", pTests->iTotal > 0 && pTests->iAlignedNewOff == pTests->iTotal,
                            "tests cl entries=" + std::to_string(pTests->iTotal) +
                                " /Zc:alignedNew-=" + std::to_string(pTests->iAlignedNewOff)));
        kBags.push_back(pTests);
    }
    int iThrowingNew = 0;
    int iGl = 0;
    int iFpFast = 0;
    int iFpContract = 0;
    int iGsMinus = 0;
    for (size_t i = 0; i < kBags.size(); ++i)
    {
        iThrowingNew += kBags[i]->iThrowingNew;
        iGl += kBags[i]->iGl;
        iFpFast += kBags[i]->iFpFast;
        iFpContract += kBags[i]->iFpContract;
        iGsMinus += kBags[i]->iGsMinus;
    }
    kRows.push_back(Row("flag-banned", iThrowingNew + iGl + iFpFast + iFpContract + iGsMinus == 0,
                        "/Zc:throwingNew=" + std::to_string(iThrowingNew) + " /GL=" + std::to_string(iGl) +
                            " /fp:fast=" + std::to_string(iFpFast) + " /fp:contract=" + std::to_string(iFpContract) +
                            " /GS-=" + std::to_string(iGsMinus)));
    if (Gate::EqualNoCase(sPlatform, "x64"))
    {
        bool bTruncOk = kGameMissing.empty();
        std::vector<std::string> kParts;
        const char* const apcNames[] = {"game", "tests", "luac"};
        const ClCounts* apBags[] = {&kGame, pTests, pLuac};
        for (size_t i = 0; i < 3; ++i)
        {
            const ClCounts* b = apBags[i];
            if (!b || b->iTotal == 0)
            {
                bTruncOk = false;
                kParts.push_back(std::string(apcNames[i]) + " cl entries=0");
                continue;
            }
            std::string sPart = std::string(apcNames[i]) + " cl entries=" + std::to_string(b->iTotal) +
                                " /we4311=" + std::to_string(b->iWe4311) + " /we4312=" + std::to_string(b->iWe4312) +
                                " /we4302=" + std::to_string(b->iWe4302) + " /we4477=" + std::to_string(b->iWe4477) +
                                " all four=" + std::to_string(b->iWeAll);
            if (b->iWeAll != b->iTotal)
            {
                bTruncOk = false;
                std::string sFirst = "unknown unit";
                for (size_t m = 0; m < b->kWeMissing.size(); ++m)
                {
                    if (!b->kWeMissing[m].empty())
                    {
                        sFirst = b->kWeMissing[m];
                        break;
                    }
                }
                sPart += " first missing " + sFirst;
            }
            kParts.push_back(sPart);
        }
        kRows.push_back(Row("x64-truncation", bTruncOk, Join(kParts, "; ") + sMissTail));
    }
    static const RegexPattern kHave("(?:^| )/D \"?(MI_(?:DEBUG|SECURE|TRACK_ASAN)(?:=[^\" ]*)?)\"?(?= |$)", false);
    static const RegexPattern kAsan(" /fsanitize=address(?= |$)", false);
    static const RegexPattern kTrack("(?:^| )/D \"?MI_TRACK_ASAN=1\"?(?= |$)", false);
    const char* const apcNeed[][2] = {{"MI_NO_GETENV=1", "MI_NO_GETENV=1"},
                                      {"MI_USE_RTLGENRANDOM", "MI_USE_RTLGENRANDOM(?:=1)?"},
                                      {"MI_PAGE_CACHE_OFFSET=0", "MI_PAGE_CACHE_OFFSET=0"}};
    Scan k(sError);
    std::vector<std::string> kMiBad;
    std::vector<std::pair<std::string, std::pair<const ClCounts*, std::string> > > kMiBags;
    kMiBags.push_back(std::make_pair(std::string("game"), std::make_pair(&kGame, sGameConfig)));
    if (pTests)
    {
        kMiBags.push_back(std::make_pair(std::string("tests"), std::make_pair(pTests, sTestsConfig)));
    }
    for (size_t b = 0; b < kMiBags.size(); ++b)
    {
        const std::string& sName = kMiBags[b].first;
        const ClCounts& kBag = *kMiBags[b].second.first;
        const std::string& sConfig = kMiBags[b].second.second;
        std::vector<std::string> kLines;
        for (size_t i = 0; i < kBag.kMiLines.size(); ++i)
        {
            if (!kBag.kMiLines[i].empty())
            {
                kLines.push_back(kBag.kMiLines[i]);
            }
        }
        if (kLines.size() != 1)
        {
            kMiBad.push_back(sName + " static.c command lines=" + std::to_string(kLines.size()));
            continue;
        }
        const std::string& l = kLines[0];
        for (size_t n = 0; n < 3; ++n)
        {
            const RegexPattern kNeed(std::string("(?:^| )/D \"?") + apcNeed[n][1] + "\"?(?= |$)", false);
            if (!k.Has(kNeed, l))
            {
                kMiBad.push_back(sName + " missing " + apcNeed[n][0]);
            }
        }
        const std::vector<std::string>* pWant = 0;
        for (size_t s = 0; s < kAllow.kSections.size(); ++s)
        {
            if (Gate::EqualNoCase(kAllow.kSections[s].sName, "MiDefines " + sConfig))
            {
                pWant = &kAllow.kSections[s].kEntries;
            }
        }
        if (!pWant || pWant->empty())
        {
            kMiBad.push_back(sName + " has no expected defines for configuration \"" + sConfig + "\"");
        }
        else
        {
            const std::string sHave = Join(Sorted(k.All(kHave, l, 1)), " ");
            const std::string sExp = Join(Sorted(*pWant), " ");
            if (sHave != sExp)
            {
                kMiBad.push_back(sName + " " + sConfig + " has \"" + sHave + "\" want \"" + sExp + "\"");
            }
        }
        const bool bAsan = k.Has(kAsan, l);
        const bool bTrack = k.Has(kTrack, l);
        if (bAsan && !bTrack)
        {
            kMiBad.push_back(sName + " missing MI_TRACK_ASAN=1 with /fsanitize=address");
        }
        else if (bTrack && !bAsan)
        {
            kMiBad.push_back(sName + " MI_TRACK_ASAN=1 without /fsanitize=address");
        }
    }
    kRows.push_back(Row("mimalloc-defines", kMiBad.empty(), Join(kMiBad, "; ")));
    return kRows;
}

namespace
{

BinaryRow D3dxImportsCheck(const std::string& sStatic, std::string& sError)
{
    static const RegexPattern kBlock("(?msi)^[ \\t]+d3dx9_43\\.dll[ \\t]*\\r?$(.*?)(?=^[ \\t]+\\S+\\.dll[ \\t]*\\r?$|^[ \\t]+"
                                     "Summary[ \\t]*\\r?$|\\z)",
                                     false);
    static const RegexPattern kEntry("(?m)^[ \\t]+(?:[0-9A-Fa-f]+[ \\t]+(\\S+)|(Ordinal[ \\t]+\\d+))[ \\t]*\\r?$", false);
    static const RegexPattern kSpace("\\s+", false);
    Scan k(sError);
    std::vector<std::string> kNames;
    std::vector<std::string> g;
    const bool bFound = k.First(kBlock, sStatic, g, 1);
    if (bFound)
    {
        const std::string sBody = g[1];
        RegexMatcher kMatcher(kEntry, sBody);
        while (kMatcher.FindNext())
        {
            if (kMatcher.GroupMatched(1))
            {
                kNames.push_back(kMatcher.GetCaptureGroup(1));
            }
            else
            {
                kNames.push_back(Join(k.Split(kSpace, kMatcher.GetCaptureGroup(2)), " "));
            }
        }
        if (kMatcher.Failed() && sError.empty())
        {
            sError = kMatcher.Error();
        }
    }
    const std::vector<std::string> kAllow = NameList(kD3dxImportsAllow);
    std::vector<std::string> kExtra;
    for (size_t i = 0; i < kNames.size(); ++i)
    {
        if (!ContainsCase(kAllow, kNames[i]))
        {
            kExtra.push_back(kNames[i]);
        }
    }
    kExtra = SortUnique(kExtra);
    std::vector<std::string> kMissing;
    for (size_t i = 0; i < kAllow.size(); ++i)
    {
        if (!ContainsCase(kNames, kAllow[i]))
        {
            kMissing.push_back(kAllow[i]);
        }
    }
    std::vector<std::string> kDetail;
    if (!bFound)
    {
        kDetail.push_back("d3dx9_43.dll not in the import table");
    }
    if (!kExtra.empty())
    {
        kDetail.push_back("extra: " + Join(kExtra, " "));
    }
    if (!kMissing.empty())
    {
        kDetail.push_back("missing: " + Join(kMissing, " "));
    }
    return Row("d3dx-imports", kDetail.empty(), Join(kDetail, "; "));
}

BinaryRow CrtFloatImportsCheck(const std::string& sStatic, std::string& sError)
{
    static const RegexPattern kNames("(?m)^[ \\t]+[0-9A-Fa-f]+[ \\t]+(\\S+)[ \\t]*\\r?$", false);
    static const RegexPattern kMath("(?mi)^[ \\t]+api-ms-win-crt-math-l1-1-0\\.dll[ \\t]*\\r?$", true);
    Scan k(sError);
    const std::vector<std::string> kAll = k.All(kNames, sStatic, 1);
    const std::vector<std::string> kFloat = NameList(kCrtFloatImports);
    std::vector<std::string> kFound;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (ContainsCase(kFloat, kAll[i]))
        {
            kFound.push_back(kAll[i]);
        }
    }
    kFound = SortUnique(kFound);
    std::vector<std::string> kDetail;
    if (!k.Has(kMath, sStatic))
    {
        kDetail.push_back("api-ms-win-crt-math-l1-1-0.dll not in the import table");
    }
    if (!kFound.empty())
    {
        kDetail.push_back("float CRT imports: " + Join(kFound, " "));
    }
    return Row("crt-float-imports", kDetail.empty(), Join(kDetail, "; "));
}

}

std::vector<BinaryRow> ImportChecks(const std::string& sImports, const std::string& sPlatform,
                                    const std::vector<std::string>& kStaticAllow,
                                    const std::vector<std::string>& kDelayAllow, std::string& sError)
{
    static const RegexPattern kShcore("(?mi)^\\s+shcore\\.dll\\s*$", true);
    static const RegexPattern kDll("(?mi)^\\s+(\\S+\\.dll)\\s*$", false);
    Scan k(sError);
    std::string sStatic;
    const std::string sDelay = DelayHalf(sImports, sStatic);
    std::vector<BinaryRow> kRows;
    for (size_t i = 0; i < sizeof(kDpiImports) / sizeof(kDpiImports[0]); ++i)
    {
        const RegexPattern kName(std::string("(?m)\\s") + kDpiImports[i] + "\\s*$", false);
        kRows.push_back(Row("import-" + std::string(kDpiImports[i]), !k.Has(kName, sStatic),
                            std::string(kDpiImports[i]) + " statically imported (breaks Win7 load)"));
    }
    kRows.push_back(Row("import-shcore", !k.Has(kShcore, sStatic), "shcore.dll in import table"));
    std::vector<std::string> kDlls = k.All(kDll, sStatic, 1);
    for (size_t i = 0; i < kDlls.size(); ++i)
    {
        kDlls[i] = LowerText(kDlls[i]);
    }
    kDlls = SortUnique(kDlls);
    std::vector<std::string> kAllow = kStaticAllow;
    kAllow.push_back(Gate::AsanRuntimeName(sPlatform));
    std::vector<std::string> kExtra;
    for (size_t i = 0; i < kDlls.size(); ++i)
    {
        if (!ContainsNoCase(kAllow, kDlls[i]))
        {
            kExtra.push_back(kDlls[i]);
        }
    }
    std::string sDetail = "imports outside the allow list: " + Join(kExtra, " ");
    const bool bKernel = ContainsNoCase(kDlls, "kernel32.dll");
    if (!bKernel)
    {
        sDetail = "kernel32.dll not found in the dumpbin /imports output (" + std::to_string(kDlls.size()) +
                  " DLLs parsed)";
    }
    kRows.push_back(Row("import-modules", kExtra.empty() && bKernel, sDetail));
    std::vector<std::string> kDelayDlls = k.All(kDll, sDelay, 1);
    for (size_t i = 0; i < kDelayDlls.size(); ++i)
    {
        kDelayDlls[i] = LowerText(kDelayDlls[i]);
    }
    kDelayDlls = SortUnique(kDelayDlls);
    std::vector<std::string> kDelayExtra;
    for (size_t i = 0; i < kDelayDlls.size(); ++i)
    {
        if (!ContainsNoCase(kDelayAllow, kDelayDlls[i]))
        {
            kDelayExtra.push_back(kDelayDlls[i]);
        }
    }
    kRows.push_back(Row("import-delay-modules", kDelayExtra.empty(),
                        "delay imports outside the allow list: " + Join(kDelayExtra, " ")));
    if (Gate::EqualNoCase(sPlatform, "x64"))
    {
        kRows.push_back(D3dxImportsCheck(sStatic, sError));
        kRows.push_back(CrtFloatImportsCheck(sStatic, sError));
    }
    return kRows;
}

BinaryRow UnicowsBuildCheck(const std::string& sExeDir, bool bStaged, const std::string& sImports,
                            std::string& sError)
{
    static const RegexPattern kUnicows("(?mi)^\\s+unicows\\.dll\\s*$", true);
    Scan k(sError);
    std::vector<std::string> kHits;
    if (!sExeDir.empty() && bStaged)
    {
        kHits.push_back("staged in " + sExeDir);
    }
    std::string sStatic;
    const std::string sDelay = DelayHalf(sImports, sStatic);
    if (k.Has(kUnicows, sStatic))
    {
        kHits.push_back("static import");
    }
    if (k.Has(kUnicows, sDelay))
    {
        kHits.push_back("delay import");
    }
    return Row("unicows-staged", kHits.empty(), Join(kHits, ", "));
}

BinaryRow BinkwBuildCheck(const std::string& sExeDir, bool bStaged, const std::string& sImports, std::string& sError)
{
    static const RegexPattern kBink("(?mi)^\\s+(binkw(32|64)\\.dll)\\s*$", true);
    Scan k(sError);
    std::vector<std::string> kHits;
    std::string sStatic;
    const std::string sDelay = DelayHalf(sImports, sStatic);
    std::vector<std::string> g;
    if (k.First(kBink, sStatic, g, 1))
    {
        kHits.push_back("static import " + g[1]);
    }
    if (k.First(kBink, sDelay, g, 1))
    {
        kHits.push_back("delay import " + g[1]);
    }
    if (!sExeDir.empty() && bStaged)
    {
        kHits.push_back("staged in " + sExeDir);
    }
    return Row("bink-gone", kHits.empty(), Join(kHits, ", "));
}

BinaryRow SteamApi64StagedCheck(const std::string& sDll, bool bStaged, const std::string& sSha, const std::string& sPin,
                                const std::string& sExports, const std::string& sImports, const std::string& sDef,
                                std::string& sError)
{
    static const RegexPattern kExport("(?m)^\\s+\\d+\\s+[0-9A-F]+\\s+[0-9A-F]{8}\\s+(\\S+)", false);
    static const RegexPattern kBlock("(?ms)^ {4}steam_api64\\.dll\\s*$(.*?)(?=^ {4}\\S|\\z)", false);
    static const RegexPattern kImportedRx("(?m)^\\s+[0-9A-F]{8,16}\\s+[0-9A-F]+\\s+(\\S+)\\s*$", false);
    static const RegexPattern kDefName("(?m)^\\s+(\\w+)", false);
    if (!bStaged)
    {
        return SkipRow("steam-api64-staged", "not staged");
    }
    if (sSha != sPin)
    {
        return Row("steam-api64-staged", false, sDll + " is " + sSha + ", pin is " + sPin);
    }
    Scan k(sError);
    const std::vector<std::string> kHave = k.All(kExport, sExports, 1);
    std::string sStatic;
    const std::string sDelay = DelayHalf(sImports, sStatic);
    std::vector<std::string> g;
    if (!k.First(kBlock, sDelay, g, 1))
    {
        return Row("steam-api64-staged", false, "the exe has no steam_api64.dll delay import");
    }
    const std::vector<std::string> kImported = k.All(kImportedRx, g[1], 1);
    const size_t uDefAt = sDef.find("EXPORTS");
    std::vector<std::string> kDefNames;
    if (uDefAt != std::string::npos)
    {
        kDefNames = k.All(kDefName, sDef.substr(uDefAt + 7), 1);
    }
    std::vector<std::string> kHits;
    if (kImported.empty())
    {
        kHits.push_back("no function parsed from the steam_api64.dll delay import");
    }
    if (kDefNames.empty())
    {
        kHits.push_back("no name parsed from lib\\x64\\steam_api64.def");
    }
    for (size_t i = 0; i < kImported.size(); ++i)
    {
        if (!ContainsCase(kHave, kImported[i]))
        {
            kHits.push_back(kImported[i] + " imported by the exe is not exported");
        }
    }
    for (size_t i = 0; i < kDefNames.size(); ++i)
    {
        if (!ContainsCase(kHave, kDefNames[i]))
        {
            kHits.push_back(kDefNames[i] + " in steam_api64.def is not exported");
        }
    }
    return Row("steam-api64-staged", kHits.empty(), Join(kHits, ", "));
}

bool MsBuildCondition(const std::string& sCondition, const std::string& sPlatform, const std::string& sConfig,
                      bool& bOk, std::string& sError)
{
    static const RegexPattern kAnd("\\s+and\\s+", true);
    static const RegexPattern kPart("^'\\$\\((Platform|Configuration)\\)'\\s*==\\s*'([^'$()]*)'$", false);
    bOk = true;
    const std::string sTrim = Trim(sCondition);
    if (sTrim.empty())
    {
        return true;
    }
    std::string sRxError;
    Scan k(sRxError);
    const std::vector<std::string> kParts = k.Split(kAnd, sTrim);
    for (size_t i = 0; i < kParts.size(); ++i)
    {
        std::vector<std::string> g;
        if (!k.First(kPart, kParts[i], g, 2))
        {
            sError = sRxError.empty() ? "unsupported MSBuild condition: " + sCondition : sRxError;
            return false;
        }
        const std::string& sHave = g[1] == "Platform" ? sPlatform : sConfig;
        if (!Gate::EqualNoCase(sHave, g[2]))
        {
            bOk = false;
        }
    }
    if (!sRxError.empty())
    {
        sError = sRxError;
        return false;
    }
    return true;
}

bool GameProjects(const BuildFiles& kFiles, const std::string& sRoot, const std::string& sPlatform,
                  const std::string& sConfig, std::vector<GameProject>& kOut, std::string& sError)
{
    kOut.clear();
    const std::string sObjDir = sRoot + "\\obj\\" + sPlatform + "\\" + sConfig;
    GameProject kExe;
    kExe.sName = "BattlefrontII";
    kExe.sVcxproj = sRoot + "\\BattlefrontII.vcxproj";
    kExe.sIntDir = sObjDir;
    kExe.sTlogDir = sObjDir + "\\BattlefrontII.tlog";
    kOut.push_back(kExe);
    std::string sText;
    if (!ReadText(kFiles, kExe.sVcxproj, sText))
    {
        sError = "cannot read " + kExe.sVcxproj;
        return false;
    }
    XmlNode kRoot;
    if (!ParseXml(sText, kExe.sVcxproj, kRoot, sError))
    {
        return false;
    }
    std::vector<const XmlNode*> kAll;
    Elements(kRoot, kAll);
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        const XmlNode& r = *kAll[i];
        if (!IsMsb(r, "ProjectReference") || !r.pParent || !IsMsb(*r.pParent, "ItemGroup"))
        {
            continue;
        }
        bool bIn = true;
        if (!ItemIncluded(r, sPlatform, sConfig, bIn, sError))
        {
            return false;
        }
        if (!bIn)
        {
            continue;
        }
        const XmlNode* pUse = 0;
        for (size_t c = 0; c < r.kChildren.size() && !pUse; ++c)
        {
            if (IsMsb(r.kChildren[c], "UseLibraryDependencyInputs"))
            {
                pUse = &r.kChildren[c];
            }
        }
        if (!pUse || !Gate::EqualNoCase(Trim(InnerText(*pUse)), "true"))
        {
            continue;
        }
        GameProject k;
        k.sVcxproj = Gate::FullPath(Gate::JoinPath(sRoot, Attr(r, "Include")));
        k.sName = BaseName(k.sVcxproj);
        k.sIntDir = sObjDir + "\\lib\\" + k.sName;
        k.sTlogDir = k.sIntDir + "\\" + k.sName + ".tlog";
        kOut.push_back(k);
    }
    return true;
}

bool LinkedObjects(const std::string& sLinkTlog, std::vector<std::string>& kObjects)
{
    const std::vector<std::string> kLines = Gate::SplitLines(sLinkTlog);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string l = StripBom(kLines[i]);
        if (l.empty() || l[0] != '^')
        {
            continue;
        }
        const std::vector<std::string> kParts = SplitOn(l.substr(1), '|');
        for (size_t p = 0; p < kParts.size(); ++p)
        {
            if (WildcardNoCase(kParts[p], "*.obj") && !ContainsCase(kObjects, kParts[p]))
            {
                kObjects.push_back(kParts[p]);
            }
        }
    }
    return !kObjects.empty();
}

bool X64ObjectsCheck(const BuildFiles& kFiles, const std::string& sRoot, const std::string& sPlatform,
                     const std::string& sConfig, BinaryRow& kRow, std::string& sError)
{
    std::vector<GameProject> kProjects;
    if (!GameProjects(kFiles, sRoot, sPlatform, sConfig, kProjects, sError))
    {
        return false;
    }
    const std::string sLinkTlog = kProjects[0].sTlogDir + "\\link.command.1.tlog";
    std::set<std::string> kLinked;
    std::vector<std::string> kBad;
    std::string sLinkText;
    if (!kFiles.IsFile(sLinkTlog) || !ReadText(kFiles, sLinkTlog, sLinkText))
    {
        kBad.push_back("no link tlog " + sLinkTlog);
    }
    else
    {
        const std::vector<std::string> kLines = Gate::SplitLines(sLinkText);
        for (size_t i = 0; i < kLines.size(); ++i)
        {
            const std::string l = StripBom(kLines[i]);
            if (!l.empty() && l[0] == '^')
            {
                const std::vector<std::string> kParts = SplitOn(l.substr(1), '|');
                for (size_t p = 0; p < kParts.size(); ++p)
                {
                    kLinked.insert(LowerText(kParts[p]));
                }
            }
        }
    }
    int iUnits = 0;
    int iMasm = 0;
    std::vector<std::string> kMissing;
    for (size_t p = 0; p < kProjects.size(); ++p)
    {
        std::string sText;
        if (!ReadText(kFiles, kProjects[p].sVcxproj, sText))
        {
            sError = "cannot read " + kProjects[p].sVcxproj;
            return false;
        }
        XmlNode kRoot;
        if (!ParseXml(sText, kProjects[p].sVcxproj, kRoot, sError))
        {
            return false;
        }
        std::vector<const XmlNode*> kAll;
        Elements(kRoot, kAll);
        for (size_t i = 0; i < kAll.size(); ++i)
        {
            const XmlNode& kItem = *kAll[i];
            const bool bMasm = IsMsb(kItem, "MASM");
            if ((!bMasm && !IsMsb(kItem, "ClCompile")) || !kItem.pParent || !IsMsb(*kItem.pParent, "ItemGroup") ||
                !HasAttr(kItem, "Include"))
            {
                continue;
            }
            bool bIn = true;
            if (!ItemIncluded(kItem, sPlatform, sConfig, bIn, sError))
            {
                return false;
            }
            if (!bIn)
            {
                continue;
            }
            if (bMasm)
            {
                ++iMasm;
            }
            else
            {
                ++iUnits;
            }
            const std::string sInclude = Attr(kItem, "Include");
            const std::string sParent = ParentDir(sInclude);
            const std::string sRel = (sParent.empty() ? std::string() : sParent + "\\") + BaseName(sInclude) + ".obj";
            const std::string sObj = Gate::FullPath(kProjects[p].sIntDir + "\\" + sRel);
            if (!kFiles.IsFile(sObj))
            {
                kMissing.push_back(sObj + " not built");
            }
            else if (!kLinked.count(LowerText(sObj)))
            {
                kMissing.push_back(sObj + " not linked");
            }
        }
    }
    if (iUnits == 0)
    {
        kBad.push_back("no ClCompile items");
    }
    std::string sDetail = "units=" + std::to_string(iUnits) + " missing=" + std::to_string(kMissing.size()) +
                          " masm=" + std::to_string(iMasm);
    if (!kMissing.empty())
    {
        std::vector<std::string> kFirst(kMissing.begin(), kMissing.begin() + std::min<size_t>(3, kMissing.size()));
        sDetail += " " + Join(kFirst, ", ");
    }
    if (!kBad.empty())
    {
        sDetail += "; " + Join(kBad, "; ");
    }
    kRow = Row("x64-objects", kMissing.empty() && kBad.empty(), sDetail);
    return true;
}

BinaryRow CrtTrigRouteCheck(const std::vector<std::string>& kObjects, const std::string& sSymbols,
                            std::string& sError)
{
    static const RegexPattern kDump("(?m)^Dump of file ", true);
    static const RegexPattern kRef(
        "(?m)^[0-9A-Fa-f]+\\s+[0-9A-Fa-f]+\\s+UNDEF\\s.*\\bExternal\\s+\\|\\s+(?:__imp_)?(sin|cos|tan)\\s*$", false);
    Scan k(sError);
    std::set<std::string> kDumped;
    std::vector<std::string> kBad;
    const std::vector<std::string> kChunks = k.Split(kDump, sSymbols);
    for (size_t c = 1; c < kChunks.size(); ++c)
    {
        const std::string sFile = Trim(SplitCrLf(kChunks[c])[0]);
        kDumped.insert(LowerText(Gate::FullPath(sFile)));
        if (Gate::EqualNoCase(FileName(sFile), kCrtTrigHelper))
        {
            continue;
        }
        const std::vector<std::string> kRefs = SortUnique(k.All(kRef, kChunks[c], 1));
        if (!kRefs.empty())
        {
            kBad.push_back(FileName(sFile) + " references " + Join(kRefs, ","));
        }
    }
    std::vector<std::string> kMissing;
    int iHelper = 0;
    for (size_t i = 0; i < kObjects.size(); ++i)
    {
        if (!kDumped.count(LowerText(Gate::FullPath(kObjects[i]))))
        {
            kMissing.push_back(kObjects[i]);
        }
        iHelper += Gate::EqualNoCase(FileName(kObjects[i]), kCrtTrigHelper) ? 1 : 0;
    }
    if (!kMissing.empty())
    {
        std::vector<std::string> kFirst(kMissing.begin(), kMissing.begin() + std::min<size_t>(3, kMissing.size()));
        kBad.push_back("no symbols for " + std::to_string(kMissing.size()) + " objects: " + Join(kFirst, ", "));
    }
    if (iHelper != 1)
    {
        kBad.push_back(std::string(kCrtTrigHelper) + " not linked");
    }
    return Row("crt-trig-route", kBad.empty(),
               "objects=" + std::to_string(kObjects.size()) + (kBad.empty() ? std::string() : "; " + Join(kBad, "; ")));
}

bool Fma3OffRanges(const std::string& sMap, std::vector<std::pair<unsigned long long, unsigned long long> >& kRanges,
                   std::string& sError)
{
    static const RegexPattern kSym("(?m)^ [0-9a-f]{4}:[0-9a-f]{8}\\s+(\\S+)\\s+([0-9a-f]{16}) ", false);
    std::vector<std::pair<std::string, unsigned long long> > kSyms;
    RegexMatcher kMatcher(kSym, sMap);
    while (kMatcher.FindNext())
    {
        kSyms.push_back(std::make_pair(kMatcher.GetCaptureGroup(1),
                                       strtoull(kMatcher.GetCaptureGroup(2).c_str(), 0, 16)));
    }
    if (kMatcher.Failed())
    {
        sError = kMatcher.Error();
        return false;
    }
    std::vector<unsigned long long> kVas;
    for (size_t i = 0; i < kSyms.size(); ++i)
    {
        kVas.push_back(kSyms[i].second);
    }
    std::sort(kVas.begin(), kVas.end());
    kVas.erase(std::unique(kVas.begin(), kVas.end()), kVas.end());
    for (size_t f = 0; f < sizeof(kFma3OffFunctions) / sizeof(kFma3OffFunctions[0]); ++f)
    {
        std::vector<unsigned long long> kHits;
        for (size_t i = 0; i < kSyms.size(); ++i)
        {
            if (kSyms[i].first == kFma3OffFunctions[f])
            {
                kHits.push_back(kSyms[i].second);
            }
        }
        if (kHits.size() != 1)
        {
            sError = "map has " + std::to_string(kHits.size()) + " symbols named " + kFma3OffFunctions[f];
            return false;
        }
        std::vector<unsigned long long>::const_iterator itEnd = std::upper_bound(kVas.begin(), kVas.end(), kHits[0]);
        if (itEnd == kVas.end())
        {
            sError = std::string("map has no symbol after ") + kFma3OffFunctions[f];
            return false;
        }
        kRanges.push_back(std::make_pair(kHits[0], *itEnd - 1));
    }
    return true;
}

BinaryRow Fma3OffCheck(const std::string& sDisasm, std::string& sError)
{
    static const RegexPattern kHead("^(\\S.*):\\s*$", false);
    static const RegexPattern kInstr("^\\s+[0-9A-F]{8,16}:\\s+(\\S.*?)\\s*$", false);
    static const RegexPattern kCallMath("^(call|jmp)\\s+\\?PcMathStartup@@YAXXZ$", false);
    static const RegexPattern kCallFma(
        "^(call|jmp)\\s+(qword\\s+ptr\\s+\\[__imp__set_FMA3_enable\\]|_set_FMA3_enable)$", false);
    static const RegexPattern kZero("^(xor\\s+ecx,ecx|mov\\s+ecx,0)$", false);
    static const RegexPattern kClobber("^(call\\b|\\w+\\s+(ecx|rcx|cx|cl|ch)\\b)", false);
    Scan k(sError);
    std::vector<std::string> kBad;
    std::vector<std::pair<std::string, std::vector<std::string> > > kBlocks;
    int iCur = -1;
    const std::vector<std::string> kLines = SplitCrLf(sDisasm);
    std::vector<std::string> g;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (k.First(kHead, kLines[i], g, 1))
        {
            iCur = -1;
            for (size_t b = 0; b < kBlocks.size(); ++b)
            {
                if (Gate::EqualNoCase(kBlocks[b].first, g[1]))
                {
                    iCur = static_cast<int>(b);
                }
            }
            if (iCur < 0)
            {
                kBlocks.push_back(std::make_pair(g[1], std::vector<std::string>()));
                iCur = static_cast<int>(kBlocks.size()) - 1;
            }
        }
        else if (iCur >= 0 && k.First(kInstr, kLines[i], g, 1))
        {
            kBlocks[iCur].second.push_back(g[1]);
        }
    }
    const std::vector<std::string>* pHeap = 0;
    const std::vector<std::string>* pMath = 0;
    for (size_t b = 0; b < kBlocks.size(); ++b)
    {
        if (Gate::EqualNoCase(kBlocks[b].first, "Startup_RedInitHeap"))
        {
            pHeap = &kBlocks[b].second;
        }
        if (Gate::EqualNoCase(kBlocks[b].first, "?PcMathStartup@@YAXXZ"))
        {
            pMath = &kBlocks[b].second;
        }
    }
    if (!pHeap || pHeap->empty())
    {
        kBad.push_back("Startup_RedInitHeap not found");
    }
    else
    {
        bool bCalls = false;
        for (size_t i = 0; i < pHeap->size(); ++i)
        {
            bCalls = bCalls || k.Has(kCallMath, (*pHeap)[i]);
        }
        if (!bCalls)
        {
            kBad.push_back("Startup_RedInitHeap does not call PcMathStartup");
        }
    }
    if (!pMath || pMath->empty())
    {
        kBad.push_back("PcMathStartup not found");
    }
    else
    {
        bool bZero = false;
        int iHit = -1;
        for (size_t i = 0; i < pMath->size(); ++i)
        {
            const std::string& s = (*pMath)[i];
            if (k.Has(kCallFma, s))
            {
                iHit = bZero ? 1 : 0;
                break;
            }
            if (k.Has(kZero, s))
            {
                bZero = true;
            }
            else if (k.Has(kClobber, s))
            {
                bZero = false;
            }
        }
        if (iHit < 0)
        {
            kBad.push_back("PcMathStartup does not call _set_FMA3_enable");
        }
        else if (iHit == 0)
        {
            kBad.push_back("PcMathStartup does not pass 0 to _set_FMA3_enable");
        }
    }
    return Row("fma3-off", kBad.empty(), Join(kBad, "; "));
}

BinaryRow MiCrtHookCheck(const std::string& sMap, std::string& sError)
{
    static const RegexPattern kStatic("(^|:)static\\.obj$", true);
    std::vector<std::string> kBad;
    if (sMap.empty())
    {
        kBad.push_back("no map");
    }
    else
    {
        Scan k(sError);
        const char* const apcWant[][2] = {{".CRT$XIB", "__mi_crt_callback_init"},
                                          {".CRT$XLB", "__mi_tls_callback_pre"},
                                          {".CRT$XLY", "__mi_tls_callback_post"}};
        for (size_t w = 0; w < 3; ++w)
        {
            const RegexPattern kSec("(?m)^ ([0-9a-f]{4}):([0-9a-f]{8}) ([0-9a-f]{8})H " + RegexEscape(apcWant[w][0]) +
                                        "\\s",
                                    false);
            const RegexPattern kSym("(?m)^ ([0-9a-f]{4}):([0-9a-f]{8})\\s+" + RegexEscape(apcWant[w][1]) +
                                        "\\s+[0-9a-f]{8}\\s+(\\S+)\\s*$",
                                    false);
            std::vector<std::string> kS;
            std::vector<std::string> kY;
            const bool bSec = k.First(kSec, sMap, kS, 3);
            const bool bSym = k.First(kSym, sMap, kY, 3);
            if (!bSec)
            {
                kBad.push_back(std::string(apcWant[w][0]) + " missing");
                continue;
            }
            if (!bSym)
            {
                kBad.push_back(std::string(apcWant[w][1]) + " missing");
                continue;
            }
            const long long iStart = HexValue(kS[2]);
            const long long iEnd = iStart + HexValue(kS[3]);
            const long long iAt = HexValue(kY[2]);
            if (!Gate::EqualNoCase(kY[1], kS[1]) || iAt < iStart || iAt >= iEnd)
            {
                kBad.push_back(std::string(apcWant[w][1]) + " outside " + apcWant[w][0]);
            }
            else if (!k.Has(kStatic, kY[3]))
            {
                kBad.push_back(std::string(apcWant[w][1]) + " from " + kY[3]);
            }
        }
    }
    return Row("mimalloc-crt-hooks", kBad.empty(), Join(kBad, "; "));
}

BinaryRow RiderLinkMapCheck(const std::string& sMap, const std::string& sConfig, std::string& sError)
{
    static const RegexPattern kLink("@RiderLink(?:Protocol)?@@|\\?g_fnLineTap@", false);
    std::vector<std::string> kBad;
    if (sMap.empty())
    {
        kBad.push_back("no map");
    }
    else
    {
        Scan k(sError);
        std::vector<std::string> kHits;
        const std::vector<std::string> kLines = SplitCrLf(sMap);
        for (size_t i = 0; i < kLines.size(); ++i)
        {
            if (k.Has(kLink, kLines[i]))
            {
                kHits.push_back(kLines[i]);
            }
        }
        if (Gate::EqualNoCase(sConfig, "Release"))
        {
            if (!kHits.empty())
            {
                std::vector<std::string> kFirst;
                for (size_t i = 0; i < kHits.size() && i < 3; ++i)
                {
                    kFirst.push_back(Trim(kHits[i]));
                }
                kBad.push_back("link code in Release: " + Join(kFirst, " | "));
            }
        }
        else if (kHits.empty())
        {
            kBad.push_back("no link symbol in the " + sConfig + " map");
        }
    }
    return Row("riderlink-map", kBad.empty(), Join(kBad, "; "));
}

std::string PeAslr(const std::string& sBytes)
{
    const size_t uGot = std::min<size_t>(4096, sBytes.size());
    const unsigned char* b = reinterpret_cast<const unsigned char*>(sBytes.data());
    struct Le
    {
        static unsigned long U32(const unsigned char* p, size_t i)
        {
            return static_cast<unsigned long>(p[i]) | (static_cast<unsigned long>(p[i + 1]) << 8) |
                   (static_cast<unsigned long>(p[i + 2]) << 16) | (static_cast<unsigned long>(p[i + 3]) << 24);
        }
        static unsigned int U16(const unsigned char* p, size_t i)
        {
            return static_cast<unsigned int>(p[i]) | (static_cast<unsigned int>(p[i + 1]) << 8);
        }
    };
    if (uGot < 0x40 || b[0] != 0x4D || b[1] != 0x5A)
    {
        return "no MZ header";
    }
    const long long e = static_cast<long long>(static_cast<int>(Le::U32(b, 0x3C)));
    const long long iOpt = e + 24;
    const std::string sOutside = "PE header at 0x" + Hex(static_cast<unsigned long>(static_cast<int>(e)), 1) +
                                 " is outside the first " + std::to_string(uGot) + " bytes";
    if (e < 0x40 || iOpt + 2 > static_cast<long long>(uGot))
    {
        return sOutside;
    }
    if (Le::U32(b, static_cast<size_t>(e)) != 0x00004550)
    {
        return "no PE signature";
    }
    const unsigned int uMagic = Le::U16(b, static_cast<size_t>(iOpt));
    long long iCountAt = 0;
    long long iDirAt = 0;
    if (uMagic == 0x10B)
    {
        iCountAt = iOpt + 92;
        iDirAt = iOpt + 96;
    }
    else if (uMagic == 0x20B)
    {
        iCountAt = iOpt + 108;
        iDirAt = iOpt + 112;
    }
    else
    {
        return "optional header magic 0x" + Hex(uMagic, 1) + " is not PE32 or PE32+";
    }
    const long long iRelocAt = iDirAt + 5 * 8;
    if (iRelocAt + 8 > static_cast<long long>(uGot))
    {
        return sOutside;
    }
    if ((Le::U16(b, static_cast<size_t>(e + 22)) & 0x0001) != 0)
    {
        return "IMAGE_FILE_RELOCS_STRIPPED is set";
    }
    if ((Le::U16(b, static_cast<size_t>(iOpt + 70)) & 0x0040) == 0)
    {
        return "IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE is clear";
    }
    if (Le::U32(b, static_cast<size_t>(iCountAt)) < 6)
    {
        return "no base relocation data directory";
    }
    if (Le::U32(b, static_cast<size_t>(iRelocAt)) == 0 || Le::U32(b, static_cast<size_t>(iRelocAt + 4)) == 0)
    {
        return "base relocation directory is empty";
    }
    return std::string();
}

BinaryRow FFmpegLicenceCheck(const std::string& sConfig, const std::string& sComponents, const std::string& sNotice,
                             const std::string& sChanges, std::string& sError)
{
    static const RegexPattern kFlag("(?m)^#define CONFIG_(?:GPL|NONFREE|GPLV3|VERSION3|LGPLV3) 1\\b[^\\r\\n]*", false);
    static const RegexPattern kLine(
        "(?m)^#define FFMPEG_CONFIGURATION \"[^\\r\\n]*--enable-(?:gpl|nonfree|version3)\\b[^\\r\\n]*", false);
    static const RegexPattern kComponent(
        "(?m)^#define CONFIG_(?!(?:BINK_DECODER|BINKAUDIO_DCT_DECODER|BINKAUDIO_RDFT_DECODER|BINK_DEMUXER) )\\w+_(?:"
        "DECODER|DEMUXER|PARSER|BSF|PROTOCOL|MUXER|ENCODER|FILTER|INDEV|OUTDEV|HWACCEL) 1\\b[^\\r\\n]*",
        false);
    Scan k(sError);
    std::vector<std::string> kBad;
    if (sConfig.empty())
    {
        kBad.push_back("config.h missing or empty");
    }
    if (sComponents.empty())
    {
        kBad.push_back("config_components.h missing or empty");
    }
    const std::string sText = sConfig + "\n" + sComponents;
    const RegexPattern* apRules[] = {&kFlag, &kLine, &kComponent};
    const char* const apcLabels[] = {"licence flag", "configure line", "component"};
    for (size_t r = 0; r < 3; ++r)
    {
        std::vector<std::string> g;
        if (k.First(*apRules[r], sText, g, 0))
        {
            kBad.push_back(std::string(apcLabels[r]) + ": " + Trim(g[0]));
        }
    }
    if (sNotice.find(kFFmpegNoticeSource) == std::string::npos)
    {
        kBad.push_back(std::string("notice: NOTICE.txt does not name ") + kFFmpegNoticeSource);
    }
    for (size_t c = 0; c < sizeof(kFFmpegChangesNeed) / sizeof(kFFmpegChangesNeed[0]); ++c)
    {
        if (sChanges.find(kFFmpegChangesNeed[c]) == std::string::npos)
        {
            kBad.push_back(std::string("changes: changes.diff lacks ") + kFFmpegChangesNeed[c]);
        }
    }
    return Row("ffmpeg-license", kBad.empty(), kBad.empty() ? std::string() : kBad[0]);
}

bool ReadClFlags(const std::string& sText, FlagMap& kFlags)
{
    std::vector<std::string> kKeys;
    const std::vector<std::string> kLines = Gate::SplitLines(sText);
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        const std::string l = StripBom(kLines[i]);
        if (!l.empty() && l[0] == '^')
        {
            kKeys = SplitOn(l.substr(1), '|');
            for (size_t k = 0; k < kKeys.size(); ++k)
            {
                kKeys[k] = Gate::UpperText(kKeys[k]);
            }
        }
        else if (!l.empty() && !kKeys.empty())
        {
            for (size_t k = 0; k < kKeys.size(); ++k)
            {
                bool bSet = false;
                for (size_t f = 0; f < kFlags.size(); ++f)
                {
                    if (Gate::EqualNoCase(kFlags[f].first, kKeys[k]))
                    {
                        kFlags[f].second = l;
                        bSet = true;
                    }
                }
                if (!bSet)
                {
                    kFlags.push_back(std::make_pair(kKeys[k], l));
                }
            }
            kKeys.clear();
        }
    }
    return true;
}

BinaryRow FFmpegFlagCheck(const FlagMap& kFlags, const std::string& sConfig, const std::string& sPlatform,
                          std::string& sError)
{
    static const RegexPattern kWd("^[/-]wd", true);
    static const RegexPattern kOptRx("^/O[d12x]$", false);
    Scan k(sError);
    std::vector<std::string> kBad;
    std::vector<std::string> kUnits;
    for (size_t i = 0; i < kFlags.size(); ++i)
    {
        kUnits.push_back(kFlags[i].first);
    }
    kUnits = Sorted(kUnits);
    if (kUnits.empty())
    {
        kBad.push_back("no FFmpegFL cl tlog lines");
    }
    const std::vector<std::string> kDebugO1 = NameList(kFFmpegDebugO1);
    std::vector<std::string> kWd4703;
    std::vector<std::string> kO1;
    for (size_t u = 0; u < kUnits.size(); ++u)
    {
        std::string sFlags;
        for (size_t f = 0; f < kFlags.size(); ++f)
        {
            if (kFlags[f].first == kUnits[u])
            {
                sFlags = kFlags[f].second;
            }
        }
        const size_t uAt = Gate::UpperText(kUnits[u]).rfind("\\SRC\\FFMPEGFL\\");
        const std::string sRel = LowerText(uAt != std::string::npos ? kUnits[u].substr(uAt + 14) : kUnits[u]);
        std::vector<std::string> kTok;
        const std::vector<std::string> kSplit = SplitOn(sFlags, ' ');
        for (size_t t = 0; t < kSplit.size(); ++t)
        {
            if (!kSplit[t].empty())
            {
                kTok.push_back(kSplit[t]);
            }
        }
        const char* const apcNeed[] = {"/sdl", "/GS", "/Gy"};
        for (size_t n = 0; n < 3; ++n)
        {
            if (!ContainsCase(kTok, apcNeed[n]))
            {
                kBad.push_back(sRel + " lacks " + apcNeed[n]);
            }
        }
        if (Gate::EqualNoCase(sPlatform, "x64"))
        {
            const char* const apcWe[] = {"/we4311", "/we4312", "/we4302", "/we4477"};
            for (size_t n = 0; n < 4; ++n)
            {
                if (!ContainsCase(kTok, apcWe[n]))
                {
                    kBad.push_back(sRel + " lacks " + apcWe[n]);
                }
            }
        }
        const char* const apcBan[] = {"/GS-", "/GL", "/fp:fast", "/fp:contract", "/Zc:throwingNew"};
        for (size_t n = 0; n < 5; ++n)
        {
            if (ContainsCase(kTok, apcBan[n]))
            {
                kBad.push_back(sRel + " has banned " + apcBan[n]);
            }
        }
        const long long iSdlAt = std::find(kTok.begin(), kTok.end(), "/sdl") - kTok.begin();
        const long long iWdAt = std::find(kTok.begin(), kTok.end(), "/wd4146") - kTok.begin();
        const long long iEnd = static_cast<long long>(kTok.size());
        if (iWdAt == iEnd)
        {
            kBad.push_back(sRel + " lacks /wd4146");
        }
        else if (iSdlAt != iEnd && iWdAt < iSdlAt)
        {
            kBad.push_back(sRel + " has /wd4146 before /sdl");
        }
        std::vector<std::string> kOpt;
        for (size_t t = 0; t < kTok.size(); ++t)
        {
            if (k.Has(kWd, kTok[t]))
            {
                if (kTok[t] == "/wd4146")
                {
                    continue;
                }
                if (kTok[t] == "/wd4703")
                {
                    kWd4703.push_back(sRel);
                    continue;
                }
                kBad.push_back(sRel + " has " + kTok[t] + ", not a named exemption");
            }
        }
        for (size_t t = 0; t < kTok.size(); ++t)
        {
            if (k.Has(kOptRx, kTok[t]))
            {
                kOpt.push_back(kTok[t]);
            }
        }
        if (ContainsCase(kOpt, "/O1"))
        {
            kO1.push_back(sRel);
        }
        const std::string sWant = !Gate::EqualNoCase(sConfig, "Debug") ? "/O2"
                                  : ContainsNoCase(kDebugO1, sRel)      ? "/O1"
                                                                        : "/Od";
        if (kOpt.size() != 1 || kOpt[0] != sWant)
        {
            kBad.push_back(sRel + " has '" + Join(kOpt, " ") + "' want " + sWant);
        }
    }
    const std::string sWd4703Want = Join(Sorted(NameList(kFFmpegWd4703)), " ");
    const std::string sWd4703Have = Join(SortUnique(kWd4703), " ");
    if (!Gate::EqualNoCase(sWd4703Have, sWd4703Want))
    {
        kBad.push_back("/wd4703 on '" + sWd4703Have + "' want '" + sWd4703Want + "'");
    }
    const std::string sO1Want = Gate::EqualNoCase(sConfig, "Debug") ? Join(Sorted(kDebugO1), " ") : std::string();
    const std::string sO1Have = Join(SortUnique(kO1), " ");
    if (!Gate::EqualNoCase(sO1Have, sO1Want))
    {
        kBad.push_back("/O1 on '" + sO1Have + "' want '" + sO1Want + "'");
    }
    std::string sDetail = "cl entries=" + std::to_string(kUnits.size());
    if (!kBad.empty())
    {
        std::vector<std::string> kHead(kBad.begin(), kBad.begin() + std::min<size_t>(4, kBad.size()));
        sDetail += "; " + Join(kHead, "; ") +
                   (kBad.size() > 4 ? " (+" + std::to_string(kBad.size() - 4) + " more)" : std::string());
    }
    const std::string sCauses = "exemptions: /wd4146 after /sdl on every file (C-FFMPEG-WD4146); /wd4703 on " +
                                Join(NameList(kFFmpegWd4703), " ") + " (C-FFMPEG-WD4703); /O1 on " +
                                Join(kDebugO1, " ") + " in Debug (C-FFMPEG-DEBUG-O1)";
    return Row("ffmpeg-flags", kBad.empty(), sDetail + "; " + sCauses);
}

BinaryRow FFmpegDllCheck(const std::string& sHeaders, const std::string& sLoadConfig, const std::string& sPlatform,
                         std::string& sError)
{
    static const RegexPattern kDll("(?m)^\\s+DLL\\s*$", true);
    static const RegexPattern kDynamic("(?m)^\\s+Dynamic base\\s*$", true);
    static const RegexPattern kNx("(?m)^\\s+NX compatible\\s*$", true);
    static const RegexPattern kRelocDir("(?m)^\\s+[0-9A-F]+ \\[\\s*([0-9A-F]+)\\] RVA \\[size\\] of Base Relocation Directory",
                                        true);
    static const RegexPattern kRelocName("(?m)^\\s+\\.reloc name\\s*$", true);
    static const RegexPattern kStripped("(?m)^\\s+Relocations stripped\\s*$", true);
    static const RegexPattern kCookie("(?m)^\\s+([0-9A-F]+) Security Cookie", true);
    static const RegexPattern kSeh("(?m)^\\s+([0-9A-F]+) Safe Exception Handler Count", true);
    Scan k(sError);
    std::vector<std::string> kBad;
    if (!k.Has(kDll, sHeaders))
    {
        kBad.push_back("not a DLL");
    }
    if (!k.Has(kDynamic, sHeaders))
    {
        kBad.push_back("Dynamic base flag missing");
    }
    if (!k.Has(kNx, sHeaders))
    {
        kBad.push_back("NX compatible flag missing");
    }
    std::vector<std::string> g;
    long long iRelocSize = 0;
    if (k.First(kRelocDir, sHeaders, g, 1))
    {
        iRelocSize = HexValue(g[1]);
    }
    if (!k.Has(kRelocName, sHeaders) || iRelocSize <= 0 || k.Has(kStripped, sHeaders))
    {
        kBad.push_back("relocations missing (reloc-dir-size=" + std::to_string(iRelocSize) + ")");
    }
    long long iCookie = 0;
    if (k.First(kCookie, sLoadConfig, g, 1))
    {
        iCookie = HexValue(g[1]);
    }
    if (iCookie == 0)
    {
        kBad.push_back("security cookie missing");
    }
    if (Gate::EqualNoCase(sPlatform, "Win32"))
    {
        long long iSeh = 0;
        if (k.First(kSeh, sLoadConfig, g, 1))
        {
            iSeh = HexValue(g[1]);
        }
        if (iSeh <= 0)
        {
            kBad.push_back("Safe Exception Handler Count=" + std::to_string(iSeh));
        }
    }
    return Row("ffmpeg-dll", kBad.empty(), Join(kBad, "; "));
}

std::vector<BinaryRow> FFmpegImportChecks(const std::string& sImports, const std::string& sPlatform,
                                          const std::string& sConfig, const std::vector<std::string>& kAsanHeap,
                                          std::string& sError)
{
    static const RegexPattern kSummary("^\\s*Summary\\s*$", false);
    static const RegexPattern kModule("^\\s+(\\S+\\.dll)\\s*$", true);
    static const RegexPattern kName("^\\s+[0-9A-F]+\\s+(\\S+)\\s*$", false);
    static const RegexPattern kHeapName(kFFmpegCrtHeapNameRx, true);
    Scan k(sError);
    std::vector<std::pair<std::string, std::vector<std::string> > > kMods;
    int iCur = -1;
    const std::vector<std::string> kLines = SplitCrLf(sImports);
    std::vector<std::string> g;
    for (size_t i = 0; i < kLines.size(); ++i)
    {
        if (k.Has(kSummary, kLines[i]))
        {
            break;
        }
        if (k.First(kModule, kLines[i], g, 1))
        {
            const std::string sMod = LowerText(g[1]);
            iCur = -1;
            for (size_t m = 0; m < kMods.size(); ++m)
            {
                if (kMods[m].first == sMod)
                {
                    iCur = static_cast<int>(m);
                }
            }
            if (iCur < 0)
            {
                kMods.push_back(std::make_pair(sMod, std::vector<std::string>()));
                iCur = static_cast<int>(kMods.size()) - 1;
            }
        }
        else if (iCur >= 0 && k.First(kName, kLines[i], g, 1))
        {
            kMods[iCur].second.push_back(g[1]);
        }
    }
    std::vector<std::pair<std::string, std::string> > kCaused;
    kCaused.push_back(std::make_pair(std::string("bcrypt.dll"), std::string("C-FFMPEG-BCRYPT")));
    if (Gate::EqualNoCase(sConfig, "Asan"))
    {
        if (sPlatform == "Win32")
        {
            kCaused.push_back(std::make_pair(std::string("clang_rt.asan_dynamic-i386.dll"),
                                             std::string("C-FFMPEG-ASAN-IMPORTS")));
            kCaused.push_back(std::make_pair(std::string("api-ms-win-crt-heap-l1-1-0.dll"),
                                             std::string("C-FFMPEG-ASAN-IMPORTS")));
        }
        else if (sPlatform == "x64")
        {
            kCaused.push_back(std::make_pair(std::string("clang_rt.asan_dynamic-x86_64.dll"),
                                             std::string("C-FFMPEG-ASAN-IMPORTS")));
            kCaused.push_back(std::make_pair(std::string("api-ms-win-crt-heap-l1-1-0.dll"),
                                             std::string("C-FFMPEG-ASAN-IMPORTS")));
            kCaused.push_back(
                std::make_pair(std::string("vcruntime140_1.dll"), std::string("C-FFMPEG-ASAN-IMPORTS")));
        }
    }
    const std::vector<std::string> kAllow = NameList(kFFmpegImportAllow);
    std::vector<std::string> kModNames;
    for (size_t m = 0; m < kMods.size(); ++m)
    {
        kModNames.push_back(kMods[m].first);
    }
    std::vector<std::string> kCausedNames;
    for (size_t c = 0; c < kCaused.size(); ++c)
    {
        kCausedNames.push_back(kCaused[c].first);
    }
    std::vector<std::string> kExtra;
    for (size_t m = 0; m < kModNames.size(); ++m)
    {
        if (!ContainsNoCase(kAllow, kModNames[m]) && !ContainsNoCase(kCausedNames, kModNames[m]))
        {
            kExtra.push_back(kModNames[m]);
        }
    }
    const bool bKernel = ContainsNoCase(kModNames, "kernel32.dll");
    std::vector<std::string> kImpBad;
    if (!bKernel)
    {
        kImpBad.push_back("kernel32.dll not found in the dumpbin /imports output (" + std::to_string(kMods.size()) +
                          " DLLs parsed)");
    }
    if (!kExtra.empty())
    {
        kImpBad.push_back("imports outside the allow list: " + Join(kExtra, " "));
    }
    std::vector<std::string> kCauseParts;
    for (size_t c = 0; c < kCaused.size(); ++c)
    {
        if (ContainsNoCase(kModNames, kCaused[c].first))
        {
            kCauseParts.push_back(kCaused[c].first + " (" + kCaused[c].second + ")");
        }
    }
    std::vector<std::string> kImpDetail = kImpBad;
    kImpDetail.push_back("caused: " + Join(kCauseParts, ", "));
    std::vector<BinaryRow> kRows;
    kRows.push_back(Row("ffmpeg-imports", kImpBad.empty(), Join(kImpDetail, "; ")));
    std::vector<std::string> kHeap;
    for (size_t m = 0; m < kMods.size(); ++m)
    {
        for (size_t f = 0; f < kMods[m].second.size(); ++f)
        {
            if (Gate::EqualNoCase(kMods[m].first, kFFmpegCrtHeapModule) || k.Has(kHeapName, kMods[m].second[f]))
            {
                kHeap.push_back(kMods[m].first + "!" + kMods[m].second[f]);
            }
        }
    }
    const bool bAsan = Gate::EqualNoCase(sConfig, "Asan");
    std::vector<std::string> kHeapBad;
    if (bAsan)
    {
        if (kAsanHeap.empty())
        {
            kHeapBad.push_back("no vcasan.obj heap imports read");
        }
        for (size_t h = 0; h < kHeap.size(); ++h)
        {
            if (!ContainsCase(kAsanHeap, SplitOn(kHeap[h], '!')[1]))
            {
                kHeapBad.push_back(kHeap[h] + " is not a vcasan.obj import");
            }
        }
    }
    else
    {
        kHeapBad = kHeap;
    }
    const std::string sHeapCause =
        bAsan ? "C-FFMPEG-ALLOC-CONFIG, C-FFMPEG-VENDOR-SCOPE, C-FFMPEG-ASAN-IMPORTS (vcasan.obj imports " +
                    Join(kAsanHeap, " ") + ")"
              : std::string("C-FFMPEG-ALLOC-CONFIG, C-FFMPEG-VENDOR-SCOPE");
    const std::string sHeapDetail =
        (kHeapBad.empty() ? std::string() : "CRT heap imports: " + Join(kHeapBad, " ") + "; ") + sHeapCause;
    kRows.push_back(Row("ffmpeg-crt-heap", kHeapBad.empty() && bKernel, sHeapDetail));
    return kRows;
}

bool AsanHeapNames(const std::string& sSymbols, const std::string& sPlatform, std::vector<std::string>& kOut,
                   std::string& sError)
{
    static const RegexPattern kHeapName(kFFmpegCrtHeapNameRx, true);
    const std::string sPre = sPlatform == "Win32" ? "__imp__" : "__imp_";
    const RegexPattern kUndef("(?m)\\sUNDEF\\s.*\\|\\s+" + RegexEscape(sPre) + "(\\w+)\\s*$", false);
    Scan k(sError);
    const std::vector<std::string> kAll = k.All(kUndef, sSymbols, 1);
    std::vector<std::string> kHeap;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (k.Has(kHeapName, kAll[i]))
        {
            kHeap.push_back(kAll[i]);
        }
    }
    kOut = SortUnique(kHeap);
    return !k.Failed();
}

std::vector<std::string> BinaryRowNames(const std::string& sPlatform)
{
    const char* const apcWin32[] = {"dynamicbase",
                                    "nxcompat",
                                    "laa",
                                    "reloc",
                                    "safeseh",
                                    "gs",
                                    "sdl",
                                    "flag-alignednew",
                                    "flag-inline",
                                    "flag-alignednew-tests",
                                    "flag-banned",
                                    "mimalloc-defines",
                                    "mimalloc-crt-hooks",
                                    "riderlink-map",
                                    "import-SetProcessDpiAwarenessContext",
                                    "import-SetProcessDpiAwareness",
                                    "import-SetProcessDPIAware",
                                    "import-GetDpiForWindow",
                                    "import-AdjustWindowRectExForDpi",
                                    "import-shcore",
                                    "import-modules",
                                    "import-delay-modules",
                                    "unicows-staged",
                                    "bink-gone",
                                    "aslr-pe",
                                    "aslr-pe-tests"};
    const char* const apcX64[] = {"x64-truncation", "x64-objects",      "crt-trig-route",
                                  "d3dx-imports",   "crt-float-imports", "steam-api64-staged", "fma3-off"};
    std::vector<std::string> kOut = NameList(apcWin32);
    if (sPlatform != "Win32")
    {
        const std::vector<std::string> kX64 = NameList(apcX64);
        kOut.insert(kOut.end(), kX64.begin(), kX64.end());
    }
    return kOut;
}

std::vector<std::string> FFmpegRowNames()
{
    const char* const apcNames[] = {"ffmpeg-license", "ffmpeg-flags", "ffmpeg-dll", "ffmpeg-imports", "ffmpeg-crt-heap"};
    return NameList(apcNames);
}

namespace
{

struct RunContext
{
    std::string sRoot;
    std::string sPlatform;
    std::string sConfig;
    std::string sExeDir;
    std::string sGameExe;
    std::string sObjDir;
    DiskFiles kFiles;
};

bool IsOneOf(const std::string& sRow, const char* const* ppcNames, size_t uCount)
{
    for (size_t i = 0; i < uCount; ++i)
    {
        if (sRow == ppcNames[i])
        {
            return true;
        }
    }
    return false;
}

bool RequireExe(const RunContext& c, std::string& sError)
{
    if (!Gate::FileExists(c.sGameExe))
    {
        sError = "[FAIL] hardening:exe - not found: " + c.sGameExe;
        return false;
    }
    return true;
}

bool GameDumpbin(const RunContext& c, const char* pcMode, std::string& sText, std::string& sError)
{
    std::vector<std::string> kArgs;
    kArgs.push_back(pcMode);
    kArgs.push_back(c.sGameExe);
    DumpbinRun kRun;
    if (!Dumpbin(c.sPlatform, kArgs, kRun, sError))
    {
        return false;
    }
    if (kRun.uCode != 0)
    {
        sError = "[FAIL] hardening:dumpbin - exit " + std::to_string(kRun.uCode);
        return false;
    }
    sText = kRun.sText;
    return true;
}

bool PickRow(const std::vector<BinaryRow>& kRows, const std::string& sRow, BinaryRow& kRow, std::string& sError)
{
    const BinaryRow* p = FindRow(kRows, sRow);
    if (!p)
    {
        sError = "no row " + sRow + " was produced";
        return false;
    }
    kRow = *p;
    return true;
}

bool GameCounts(const RunContext& c, ClCounts& kGame, std::string& sError)
{
    std::vector<GameProject> kProjects;
    if (!GameProjects(c.kFiles, c.sRoot, c.sPlatform, c.sConfig, kProjects, sError))
    {
        return false;
    }
    std::vector<std::string> kDirs;
    for (size_t i = 0; i < kProjects.size(); ++i)
    {
        kDirs.push_back(kProjects[i].sTlogDir);
    }
    return CountClTlogs(c.kFiles, kDirs, kGame, sError);
}

bool ReadMap(const RunContext& c, std::string& sMap)
{
    const std::string sPath = c.sObjDir + "\\BattlefrontII.map";
    sMap.clear();
    return c.kFiles.IsFile(sPath) && ReadText(c.kFiles, sPath, sMap);
}

bool ReadPrefix(const std::string& sPath, std::string& sBytes)
{
    const std::string sRoot = RepoRoot() + "\\";
    if (!Gate::StartsWithNoCase(sPath, sRoot))
    {
        return false;
    }
    FILE* pFile = OpenRepoFile(sPath.substr(sRoot.size()));
    if (!pFile)
    {
        return false;
    }
    char acBuf[4096];
    const size_t uRead = fread(acBuf, 1, sizeof(acBuf), pFile);
    const bool bOk = ferror(pFile) == 0;
    fclose(pFile);
    sBytes.assign(acBuf, uRead);
    return bOk;
}

bool FlagRow(const RunContext& c, const std::string& sRow, BinaryRow& kRow, std::string& sError)
{
    ClCounts kGame;
    if (!GameCounts(c, kGame, sError))
    {
        return false;
    }
    const std::string sTestsDir = c.sRoot + "\\obj\\Tests\\" + c.sPlatform + "\\" + c.sConfig + "\\OpenSWBF2Tests.tlog";
    ClCounts kTests;
    const bool bTests = c.kFiles.IsDir(sTestsDir);
    if (bTests && !CountClTlogs(c.kFiles, std::vector<std::string>(1, sTestsDir), kTests, sError))
    {
        return false;
    }
    if (sRow == "flag-alignednew-tests" && !bTests)
    {
        kRow = SkipRow(sRow, "tests not built");
        return true;
    }
    ClCounts kLuac;
    bool bLuac = false;
    if (c.sPlatform == "x64")
    {
        const std::string sLuacDir = c.sObjDir + "\\LuaCompiler\\LuaCompiler.tlog";
        bLuac = c.kFiles.IsDir(sLuacDir);
        if (bLuac && !CountClTlogs(c.kFiles, std::vector<std::string>(1, sLuacDir), kLuac, sError))
        {
            return false;
        }
    }
    AllowFile kAllow;
    if (!ReadAllowFile(kAllow, sError))
    {
        return false;
    }
    const std::vector<BinaryRow> kRows = FlagChecks(kGame, bTests ? &kTests : 0, c.sConfig, c.sConfig,
                                                    bLuac ? &kLuac : 0, c.sPlatform, kAllow, sError);
    return sError.empty() && PickRow(kRows, sRow, kRow, sError);
}

bool ImportRow(const RunContext& c, const std::string& sRow, BinaryRow& kRow, std::string& sError)
{
    std::string sImports;
    if (!RequireExe(c, sError) || !GameDumpbin(c, "/imports", sImports, sError))
    {
        return false;
    }
    if (sRow == "unicows-staged" || sRow == "bink-gone")
    {
        const bool bUnicows = sRow == "unicows-staged";
        const std::string sStaged = c.sExeDir + (bUnicows ? "\\unicows.dll" : "\\binkw32.dll");
        const bool bStaged = c.kFiles.IsFile(sStaged) || c.kFiles.IsDir(sStaged);
        kRow = bUnicows ? UnicowsBuildCheck(c.sExeDir, bStaged, sImports, sError)
                        : BinkwBuildCheck(c.sExeDir, bStaged, sImports, sError);
        return sError.empty();
    }
    if (sRow == "steam-api64-staged")
    {
        const std::string sDll = c.sExeDir + "\\steam_api64.dll";
        const bool bStaged = c.kFiles.IsFile(sDll);
        std::string sSha;
        std::string sPin;
        std::string sExports;
        std::string sDef;
        if (bStaged)
        {
            std::string sPins;
            if (!ReadText(c.kFiles, c.sRoot + "\\lib\\vendor-pins.props", sPins))
            {
                sError = "cannot read lib\\vendor-pins.props";
                return false;
            }
            bool bPins = true;
            const std::map<std::string, std::string> kPins = VendorPinValues(sPins, sError, bPins);
            if (!bPins)
            {
                return false;
            }
            const std::map<std::string, std::string>::const_iterator it = kPins.find("steamapi64sha256");
            sPin = it == kPins.end() ? std::string() : it->second;
            if (!Gate::FileSha(sDll, sSha))
            {
                sError = "cannot hash " + sDll;
                return false;
            }
            sSha = LowerText(sSha);
            if (sSha == sPin)
            {
                std::vector<std::string> kArgs;
                kArgs.push_back("/exports");
                kArgs.push_back(sDll);
                DumpbinRun kRun;
                if (!Dumpbin("x64", kArgs, kRun, sError))
                {
                    return false;
                }
                if (kRun.uCode != 0)
                {
                    kRow = Row("steam-api64-staged", false, "dumpbin /exports exit " + std::to_string(kRun.uCode));
                    return true;
                }
                sExports = kRun.sText;
                if (!ReadText(c.kFiles, c.sRoot + "\\lib\\x64\\steam_api64.def", sDef))
                {
                    sError = "cannot read lib\\x64\\steam_api64.def";
                    return false;
                }
            }
        }
        kRow = SteamApi64StagedCheck(sDll, bStaged, sSha, sPin, sExports, sImports, sDef, sError);
        return sError.empty();
    }
    AllowFile kAllow;
    if (!ReadAllowFile(kAllow, sError))
    {
        return false;
    }
    const std::vector<std::string>* pStatic = FindAllow(kAllow, "ImportStaticAllow " + c.sPlatform);
    const std::vector<std::string>* pDelay = FindAllow(kAllow, "ImportDelayAllow " + c.sPlatform);
    if (!pStatic || !pDelay)
    {
        sError = "allow.txt has no import lists for " + c.sPlatform;
        return false;
    }
    const std::vector<BinaryRow> kRows = ImportChecks(sImports, c.sPlatform, *pStatic, *pDelay, sError);
    return sError.empty() && PickRow(kRows, sRow, kRow, sError);
}

bool CrtTrigRouteRow(const RunContext& c, BinaryRow& kRow, std::string& sError)
{
    std::vector<GameProject> kProjects;
    if (!GameProjects(c.kFiles, c.sRoot, c.sPlatform, c.sConfig, kProjects, sError))
    {
        return false;
    }
    const std::string sLinkTlog = kProjects[0].sTlogDir + "\\link.command.1.tlog";
    std::string sLinkText;
    if (!c.kFiles.IsFile(sLinkTlog) || !ReadText(c.kFiles, sLinkTlog, sLinkText))
    {
        kRow = Row("crt-trig-route", false, "no link tlog " + sLinkTlog);
        return true;
    }
    std::vector<std::string> kObjects;
    if (!LinkedObjects(sLinkText, kObjects))
    {
        kRow = Row("crt-trig-route", false, "no objects in " + sLinkTlog);
        return true;
    }
    std::string sSymbols;
    for (size_t k = 0; k < kObjects.size(); k += 100)
    {
        std::vector<std::string> kArgs(1, "/symbols");
        kArgs.insert(kArgs.end(), kObjects.begin() + k, kObjects.begin() + std::min(kObjects.size(), k + 100));
        DumpbinRun kRun;
        if (!Dumpbin(c.sPlatform, kArgs, kRun, sError))
        {
            return false;
        }
        if (kRun.uCode != 0)
        {
            kRow = Row("crt-trig-route", false, "dumpbin /symbols exit " + std::to_string(kRun.uCode));
            return true;
        }
        sSymbols += kRun.sText + "\n";
    }
    kRow = CrtTrigRouteCheck(kObjects, sSymbols, sError);
    return sError.empty();
}

bool Fma3OffRow(const RunContext& c, BinaryRow& kRow, std::string& sError)
{
    if (!RequireExe(c, sError))
    {
        return false;
    }
    if (!Gate::EqualNoCase(c.sConfig, "Release"))
    {
        kRow = SkipRow("fma3-off", "judged on the Release exe, smoke configuration is " + c.sConfig);
        return true;
    }
    const std::string sMapPath = c.sObjDir + "\\BattlefrontII.map";
    std::string sMap;
    if (!c.kFiles.IsFile(sMapPath) || !ReadText(c.kFiles, sMapPath, sMap))
    {
        kRow = Row("fma3-off", false, "Could not find file '" + sMapPath + "'.");
        return true;
    }
    std::vector<std::pair<unsigned long long, unsigned long long> > kRanges;
    std::string sWhy;
    if (!Fma3OffRanges(sMap, kRanges, sWhy))
    {
        kRow = Row("fma3-off", false, sWhy);
        return true;
    }
    std::vector<std::string> kText;
    for (size_t r = 0; r < kRanges.size(); ++r)
    {
        std::vector<std::string> kArgs;
        kArgs.push_back("/nologo");
        kArgs.push_back("/disasm:nobytes");
        kArgs.push_back("/range:0x" + Hex(kRanges[r].first, 1) + ",0x" + Hex(kRanges[r].second, 1));
        kArgs.push_back(c.sGameExe);
        DumpbinRun kRun;
        if (!Dumpbin(c.sPlatform, kArgs, kRun, sError))
        {
            return false;
        }
        if (kRun.uCode != 0)
        {
            kRow = Row("fma3-off", false, "dumpbin /disasm exit " + std::to_string(kRun.uCode));
            return true;
        }
        kText.push_back(kRun.sText);
    }
    kRow = Fma3OffCheck(Join(kText, "\n"), sError);
    return sError.empty();
}

bool RunRow(const RunContext& c, const std::string& sRow, BinaryRow& kRow, std::string& sError)
{
    const bool bX64 = c.sPlatform == "x64";
    const char* const apcPe[] = {"dynamicbase", "nxcompat", "laa", "reloc", "safeseh", "gs", "sdl"};
    const char* const apcPhase52[] = {"dynamicbase", "nxcompat",           "laa",     "reloc",        "safeseh",
                                      "gs",          "mimalloc-crt-hooks", "aslr-pe", "aslr-pe-tests"};
    const char* const apcFlags[] = {"flag-alignednew", "flag-inline", "flag-alignednew-tests", "flag-banned",
                                    "mimalloc-defines", "x64-truncation"};
    const char* const apcImports[] = {"import-SetProcessDpiAwarenessContext",
                                      "import-SetProcessDpiAwareness",
                                      "import-SetProcessDPIAware",
                                      "import-GetDpiForWindow",
                                      "import-AdjustWindowRectExForDpi",
                                      "import-shcore",
                                      "import-modules",
                                      "import-delay-modules",
                                      "d3dx-imports",
                                      "crt-float-imports",
                                      "unicows-staged",
                                      "bink-gone",
                                      "steam-api64-staged"};
    if (bX64 && IsOneOf(sRow, apcPhase52, sizeof(apcPhase52) / sizeof(apcPhase52[0])))
    {
        kRow = SkipRow(sRow, "Phase 52");
        return true;
    }
    if (IsOneOf(sRow, apcPe, sizeof(apcPe) / sizeof(apcPe[0])))
    {
        std::string sHeaders;
        std::string sLoadConfig;
        if (!bX64 && (!RequireExe(c, sError) || !GameDumpbin(c, "/headers", sHeaders, sError) ||
                      !GameDumpbin(c, "/loadconfig", sLoadConfig, sError)))
        {
            return false;
        }
        ClCounts kGame;
        if (!GameCounts(c, kGame, sError))
        {
            return false;
        }
        const std::vector<BinaryRow> kRows =
            HardeningChecks(sHeaders, sLoadConfig, kGame.iTotal, kGame.iGsMinus, kGame.iSdl, sError);
        return sError.empty() && PickRow(kRows, sRow, kRow, sError);
    }
    if (IsOneOf(sRow, apcFlags, sizeof(apcFlags) / sizeof(apcFlags[0])))
    {
        if (!bX64 && !RequireExe(c, sError))
        {
            return false;
        }
        return FlagRow(c, sRow, kRow, sError);
    }
    if (IsOneOf(sRow, apcImports, sizeof(apcImports) / sizeof(apcImports[0])))
    {
        return ImportRow(c, sRow, kRow, sError);
    }
    if (sRow == "mimalloc-crt-hooks" || sRow == "riderlink-map")
    {
        if (!RequireExe(c, sError))
        {
            return false;
        }
        std::string sMap;
        ReadMap(c, sMap);
        kRow = sRow == "riderlink-map" ? RiderLinkMapCheck(sMap, c.sConfig, sError) : MiCrtHookCheck(sMap, sError);
        return sError.empty();
    }
    if (sRow == "aslr-pe" || sRow == "aslr-pe-tests")
    {
        if (!RequireExe(c, sError))
        {
            return false;
        }
        const std::string sExe = sRow == "aslr-pe" ? c.sGameExe
                                                   : c.sRoot + "\\bin\\Tests\\" + c.sPlatform + "\\" + c.sConfig +
                                                         "\\OpenSWBF2Tests.exe";
        if (!Gate::FileExists(sExe))
        {
            kRow = SkipRow(sRow, "tests not built");
            return true;
        }
        std::string sBytes;
        if (!ReadPrefix(sExe, sBytes))
        {
            sError = "cannot read " + sExe;
            return false;
        }
        const std::string sWhy = PeAslr(sBytes);
        kRow = Row(sRow, sWhy.empty(), sWhy);
        return true;
    }
    if (sRow == "x64-objects")
    {
        return X64ObjectsCheck(c.kFiles, c.sRoot, c.sPlatform, c.sConfig, kRow, sError);
    }
    if (sRow == "crt-trig-route")
    {
        return CrtTrigRouteRow(c, kRow, sError);
    }
    if (sRow == "fma3-off")
    {
        return Fma3OffRow(c, kRow, sError);
    }
    sError = "unknown hardening binary row " + sRow;
    return false;
}

RunContext NewContext(const std::string& sPlatform, const std::string& sConfig)
{
    RunContext c;
    c.sRoot = RepoRoot();
    c.sPlatform = sPlatform;
    c.sConfig = sConfig;
    c.sExeDir = c.sRoot + "\\bin\\" + sPlatform + "\\" + sConfig;
    c.sGameExe = c.sExeDir + "\\BattlefrontII.exe";
    c.sObjDir = c.sRoot + "\\obj\\" + sPlatform + "\\" + sConfig;
    return c;
}

bool FailLine(const std::string& sRow, bool bRan, std::string& sError)
{
    if (!bRan && sError.compare(0, 7, "[FAIL] ") != 0)
    {
        sError = "[FAIL] hardening:" + sRow + " - " + sError;
    }
    return bRan;
}

}

bool RunBinaryRow(const std::string& sRow, const std::string& sPlatform, const std::string& sConfig, BinaryRow& kRow,
                  std::string& sError)
{
    const RunContext c = NewContext(sPlatform, sConfig);
    if (c.sRoot.empty())
    {
        sError = "cannot resolve the repo root";
        return FailLine(sRow, false, sError);
    }
    return FailLine(sRow, RunRow(c, sRow, kRow, sError), sError);
}

bool RunFFmpegRow(const std::string& sRow, const std::string& sPlatform, const std::string& sConfig, BinaryRow& kRow,
                  std::string& sError)
{
    const RunContext c = NewContext(sPlatform, sConfig);
    if (c.sRoot.empty())
    {
        sError = "cannot resolve the repo root";
        return FailLine(sRow, false, sError);
    }
    const std::string sIntDir = c.sObjDir + "\\FFmpegFL";
    const std::string sTlogDir = sIntDir + "\\FFmpegFL.tlog";
    const std::string sDll = c.sExeDir + "\\openswbf2-ffmpeg.dll";
    const std::string sSrc = c.sRoot + "\\src\\FFmpegFL";
    if (sRow == "ffmpeg-license")
    {
        const std::string asPaths[] = {sSrc + "\\config\\" + sPlatform + "\\config.h",
                                        sSrc + "\\config\\common\\config_components.h", sSrc + "\\NOTICE.txt",
                                        sSrc + "\\changes.diff"};
        std::string asTexts[4];
        for (size_t i = 0; i < 4; ++i)
        {
            if (c.kFiles.IsFile(asPaths[i]))
            {
                ReadText(c.kFiles, asPaths[i], asTexts[i]);
            }
        }
        kRow = FFmpegLicenceCheck(asTexts[0], asTexts[1], asTexts[2], asTexts[3], sError);
        return FailLine(sRow, sError.empty(), sError);
    }
    if (sRow == "ffmpeg-flags")
    {
        std::vector<std::string> kFound;
        ListFiles(c.kFiles, sTlogDir, "cl.command*.tlog", false, kFound);
        FlagMap kFlags;
        std::string sWhy;
        if (kFound.empty())
        {
            sWhy = "cl command tlog not found in " + sTlogDir;
        }
        for (size_t i = 0; i < kFound.size(); ++i)
        {
            std::string sText;
            if (!ReadText(c.kFiles, kFound[i], sText))
            {
                sError = "cannot read " + kFound[i];
                return FailLine(sRow, false, sError);
            }
            ReadClFlags(sText, kFlags);
        }
        kRow = FFmpegFlagCheck(kFlags, sConfig, sPlatform, sError);
        if (!sWhy.empty())
        {
            kRow.sDetail = sWhy + "; " + kRow.sDetail;
        }
        return FailLine(sRow, sError.empty(), sError);
    }
    if (!c.kFiles.IsFile(sDll))
    {
        kRow = Row(sRow, false, "not found: " + sDll);
        return true;
    }
    std::string asText[3];
    const char* const apcModes[] = {"/headers", "/loadconfig", "/imports"};
    unsigned long uCode = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        std::vector<std::string> kArgs;
        kArgs.push_back("/nologo");
        kArgs.push_back(apcModes[i]);
        kArgs.push_back(sDll);
        DumpbinRun kRun;
        if (!Dumpbin(sPlatform, kArgs, kRun, sError))
        {
            return FailLine(sRow, false, sError);
        }
        if (uCode == 0)
        {
            uCode = kRun.uCode;
        }
        asText[i] = kRun.sText;
    }
    if (uCode != 0)
    {
        kRow = Row(sRow, false, "dumpbin exit " + std::to_string(uCode) + " on " + sDll);
        return true;
    }
    if (sRow == "ffmpeg-dll")
    {
        kRow = FFmpegDllCheck(asText[0], asText[1], sPlatform, sError);
        return FailLine(sRow, sError.empty(), sError);
    }
    std::vector<std::string> kAsanHeap;
    std::string sAsanWhy;
    if (sConfig == "Asan")
    {
        std::vector<std::string> kLinkReads;
        ListFiles(c.kFiles, sTlogDir, "link.read*.tlog", false, kLinkReads);
        std::vector<std::string> kLibs;
        for (size_t f = 0; f < kLinkReads.size(); ++f)
        {
            std::string sText;
            if (!ReadText(c.kFiles, kLinkReads[f], sText))
            {
                continue;
            }
            const std::vector<std::string> kLines = Gate::SplitLines(sText);
            for (size_t l = 0; l < kLines.size(); ++l)
            {
                std::string s = StripBom(kLines[l]);
                size_t uStart = 0;
                while (uStart < s.size() &&
                       (s[uStart] == '^' || s.compare(uStart, 3, "\xEF\xBB\xBF") == 0))
                {
                    uStart += s[uStart] == '^' ? 1 : 3;
                }
                const std::vector<std::string> kParts = SplitOn(s.substr(uStart), '|');
                for (size_t p = 0; p < kParts.size(); ++p)
                {
                    if (EndsWithNoCase(kParts[p], "\\VCASAN.LIB"))
                    {
                        kLibs.push_back(kParts[p]);
                    }
                }
            }
        }
        kLibs = SortUnique(kLibs);
        if (kLibs.size() != 1)
        {
            sAsanWhy = "FFmpegFL link read " + std::to_string(kLibs.size()) + " vcasan.lib paths in " + sTlogDir;
        }
        else
        {
            std::vector<std::string> kArgs;
            kArgs.push_back("/nologo");
            kArgs.push_back("/symbols");
            kArgs.push_back(kLibs[0]);
            DumpbinRun kRun;
            if (!Dumpbin(sPlatform, kArgs, kRun, sError))
            {
                return FailLine(sRow, false, sError);
            }
            if (kRun.uCode != 0)
            {
                sAsanWhy = "dumpbin /symbols " + kLibs[0] + " exit " + std::to_string(kRun.uCode);
            }
            else if (!AsanHeapNames(kRun.sText, sPlatform, kAsanHeap, sError))
            {
                return FailLine(sRow, false, sError);
            }
        }
    }
    std::vector<BinaryRow> kRows = FFmpegImportChecks(asText[2], sPlatform, sConfig, kAsanHeap, sError);
    if (!sError.empty())
    {
        return FailLine(sRow, false, sError);
    }
    if (!sAsanWhy.empty())
    {
        kRows[1].sDetail = sAsanWhy + "; " + kRows[1].sDetail;
    }
    return FailLine(sRow, PickRow(kRows, sRow, kRow, sError), sError);
}

}
