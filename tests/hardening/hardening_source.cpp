#include <windows.h>

#include <algorithm>
#include <memory>
#include <set>

#include "gate/gate_machine.h"
#include "hardening/hardening_allow.h"
#include "hardening/hardening_regex.h"
#include "hardening/hardening_source.h"
#include "hardening/hardening_xml.h"
#include "testutil.h"

namespace Hardening
{

namespace
{

bool IsSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

bool IsWord(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

char LowerAscii(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

bool EqualNoCase(const std::string& sA, const std::string& sB)
{
    return sA.size() == sB.size() && _stricmp(sA.c_str(), sB.c_str()) == 0;
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

std::string Narrow(const std::wstring& sText)
{
    if (sText.empty())
    {
        return std::string();
    }
    const int iLen = WideCharToMultiByte(CP_UTF8, 0, sText.data(), static_cast<int>(sText.size()), 0, 0, 0, 0);
    std::string sOut(static_cast<size_t>(iLen), '\0');
    WideCharToMultiByte(CP_UTF8, 0, sText.data(), static_cast<int>(sText.size()), &sOut[0], iLen, 0, 0);
    return sOut;
}

struct Span
{
    size_t uStart;
    size_t uLength;
    size_t uEnd;
    std::string sText;
};

bool AllMatches(const RegexPattern& kPattern, const std::string& sText, std::vector<Span>& kOut, std::string& sError)
{
    RegexMatcher kMatcher(kPattern, sText);
    while (kMatcher.FindNext())
    {
        Span kSpan;
        kSpan.uStart = kMatcher.GetMatchBeginning();
        kSpan.uLength = kMatcher.GetMatchLength();
        kSpan.uEnd = kSpan.uStart + (kSpan.uLength > 0 ? kSpan.uLength : 1);
        kSpan.sText = sText.substr(kSpan.uStart, kSpan.uLength);
        kOut.push_back(kSpan);
    }
    if (kMatcher.Failed())
    {
        sError = kMatcher.Error();
        return false;
    }
    return true;
}

bool IsMatch(const RegexPattern& kPattern, const std::string& sText, bool& bMatch, std::string& sError)
{
    std::string sWhy;
    bMatch = RegexIsMatch(kPattern, sText, sWhy);
    if (!sWhy.empty())
    {
        sError = sWhy;
        return false;
    }
    return true;
}

typedef std::map<std::string, int> OrdinalCounts;
typedef std::map<std::string, int, CultureLess> CultureCounts;

template <class COUNTS>
void AddUnused(const COUNTS& kLeft, std::vector<std::string>& kHits)
{
    std::vector<std::string> kKeys;
    for (typename COUNTS::const_iterator it = kLeft.begin(); it != kLeft.end(); ++it)
    {
        kKeys.push_back(it->first);
    }
    Tables::CultureSort(kKeys, true);
    for (size_t i = 0; i < kKeys.size(); ++i)
    {
        const int iLeft = kLeft.find(kKeys[i])->second;
        for (int j = 0; j < iLeft; ++j)
        {
            kHits.push_back("unused allow " + kKeys[i]);
        }
    }
}

template <class COUNTS>
bool TakeAllow(COUNTS& kLeft, const std::string& sKey)
{
    typename COUNTS::iterator it = kLeft.find(sKey);
    if (it == kLeft.end() || it->second <= 0)
    {
        return false;
    }
    --it->second;
    return true;
}

bool VerdictKey(const std::string& sEntry, std::string& sKey)
{
    static const RegexPattern kPattern(R"rx(^(.*)\|x64:(safe|fixed):(.+)$)rx", false);
    RegexMatcher kMatcher(kPattern, sEntry);
    if (kMatcher.FindNext() && !TrimSpace(kMatcher.GetCaptureGroup(3)).empty())
    {
        sKey = kMatcher.GetCaptureGroup(1);
        return true;
    }
    sKey = sEntry;
    return false;
}

const std::string kByteTypes =
    R"rx((?:char|__int8|BYTE|UCHAR|CHAR|INT8|UINT8|uint8_t|int8_t|std::uint8_t|std::int8_t|std::byte|byte))rx";

const std::string kByteDeclItem =
    R"rx(\s*((?:[*&]\s*(?:(?:const|volatile)\b\s*)*)*)(\w+)\s*((?:\[[^\]]*\]\s*)*)(?:=\s*(?:\{(?:[^{}]|\{(?:[^{}]|\{[^{}]*\})*\})*\}|[^,;{}]*)|\{(?:[^{}]|\{[^{}]*\})*\})?\s*)rx";

bool ByteDeclBefore(const std::string& sText, size_t uAt)
{
    size_t i = uAt;
    while (i > 0 && (sText[i - 1] == ' ' || sText[i - 1] == '\t'))
    {
        --i;
    }
    if (i == 0)
    {
        return true;
    }
    const char c = sText[i - 1];
    return c == '\n' || c == '{' || c == '}' || c == ';' || c == ':';
}

bool SteamIdBefore(const std::string& sText, size_t uAt)
{
    size_t i = uAt;
    while (i > 0 && IsSpace(sText[i - 1]))
    {
        --i;
    }
    if (i == uAt)
    {
        return true;
    }
    const char* const apcWords[] = {"class", "struct"};
    for (size_t w = 0; w < 2; ++w)
    {
        const size_t uLen = strlen(apcWords[w]);
        if (i >= uLen && sText.compare(i - uLen, uLen, apcWords[w]) == 0 && (i == uLen || !IsWord(sText[i - uLen - 1])))
        {
            return false;
        }
    }
    return true;
}

const RegexPattern& ByteCommentPattern()
{
    static const RegexPattern kPattern(R"rx("(?:[^"\\\r\n]|\\.)*"|'(?:[^'\\\r\n]|\\.)*'|//[^\r\n]*|/\*[\s\S]*?\*/)rx", false);
    return kPattern;
}

const RegexPattern& ByteDeclPattern()
{
    static const RegexPattern kPattern(
        R"rx((?:(?:static|const|volatile|mutable|extern|thread_local|register|constexpr|inline|typedef|__unaligned|alignas\s*\((?:[^()]|\([^()]*\))*\)|__declspec\s*\((?:[^()]|\([^()]*\))*\))\s+)*(?:(?:unsigned|signed)\s+)?)rx" +
            kByteTypes + R"rx((?:\s+(?:const|volatile))*\b(?!\s*[(:])(?<list>)rx" + kByteDeclItem + "(?:," +
            kByteDeclItem + ")*);",
        false, ByteDeclBefore);
    return kPattern;
}

const RegexPattern& ByteDeclItemPattern()
{
    static const RegexPattern kPattern("\\G" + kByteDeclItem + "(?:,|$)", false);
    return kPattern;
}

const std::string kOffsetQual = R"rx((?:(?:const|volatile)\s*)*)rx";
const std::string kOffsetByteType = R"rx((?:(?:const|volatile)\s+)*(?:(?:unsigned|signed)\s+)?)rx" + kByteTypes +
                                    R"rx((?:\s+(?:const|volatile))*\s*\*\s*)rx" + kOffsetQual;
const std::string kOffsetCall = R"rx((?<=[\w\]])\((?:[^()]|\([^()]*\))*\))rx";
const std::string kOffsetOperand =
    R"rx((?:(?:&\s*)?(?:[\w.]|->|::|\[[^\[\];]*\]|)rx" + kOffsetCall + R"rx()+|\((?:[^()]|\([^()]*\))*\)))rx";
const std::string kOffsetByteCast = R"rx((?:\(\s*)rx" + kOffsetByteType + R"rx(\)\s*)rx" + kOffsetOperand +
                                    R"rx(|\breinterpret_cast\s*<\s*)rx" + kOffsetByteType +
                                    R"rx(>\s*\((?:[^()]|\([^()]*\))*\)))rx";
const std::string kOffsetTypeCast =
    R"rx((?:\(\s*(?:(?:const|volatile|struct|class|unsigned|signed)\s+)*[A-Za-z_][\w:]*(?:\s+(?:int|char|short|long|__int\d+))?(?:\s+(?:const|volatile))*\s*[*&][\s*&]*)rx" +
    kOffsetQual + R"rx(\)|\b(?:reinterpret|static|const)_cast\s*<[^<>;()]*[*&]\s*)rx" + kOffsetQual + ">)";
const std::string kOffsetOp =
    R"rx(\s*[+-](?![+=>-]|\s*(?:\(\s*\w+\s*\)\s*)?offsetof\b|\s*(?:\w+\s*(?:->|\.)\s*)+e_lfanew\b))rx";
const std::string kOffsetTail = kOffsetOp + R"rx([^;()]*(?:\((?:[^()]|\([^()]*\))*\)[^;()]*)*\))rx";

const std::vector<RegexPattern>& OffsetPatterns()
{
    static const std::vector<RegexPattern> kPatterns = {
        RegexPattern(kOffsetTypeCast + R"rx(\s*\(\s*)rx" + kOffsetByteCast + kOffsetTail, false),
        RegexPattern(kOffsetTypeCast + R"rx(\s*\(\s*(?!\d)(?:[\w.]|->|::|\[[^\[\];]*\]|)rx" + kOffsetCall + ")+" +
                         kOffsetTail,
                     false),
        RegexPattern(kOffsetByteCast + kOffsetOp + R"rx(\s*(?:0[xX][0-9A-Fa-f]+|\d+)[uUlL]*\b)rx", false),
    };
    return kPatterns;
}

const std::string kSizeLiteral = R"rx((?:0[xX][0-9A-Fa-f]+|[1-9]\d*)[uUlL]*)rx";
const std::string kSizeLiteralArg = R"rx((?:[^,();]|\((?:[^()]|\([^()]*\))*\))+)rx";
const std::string kSizeFourArg = R"rx((?:4[uUlL]*\s*\*)rx" + kSizeLiteralArg +
                                 R"rx(|(?:[^,();]|\((?:[^()]|\([^()]*\))*\))+?[*+]\s*4[uUlL]*(?=\s*[,)])))rx";
const std::string kSizeArg = "(?:" + kSizeLiteral + "|" + kSizeFourArg + ")";

std::vector<RegexPattern> SizeLiteralPatterns(const std::string& sSize, const std::string& sAlloc)
{
    std::vector<RegexPattern> kOut;
    kOut.push_back(RegexPattern(R"rx(\bMemoryPool\s+[\w:]+\s*\(\s*"[^"\r\n]*"\s*,\s*)rx" + sSize + R"rx(\s*\))rx", false));
    kOut.push_back(RegexPattern(R"rx(\bFreeList\s+[\w:]+\s*\(\s*)rx" + sSize + R"rx(\s*,)rx", false));
    kOut.push_back(RegexPattern(R"rx((?:\.|->)(?:Setup|Create)\s*\()rx" + kSizeLiteralArg + R"rx(,\s*)rx" + sSize +
                                    R"rx(\s*,)rx" + kSizeLiteralArg + R"rx(\))rx",
                                false));
    kOut.push_back(RegexPattern(R"rx(\bSetupMemoryPool\s*\()rx" + kSizeLiteralArg + R"rx(,\s*)rx" + sSize + R"rx(\s*,)rx",
                                false));
    kOut.push_back(RegexPattern(R"rx(\bUpdateMemoryPoolSize\s*\(\s*)rx" + sSize + R"rx(\s*\))rx", false));
    kOut.push_back(RegexPattern(R"rx(\bRed(?:Alloc|Realloc)\w*\s*\((?:)rx" + kSizeLiteralArg + R"rx(,)*\s*)rx" + sAlloc +
                                    R"rx(\s*[,)])rx",
                                false));
    return kOut;
}

const std::vector<RegexPattern>& SizeLiteralBase()
{
    static const std::vector<RegexPattern> kPatterns = SizeLiteralPatterns(kSizeArg, kSizeFourArg);
    return kPatterns;
}

const RegexPattern& SizeFourLocalPattern()
{
    static const RegexPattern kPattern(
        R"rx(\b(?<name>[A-Za-z_]\w*)\s*=(?!=)\s*(?:4[uUlL]*\s*\*[^;{}]+|[^;{}=]+?[*+]\s*4[uUlL]*)\s*;)rx", false);
    return kPattern;
}

std::string RegexEscape(const std::string& sText)
{
    std::string sOut;
    for (size_t i = 0; i < sText.size(); ++i)
    {
        if (!IsWord(sText[i]))
        {
            sOut += '\\';
        }
        sOut += sText[i];
    }
    return sOut;
}

struct RegexRuleDef
{
    std::shared_ptr<RegexPattern> pPath;
    std::shared_ptr<RegexPattern> pPattern;
};

struct RegexRowDef
{
    std::string sName;
    std::shared_ptr<RegexPattern> pPath;
    bool bTests;
    std::string sNote;
    std::string sAllow;
    std::vector<RegexRuleDef> kRules;
};

std::shared_ptr<RegexPattern> Path(const char* pcPattern)
{
    return std::make_shared<RegexPattern>(pcPattern, true);
}

RegexRowDef Row(const char* pcName, const char* pcPath, bool bTests, const char* pcNote, const char* pcAllow,
                const std::vector<std::string>& kPatterns)
{
    RegexRowDef kRow;
    kRow.sName = pcName;
    if (pcPath)
    {
        kRow.pPath = Path(pcPath);
    }
    kRow.bTests = bTests;
    kRow.sNote = pcNote ? pcNote : "";
    kRow.sAllow = pcAllow ? pcAllow : "";
    for (size_t i = 0; i < kPatterns.size(); ++i)
    {
        RegexRuleDef kRule;
        kRule.pPattern = std::make_shared<RegexPattern>(kPatterns[i], false);
        kRow.kRules.push_back(kRule);
    }
    return kRow;
}

std::vector<RegexRowDef> BuildRegexRows()
{
    const std::string sAoT = R"rx(unsigned\s+int|unsigned\s+long|unsigned|int|long|size_t|uintptr_t|intptr_t|UINT_PTR|DWORD_PTR|DWORD|uint32_t)rx";
    const std::string sAoP = R"rx((?:this|(?:\w+(?:\.|->))*m?_?p{1,2}[A-Z]\w*))rx";
    const std::string sAoC = R"rx((?:const\s+)?(?:unsigned\s+|signed\s+)?(?:char|void)\s*\*\s*(?:const\s*)?)rx";
    std::vector<RegexRowDef> kRows;
    kRows.push_back(Row("new-size-t", 0, true, 0, 0,
                        {R"rx(\boperator\s*(new|delete)\s*(\[\s*\])?\s*\(\s*(?:void\s*\*\s*\w*\s*,\s*)?(?:unsigned(?:\s+int)?|UINT|uint32_t|unsigned\s+__int32)\b)rx"}));
    kRows.push_back(Row(
        "win32-callbacks", 0, false, 0, 0,
        {R"rx(\(\s*(?:[A-Z_]*PROC[A-Z]*|L?P?THREAD_START_ROUTINE|PAPCFUNC|PTIMERAPCROUTINE)\s*\))rx",
         R"rx(_cast\s*<\s*(?:[A-Z_]*PROC[A-Z]*|L?P?THREAD_START_ROUTINE|PAPCFUNC|PTIMERAPCROUTINE)\s*>)rx",
         R"rx(\b(?:\w+::)*(?!\w*Timer)\w*Proc\s*\(\s*HWND\w*\s*\*?\s*\w*\s*,(?!\s*UINT\b\s*\w*\s*,\s*WPARAM\b\s*\w*\s*,\s*LPARAM\b\s*\w*\s*\))[^,()]*,[^,()]*,[^,()]*\))rx",
         R"rx((?:CALLBACK|WINAPI|APIENTRY|__stdcall)\s+(?:\w+::)*(?!\w*Timer)\w+\s*\(\s*HWND\w*\s*\*?\s*\w*\s*,\s*(?:UINT|unsigned)\b(?!\s*\w*\s*,\s*WPARAM\b\s*\w*\s*,\s*LPARAM\b\s*\w*\s*\))[^,()]*,[^,()]*,[^,()]*\))rx",
         R"rx(\b(?!LRESULT\b|INT_PTR\b)\w+\s+(?:CALLBACK|WINAPI|APIENTRY|__stdcall)\s+(?:\w+::)*(?!\w*Timer)\w*Proc\s*\(\s*HWND\w*\s*\*?\s*\w*\s*,[^,()]*,[^,()]*,[^,()]*\))rx",
         R"rx(\b(?:\w+::)*\w*Proc\s*\(\s*int\b\s*\w*\s*,(?!\s*WPARAM\b\s*\w*\s*,\s*LPARAM\b\s*\w*\s*\))[^,()]*,[^,()]*\))rx",
         R"rx(\b(?!LRESULT\b)\w+\s+(?:CALLBACK|WINAPI|APIENTRY|__stdcall)\s+(?:\w+::)*\w*Proc\s*\(\s*int\b\s*\w*\s*,[^,()]*,[^,()]*\))rx"}));
    kRows.push_back(Row("galaxy-vtable", R"rx(\\src\\Steam\\)rx", false, 0, 0,
                        {R"rx(\(\s*(?:const\s+)?\w+(?:\s+\w+)?\s*(?:const\s*)?\*\s*(?:const\s*)?\*[\s*]*\))rx",
                         R"rx(_cast\s*<\s*(?:const\s+)?\w+(?:\s+\w+)?\s*(?:const\s*)?\*\s*(?:const\s*)?\*[\s*]*>)rx",
                         R"rx(__thiscall\s*\*)rx"}));
    kRows.push_back(Row(
        "fastcall-edx", 0, false, 0, "FastcallAllow",
        {R"rx(__fastcall\s*\*?\s*(?:\w+::)*\w*\s*\)?\s*\(\s*[^,()]+?,\s*(?:const\s+)?void\s*\*\s*(?:/\*[^*]*\*/\s*)?(?:\w*(?:[Ee]dx|EDX|[Uu]nused|[Dd]ummy|[Ii]gnored)\w*)?\s*[,)])rx",
         R"rx(__fastcall\s*\*?\s*(?:\w+::)*\w*\s*\)?\s*\(\s*(?:[^,()]+?,\s*)?(?:const\s+)?(?:int|unsigned(?:\s+int)?|void\s*\*)\s*(?:(?:/\*[^*]*\*/\s*)?\w*(?:[Ee]dx|EDX|[Uu]nused|[Dd]ummy|[Ii]gnored)\w*|/\*[^*]*(?:[Ee]dx|EDX|[Uu]nused|[Dd]ummy|[Ii]gnored)[^*]*\*/)\s*[,)])rx",
         R"rx(__fastcall\s*\*?\s*(?:\w+::)*\w*\s*\)?\s*\(\s*int\s+iLocalIndex\s*,)rx",
         R"rx(__fastcall\s*\*?\s*(?:\w+::)*\w*\s*\)?\s*\(\s*(?:const\s+)?\w+\s*\*\s*\w*\s*,\s*int\s*,\s*unsigned\b)rx",
         R"rx(__fastcall\s+(?:\w+::)*\w+\s*\((?=[^(){};]*\)\s*\{)\s*(?:(?:const\s+)?[\w:]+(?:\s+(?:int|char|short|long))?(?:\s*[*&]+\s*|\s+)(?!(?:int|char|short|long|const)\b)\w+\s*,\s*int|int\s*,\s*(?:const\s+)?[\w:]+(?:\s+(?:int|char|short|long))?(?:\s*[*&]+\s*|\s+)(?!(?:int|char|short|long|const)\b)\w+)\s*[,)])rx"}));
    kRows.push_back(Row(
        "d3dx-decl", R"rx(\\src\\(?!PebbleFL\\pc\\pcD3dxLinkX64\.cpp$))rx", false, 0, 0,
        {R"rx(extern\s*"C"[^;{}]*?\bD3DX\w*\s*\()rx",
         R"rx((?:\b_{1,2}stdcall|\bWINAPI|\bAPIENTRY|\bCALLBACK|\bSTDMETHODCALLTYPE|\bSTDAPICALLTYPE|\bNTAPI|\bPASCAL)\b(?:\s|/\*.*?\*/)*(?:(?:const\s+)?[\w:]+(?:\s*\*+\s*|\s+)(?:/\*.*?\*/\s*)*)?D3DX\w*\s*\()rx"}));
    kRows.push_back(Row("gamma-get", 0, false, 0, 0, {"GetDeviceGammaRamp"}));
    kRows.push_back(Row("addr-order", R"rx(\\src\\(?:Battlefront2|PebbleFL)\\)rx", false, 0, "AddrOrderAllow",
                        {R"rx(\(\s*(?:)rx" + sAoT + R"rx()\s*\)\s*\(?\s*this\b)rx",
                         R"rx(\(\s*(?:)rx" + sAoT + R"rx()\s*\)\s*(?:\(\s*(?:size_t|uintptr_t)\s*\)\s*)?)rx" + sAoP +
                             R"rx(\s*[<>]=?[^<>=])rx",
                         R"rx([<>]=?\s*\(\s*(?:)rx" + sAoT + R"rx()\s*\)\s*(?:\(\s*(?:size_t|uintptr_t)\s*\)\s*)?)rx" + sAoP +
                             R"rx(\b(?!\s*(?:->|\.|\[|\()))rx",
                         R"rx(reinterpret_cast\s*<\s*(?:)rx" + sAoT + R"rx()\s*>\s*\(\s*)rx" + sAoP + R"rx(\s*\)\s*[<>])rx",
                         R"rx([<>]=?\s*reinterpret_cast\s*<\s*(?:)rx" + sAoT + R"rx()\s*>\s*\(\s*)rx" + sAoP + R"rx(\s*\))rx",
                         R"rx(\(\s*)rx" + sAoC + R"rx(\)\s*)rx" + sAoP + R"rx(\s*[<>]=?[^<>=])rx",
                         R"rx([<>]=?\s*\(\s*)rx" + sAoC + R"rx(\)\s*)rx" + sAoP + R"rx(\b(?!\s*(?:->|\.|\[|\()))rx",
                         R"rx((?:\.|->)(?:Add|Move)\s*\([^;]*\(\s*(?:)rx" + sAoT + R"rx()\s*\)\s*\(?\s*)rx" + sAoP +
                             R"rx(\b(?!\s*(?:->|\.|\[|\()))rx",
                         R"rx(\bstd::(?:less|greater)(?:_equal)?\s*<[^<>;]*\*\s*>)rx", R"rx(\bPblPtr(?:OrderKey|After)\b)rx",
                         R"rx(\b(?:this|m?_?p{1,2}[A-Z]\w*)\s*[<>]=?\s*(?:this|m?_?p{1,2}[A-Z]\w*)\b)rx"}));
    kRows.push_back(Row("mi-api-ban", R"rx(\\src\\(?!.*\\RedMemoryMi\.cpp$))rx", false, 0, 0, {R"rx(\bmi_\w+\s*\()rx"}));
    kRows.push_back(Row(
        "crt-heap-ban", R"rx(\\src\\(?!PebbleFL\\Common\\PblCrtHeap\.cpp$))rx", false, 0, 0,
        {R"rx((?<![\w.>:~])(?:malloc|calloc|realloc|free|_strdup|strdup|_wcsdup|wcsdup|_msize|_expand|_recalloc|_aligned_malloc|_aligned_offset_malloc|_aligned_realloc|_aligned_free)\s*\()rx"}));
    kRows.push_back(Row("snd-alloc-size-t", R"rx(\\src\\(?:SoundFL\\|Battlefront2\\Source\\GameSoundEngine\.(?:cpp|h)$))rx",
                        false, 0, 0,
                        {R"rx(\b(?:SndAlloc|SndTempAlloc)\s*\(\s*unsigned\b)rx",
                         R"rx(\(\s*__cdecl\s*\*\s*\w*Alloc\w*\s*\)\s*\(\s*unsigned\b)rx"}));
    kRows.push_back(Row("galaxy-getter-cast", R"rx(\\src\\)rx", false, 0, 0,
                        {R"rx(\(galaxy::api::I\w+\s*\*\)\s*\(?\s*Get\w*Galaxy)rx", R"rx(void\*\s+\w+\s*=\s*Get\w*Galaxy)rx",
                         R"rx(void\s*\*\s*\w*Get\w*Galaxy\w*\s*\()rx"}));
    kRows.push_back(Row("galaxy-factory-seam", R"rx(\\src\\(?!Steam\\gfVendor\.cpp$))rx", false, 0, 0,
                        {R"rx(\bGalaxyFactory\s*::\s*\w+\s*\()rx"}));
    RegexRowDef kSteamId = Row(
        "steamid-abi", 0, false, "(settle the x64 CSteamID hidden-return and by-value convention first)", 0,
        {R"rx((?:->|\.|::)\s*(?:GetSteamID|InitiateGameConnection|BeginAuthSession|EndAuthSession|UserHasLicenseForApp|AdvertiseGame)\s*\()rx"});
    RegexRuleDef kByValue;
    kByValue.pPath = Path(R"rx(\\src\\(?!Steam\\steamworks\.h$))rx");
    kByValue.pPattern = std::make_shared<RegexPattern>(R"rx(\bCSteamID\b(?!\s*[*&]))rx", false, SteamIdBefore);
    kSteamId.kRules.push_back(kByValue);
    kRows.push_back(kSteamId);
    kRows.push_back(Row(
        "handle-raw", R"rx(\\src\\(?!PebbleFL\\Common\\PblHandle\.(?:h|cpp)$))rx", false, 0, 0,
        {R"rx(\bPBL_HANDLE_ID\b)rx", R"rx(\bm(?:Saved)?HandleId\b)rx",
         R"rx((?i)(?:[!=]=\s*(?:\(\s*[\w:]+(?:\s+[\w:]+)*\s*\*?\s*\)\s*)?0xd{8}[ul]*\b|\b0xd{8}[ul]*\s*[!=]=|\*\s*\)\s*0xd{8}[ul]*\b|_cast\s*<[^<>;]*>\s*\(\s*0xd{8}[ul]*\b))rx",
         R"rx(\bGetHandleId\s*\(\s*\)\s*[!=]=(?!\s*\(unsigned int\)\s*debug\w*Id\b)(?!\s*net\w*Trace\w*\b)(?!\s*[1-9]\d*\b))rx",
         R"rx([!=]=\s*(?:\w+(?:\(\s*\))?(?:\.|->))*(?:\w+::)?GetHandleId\s*\()rx",
         R"rx((?:&&|\|\||\b(?:if|while)\s*\(|!)\s*(?:\w+(?:\(\s*\))?(?:\.|->))*(?:\w+::)?GetHandleId\s*\(\s*\)\s*(?:\)|&&|\|\||\?))rx",
         R"rx(\bGameObjectHandleId\b)rx"}));
    return kRows;
}

const std::vector<RegexRowDef>& RegexRows()
{
    static const std::vector<RegexRowDef> kRows = BuildRegexRows();
    return kRows;
}

const RegexRowDef* FindRegexRow(const std::string& sRow)
{
    const std::vector<RegexRowDef>& kRows = RegexRows();
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        if (kRows[i].sName == sRow)
        {
            return &kRows[i];
        }
    }
    return 0;
}

const RegexPattern& VendorPattern()
{
    static const RegexPattern kPattern(R"rx(\\src\\(LuaFL|ZlibFL|mimalloc|FFmpegFL)\\)rx", true);
    return kPattern;
}

const RegexPattern& TestsThirdPartyPattern()
{
    static const RegexPattern kPattern(R"rx(\\tests\\third_party\\)rx", true);
    return kPattern;
}

bool SourceExtension(const std::string& sName)
{
    const size_t uDot = sName.find_last_of('.');
    if (uDot == std::string::npos)
    {
        return false;
    }
    const char* const apcExt[] = {".h", ".hpp", ".inl", ".c", ".cpp"};
    for (size_t i = 0; i < sizeof(apcExt) / sizeof(apcExt[0]); ++i)
    {
        if (_stricmp(sName.c_str() + uDot, apcExt[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

std::string LeafName(const std::string& sRel)
{
    const size_t uSlash = sRel.find_last_of('\\');
    return uSlash == std::string::npos ? sRel : sRel.substr(uSlash + 1);
}

std::string JoinRel(const std::string& sDir, const std::string& sName)
{
    return sDir.empty() ? sName : sDir + "\\" + sName;
}

std::string FullPath(const std::string& sRoot, const std::string& sRel)
{
    return sRoot + "\\" + sRel;
}

bool WalkFiles(const TreeSource& kTree, const std::string& sRelDir, bool bRecurse, bool bMustExist,
               std::vector<std::string>& kOut, std::string& sError)
{
    std::vector<RepoEntry> kEntries;
    if (!kTree.List(sRelDir, kEntries))
    {
        if (bMustExist)
        {
            sError = "cannot list " + sRelDir;
            return false;
        }
        return true;
    }
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        if (!kEntries[i].bDir)
        {
            kOut.push_back(JoinRel(sRelDir, kEntries[i].sName));
        }
    }
    if (!bRecurse)
    {
        return true;
    }
    for (size_t i = 0; i < kEntries.size(); ++i)
    {
        if (kEntries[i].bDir && !WalkFiles(kTree, JoinRel(sRelDir, kEntries[i].sName), true, true, kOut, sError))
        {
            return false;
        }
    }
    return true;
}

bool ReadText(const TreeSource& kTree, const std::string& sRel, std::string& sText, std::string& sError)
{
    std::string sBytes;
    if (!kTree.Read(sRel, sBytes))
    {
        sError = "cannot read " + sRel;
        return false;
    }
    sText = DecodeText(sBytes);
    return true;
}

bool ReadFiles(const TreeSource& kTree, const std::vector<std::string>& kRels, std::vector<TextFile>& kOut,
               std::string& sError)
{
    for (size_t i = 0; i < kRels.size(); ++i)
    {
        TextFile kFile;
        kFile.sRel = kRels[i];
        if (!ReadText(kTree, kRels[i], kFile.sText, sError))
        {
            return false;
        }
        kOut.push_back(kFile);
    }
    return true;
}

bool SourceTree(const SourceInput& kIn, const char* pcDir, bool bMustExist, std::vector<TextFile>& kOut,
                std::string& sError)
{
    std::vector<std::string> kAll;
    if (!WalkFiles(*kIn.pTree, pcDir, true, bMustExist, kAll, sError))
    {
        return false;
    }
    std::vector<std::string> kKeep;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        bool bIn = false;
        if (!InSourceScope(kIn.sRoot, kAll[i], bIn, sError))
        {
            return false;
        }
        if (bIn)
        {
            kKeep.push_back(kAll[i]);
        }
    }
    return ReadFiles(*kIn.pTree, kKeep, kOut, sError);
}

void SortByFullPath(const std::string& sRoot, std::vector<std::string>& kRels)
{
    std::stable_sort(kRels.begin(), kRels.end(), [&sRoot](const std::string& sA, const std::string& sB) {
        return Tables::CultureCompare(FullPath(sRoot, sA), FullPath(sRoot, sB), true) < 0;
    });
}

bool AllowList(const SourceInput& kIn, const char* pcSection, std::vector<std::string>& kOut, std::string& sError)
{
    const std::vector<std::string>* pList = FindAllow(kIn.kAllow, pcSection);
    if (!pList)
    {
        sError = std::string("tests\\hardening\\allow.txt has no section [") + pcSection + "]";
        return false;
    }
    kOut = *pList;
    return true;
}

std::vector<const XmlNode*> StageVendorChildren(const XmlNode& kRoot, const char* pcLocal)
{
    std::vector<const XmlNode*> kAll;
    Elements(kRoot, kAll);
    std::vector<const XmlNode*> kOut;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (!IsMsb(*kAll[i], "Target") || Attr(*kAll[i], "Name") != "StageVendor")
        {
            continue;
        }
        for (size_t j = 0; j < kAll[i]->kChildren.size(); ++j)
        {
            const XmlNode& kChild = kAll[i]->kChildren[j];
            if (kChild.bElement && (!pcLocal || IsMsb(kChild, pcLocal)))
            {
                kOut.push_back(&kChild);
            }
        }
    }
    return kOut;
}

bool IsHex64(const std::string& sText)
{
    static const RegexPattern kPattern("^[0-9a-f]{64}$", false);
    std::string sError;
    return RegexIsMatch(kPattern, sText, sError);
}

std::string PinValue(const std::map<std::string, std::string, CultureLess>& kPins, const char* pcName)
{
    const std::map<std::string, std::string, CultureLess>::const_iterator it = kPins.find(pcName);
    return it == kPins.end() ? std::string() : it->second;
}

bool RunRegexRow(const RegexRowDef& kRow, const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    std::vector<TextFile> kFiles;
    if (!SourceTree(kIn, "src", true, kFiles, sError))
    {
        return false;
    }
    if (kRow.bTests && !SourceTree(kIn, "tests", false, kFiles, sError))
    {
        return false;
    }
    std::vector<std::string> kAllow;
    if (!kRow.sAllow.empty() && !AllowList(kIn, kRow.sAllow.c_str(), kAllow, sError))
    {
        return false;
    }
    return ScanRegexRow(kRow.sName, kIn.sRoot, kFiles, kAllow, kHits, sError);
}

bool RunCharSpacer(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    std::vector<TextFile> kFiles;
    std::vector<std::string> kAllow;
    return SourceTree(kIn, "src", true, kFiles, sError) && AllowList(kIn, "SpacerAllow", kAllow, sError) &&
           ScanCharSpacer(kFiles, kAllow, kHits, sError);
}

bool RunOffsetHack(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    std::vector<TextFile> kFiles;
    std::vector<std::string> kAllow;
    return SourceTree(kIn, "src", true, kFiles, sError) && AllowList(kIn, "OffsetAllow", kAllow, sError) &&
           ScanOffsetHack(kFiles, kAllow, kHits, sError);
}

bool RunAllowVerdict(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    std::vector<std::string> kSpacer;
    std::vector<std::string> kOffset;
    if (!AllowList(kIn, "SpacerAllow", kSpacer, sError) || !AllowList(kIn, "OffsetAllow", kOffset, sError))
    {
        return false;
    }
    kHits = AllowVerdicts(kSpacer, kOffset);
    return true;
}

bool RunSerialValue(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    static const RegexPattern kSerial(R"rx(\b(?:sThreadActivationKey|mCollisionSerial|sCollisionSerialNext)\b)rx", false);
    std::vector<std::string> kAllow;
    if (!AllowList(kIn, "SerialAllow", kAllow, sError))
    {
        return false;
    }
    OrdinalCounts kLeft;
    for (size_t i = 0; i < kAllow.size(); ++i)
    {
        if (!kAllow[i].empty())
        {
            ++kLeft[kAllow[i]];
        }
    }
    std::vector<std::string> kAll;
    if (!WalkFiles(*kIn.pTree, "src", true, true, kAll, sError))
    {
        return false;
    }
    std::vector<std::string> kRels;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (SourceExtension(LeafName(kAll[i])))
        {
            kRels.push_back(kAll[i]);
        }
    }
    SortByFullPath(kIn.sRoot, kRels);
    for (size_t f = 0; f < kRels.size(); ++f)
    {
        std::string sText;
        if (!ReadText(*kIn.pTree, kRels[f], sText, sError))
        {
            return false;
        }
        bool bAny = false;
        if (!IsMatch(kSerial, sText, bAny, sError))
        {
            return false;
        }
        if (!bAny)
        {
            continue;
        }
        size_t uFrom = 0;
        for (size_t uLine = 1;; ++uLine)
        {
            const size_t uNl = sText.find('\n', uFrom);
            const std::string sLine = sText.substr(uFrom, uNl == std::string::npos ? std::string::npos : uNl - uFrom);
            bool bHit = false;
            if (!IsMatch(kSerial, sLine, bHit, sError))
            {
                return false;
            }
            if (bHit && !TakeAllow(kLeft, kRels[f] + "|" + CollapseSpace(TrimSpace(sLine))))
            {
                kHits.push_back(kRels[f] + ":" + std::to_string(uLine));
            }
            if (uNl == std::string::npos)
            {
                break;
            }
            uFrom = uNl + 1;
        }
    }
    AddUnused(kLeft, kHits);
    return true;
}

bool RunSizeLiteral(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    static const RegexPattern kVerdict(R"rx(^(.*)\|x64:safe:(.+)$)rx", false);
    std::vector<std::string> kAllow;
    if (!AllowList(kIn, "SizeLiteralAllow", kAllow, sError))
    {
        return false;
    }
    OrdinalCounts kLeft;
    for (size_t i = 0; i < kAllow.size(); ++i)
    {
        if (kAllow[i].empty())
        {
            continue;
        }
        RegexMatcher kMatcher(kVerdict, kAllow[i]);
        if (!kMatcher.FindNext() || TrimSpace(kMatcher.GetCaptureGroup(2)).empty())
        {
            kHits.push_back("allow without an x64:safe reason " + kAllow[i]);
            continue;
        }
        ++kLeft[kMatcher.GetCaptureGroup(1)];
    }
    std::vector<std::string> kAll;
    if (!WalkFiles(*kIn.pTree, "src", true, false, kAll, sError))
    {
        return false;
    }
    std::vector<std::string> kRels;
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        bool bIn = false;
        if (!InSourceScope(kIn.sRoot, kAll[i], bIn, sError))
        {
            return false;
        }
        if (bIn)
        {
            kRels.push_back(kAll[i]);
        }
    }
    SortByFullPath(kIn.sRoot, kRels);
    const int iName = SizeFourLocalPattern().GroupNumber("name");
    for (size_t f = 0; f < kRels.size(); ++f)
    {
        std::string sText;
        std::string sBlank;
        if (!ReadText(*kIn.pTree, kRels[f], sText, sError) || !BlankComments(sText, sBlank, sError))
        {
            return false;
        }
        std::vector<std::string> kLocals;
        RegexMatcher kLocal(SizeFourLocalPattern(), sBlank);
        while (kLocal.FindNext())
        {
            const std::string sName = kLocal.GetCaptureGroup(static_cast<size_t>(iName));
            if (std::find(kLocals.begin(), kLocals.end(), sName) == kLocals.end())
            {
                kLocals.push_back(sName);
            }
        }
        if (kLocal.Failed())
        {
            sError = kRels[f] + ": " + kLocal.Error();
            return false;
        }
        std::vector<RegexPattern> kFileRx = SizeLiteralBase();
        if (!kLocals.empty())
        {
            std::string sLocalArg = "\\b(?:";
            for (size_t i = 0; i < kLocals.size(); ++i)
            {
                sLocalArg += (i ? "|" : "") + RegexEscape(kLocals[i]);
            }
            sLocalArg += ")\\b";
            const std::vector<RegexPattern> kLocalRx = SizeLiteralPatterns(sLocalArg, sLocalArg);
            kFileRx.insert(kFileRx.end(), kLocalRx.begin(), kLocalRx.end());
        }
        for (size_t r = 0; r < kFileRx.size(); ++r)
        {
            std::vector<Span> kSpans;
            if (!AllMatches(kFileRx[r], sBlank, kSpans, sError))
            {
                sError = kRels[f] + ": " + sError;
                return false;
            }
            for (size_t s = 0; s < kSpans.size(); ++s)
            {
                if (!TakeAllow(kLeft, kRels[f] + "|" + CollapseSpace(kSpans[s].sText)))
                {
                    kHits.push_back(kRels[f] + ":" + std::to_string(LineAt(sBlank, kSpans[s].uStart)));
                }
            }
        }
    }
    AddUnused(kLeft, kHits);
    return true;
}

bool RunRedlogVendor(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    const char* const apcDirs[] = {"src\\LuaFL", "src\\ZlibFL", "src\\mimalloc", "src\\FFmpegFL"};
    for (size_t d = 0; d < sizeof(apcDirs) / sizeof(apcDirs[0]); ++d)
    {
        std::vector<std::string> kRels;
        if (!WalkFiles(*kIn.pTree, apcDirs[d], true, false, kRels, sError))
        {
            return false;
        }
        for (size_t i = 0; i < kRels.size(); ++i)
        {
            std::string sText;
            if (!ReadText(*kIn.pTree, kRels[i], sText, sError))
            {
                return false;
            }
            if (ContainsNoCase(sText, "RedLog.h"))
            {
                kHits.push_back(FullPath(kIn.sRoot, kRels[i]));
            }
        }
    }
    return true;
}

bool RunUnicowsTree(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    std::string sProject;
    XmlNode kRoot;
    if (!ReadText(*kIn.pTree, "BattlefrontII.vcxproj", sProject, sError) ||
        !ParseXml(sProject, "BattlefrontII.vcxproj", kRoot, sError))
    {
        return false;
    }
    std::vector<const XmlNode*> kAll;
    Elements(kRoot, kAll);
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        const XmlNode& e = *kAll[i];
        if ((IsMsb(e, "GameDataFile") || IsMsb(e, "VendorFile")) && e.pParent && IsMsb(*e.pParent, "ItemGroup") &&
            ContainsNoCase(Attr(e, "Include"), "unicows"))
        {
            kHits.push_back("vcxproj " + e.sLocal + " " + Attr(e, "Include"));
        }
    }
    for (size_t i = 0; i < kAll.size(); ++i)
    {
        if (IsMsb(*kAll[i], "Copy") && ContainsNoCase(Attr(*kAll[i], "SourceFiles"), "unicows"))
        {
            kHits.push_back("vcxproj Copy " + Attr(*kAll[i], "SourceFiles"));
        }
    }
    const std::vector<std::string>* const apWorkers[] = {&kIn.kWin32Workers, &kIn.kX64Workers};
    const char* const apcPlatforms[] = {"Win32", "x64"};
    for (size_t p = 0; p < 2; ++p)
    {
        for (size_t i = 0; i < apWorkers[p]->size(); ++i)
        {
            if (ContainsNoCase((*apWorkers[p])[i], "unicows"))
            {
                kHits.push_back(std::string(apcPlatforms[p]) + " worker list");
                break;
            }
        }
    }
    std::vector<std::string> kRels;
    if (!WalkFiles(*kIn.pTree, "src", true, false, kRels, sError))
    {
        return false;
    }
    for (size_t i = 0; i < kRels.size(); ++i)
    {
        std::string sText;
        if (!ReadText(*kIn.pTree, kRels[i], sText, sError))
        {
            return false;
        }
        if (ContainsNoCase(LeafName(kRels[i]), "unicows") || ContainsNoCase(sText, "unicows"))
        {
            kHits.push_back("named in " + FullPath(kIn.sRoot, kRels[i]));
        }
    }
    return true;
}

bool ProjectFileName(const std::string& sName)
{
    static const RegexPattern kPattern(R"rx(\.(vcxproj|vcxproj\.filters|props)$)rx", true);
    std::string sError;
    return RegexIsMatch(kPattern, sName, sError);
}

bool RunBinkTree(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    static const RegexPattern kBinkAny(R"rx((?i)binkw(32|64))rx", false);
    static const RegexPattern kBinkCall(R"rx(\bBink[A-Z]\w*\s*\(|\bBINK(IO|FILE)?\b)rx", false);
    static const RegexPattern kBinkName(R"rx((?i)\bbinkw(32|64)\b)rx", false);
    std::vector<std::string> kNameAllow;
    if (!AllowList(kIn, "BinkNameAllow", kNameAllow, sError))
    {
        return false;
    }
    std::vector<std::string> kLib;
    if (!WalkFiles(*kIn.pTree, "lib", true, false, kLib, sError))
    {
        return false;
    }
    for (size_t i = 0; i < kLib.size(); ++i)
    {
        if (StartsWithNoCase(LeafName(kLib[i]), "binkw"))
        {
            kHits.push_back("lib file " + kLib[i]);
        }
    }
    std::vector<std::string> kProjDirs = {"", "lib", "tests"};
    std::vector<RepoEntry> kSrcEntries;
    if (kIn.pTree->List("src", kSrcEntries))
    {
        for (size_t i = 0; i < kSrcEntries.size(); ++i)
        {
            if (kSrcEntries[i].bDir)
            {
                kProjDirs.push_back("src\\" + kSrcEntries[i].sName);
            }
        }
    }
    for (size_t d = 0; d < kProjDirs.size(); ++d)
    {
        std::vector<std::string> kFiles;
        if (!WalkFiles(*kIn.pTree, kProjDirs[d], false, false, kFiles, sError))
        {
            return false;
        }
        for (size_t f = 0; f < kFiles.size(); ++f)
        {
            if (!ProjectFileName(LeafName(kFiles[f])))
            {
                continue;
            }
            std::string sText;
            XmlNode kRoot;
            if (!ReadText(*kIn.pTree, kFiles[f], sText, sError) || !ParseXml(sText, kFiles[f], kRoot, sError))
            {
                return false;
            }
            std::vector<const XmlNode*> kAll;
            Elements(kRoot, kAll);
            for (size_t e = 0; e < kAll.size(); ++e)
            {
                if (EqualNoCase(kAll[e]->sLocal, "Delete"))
                {
                    continue;
                }
                bool bNamed = false;
                for (size_t a = 0; a < kAll[e]->kAttrs.size() && !bNamed; ++a)
                {
                    bNamed = ContainsNoCase(kAll[e]->kAttrs[a].second, "binkw32");
                }
                for (size_t c = 0; c < kAll[e]->kChildren.size() && !bNamed; ++c)
                {
                    const XmlNode& kChild = kAll[e]->kChildren[c];
                    bNamed = !kChild.bElement && !kChild.bCData && ContainsNoCase(kChild.sText, "binkw32");
                }
                if (bNamed)
                {
                    kHits.push_back(kFiles[f] + " " + kAll[e]->sLocal);
                }
            }
        }
    }
    bool bHasDelete = false;
    std::string sMain;
    if (ReadText(*kIn.pTree, "BattlefrontII.vcxproj", sMain, sError))
    {
        XmlNode kRoot;
        if (!ParseXml(sMain, "BattlefrontII.vcxproj", kRoot, sError))
        {
            return false;
        }
        const std::vector<const XmlNode*> kDeletes = StageVendorChildren(kRoot, "Delete");
        size_t uCount = 0;
        for (size_t i = 0; i < kDeletes.size(); ++i)
        {
            uCount += Attr(*kDeletes[i], "Files") == "$(OutDir)binkw32.dll" ? 1 : 0;
        }
        bHasDelete = uCount == 1;
    }
    sError.clear();
    if (!bHasDelete)
    {
        kHits.push_back("StageVendor does not delete a stale binkw32.dll");
    }
    std::string sWorkers;
    if (ReadText(*kIn.pTree, "tests\\gate\\gate_machine.cpp", sWorkers, sError))
    {
        bool bNamed = false;
        if (!IsMatch(kBinkAny, sWorkers, bNamed, sError))
        {
            return false;
        }
        if (bNamed)
        {
            kHits.push_back("named in tests\\gate\\gate_machine.cpp");
        }
    }
    sError.clear();
    std::set<std::string> kUsed;
    const char* const apcTestDirs[] = {"harness", "game"};
    for (size_t d = 0; d < 2; ++d)
    {
        std::vector<std::string> kFiles;
        if (!WalkFiles(*kIn.pTree, std::string("tests\\") + apcTestDirs[d], false, false, kFiles, sError))
        {
            return false;
        }
        for (size_t f = 0; f < kFiles.size(); ++f)
        {
            const std::string sRel = std::string("tests\\") + apcTestDirs[d] + "\\" + LeafName(kFiles[f]);
            std::string sBody;
            if (!ReadText(*kIn.pTree, kFiles[f], sBody, sError))
            {
                return false;
            }
            for (size_t a = 0; a < kNameAllow.size(); ++a)
            {
                const std::vector<std::string> kParts = SplitOn(kNameAllow[a], '|');
                if (kParts.size() != 3 || TrimSpace(kParts[2]).empty())
                {
                    continue;
                }
                if (EqualNoCase(kParts[0], sRel) && !kParts[1].empty() && sBody.find(kParts[1]) != std::string::npos)
                {
                    for (size_t uAt = sBody.find(kParts[1]); uAt != std::string::npos; uAt = sBody.find(kParts[1], uAt))
                    {
                        sBody.erase(uAt, kParts[1].size());
                    }
                    kUsed.insert(kNameAllow[a]);
                }
            }
            bool bNamed = false;
            if (!IsMatch(kBinkAny, sBody, bNamed, sError))
            {
                return false;
            }
            if (bNamed)
            {
                kHits.push_back("named in " + sRel);
            }
        }
    }
    for (size_t a = 0; a < kNameAllow.size(); ++a)
    {
        const std::vector<std::string> kParts = SplitOn(kNameAllow[a], '|');
        if (kParts.size() != 3 || TrimSpace(kParts[2]).empty())
        {
            kHits.push_back("allow entry without file|text|reason: " + kNameAllow[a]);
        }
        else if (!kUsed.count(kNameAllow[a]))
        {
            kHits.push_back("unused allow " + kNameAllow[a]);
        }
    }
    const std::vector<std::string>* const apWorkers[] = {&kIn.kWin32Workers, &kIn.kX64Workers};
    bool bWorker = false;
    for (size_t p = 0; p < 2 && !bWorker; ++p)
    {
        for (size_t i = 0; i < apWorkers[p]->size() && !bWorker; ++i)
        {
            bWorker = ContainsNoCase((*apWorkers[p])[i], "binkw");
        }
    }
    if (bWorker)
    {
        kHits.push_back("worker list");
    }
    std::vector<std::string> kRels;
    std::vector<std::string> kSrc;
    if (!WalkFiles(*kIn.pTree, "src", true, false, kSrc, sError))
    {
        return false;
    }
    for (size_t i = 0; i < kSrc.size(); ++i)
    {
        bool bVendor = false;
        if (!IsMatch(VendorPattern(), FullPath(kIn.sRoot, kSrc[i]), bVendor, sError))
        {
            return false;
        }
        if (!bVendor)
        {
            kRels.push_back(kSrc[i]);
        }
    }
    if (!WalkFiles(*kIn.pTree, "tests\\unit", true, false, kRels, sError))
    {
        return false;
    }
    for (size_t f = 0; f < kRels.size(); ++f)
    {
        if (!SourceExtension(LeafName(kRels[f])))
        {
            continue;
        }
        std::string sText;
        if (!ReadText(*kIn.pTree, kRels[f], sText, sError))
        {
            return false;
        }
        const bool bGame = StartsWithNoCase(kRels[f], "src\\");
        const std::vector<std::string> kLines = Tables::ReadAllLines(sText);
        for (size_t i = 0; i < kLines.size(); ++i)
        {
            bool bHit = false;
            if (!IsMatch(kBinkCall, kLines[i], bHit, sError))
            {
                return false;
            }
            if (!bHit && bGame && !IsMatch(kBinkName, kLines[i], bHit, sError))
            {
                return false;
            }
            if (bHit)
            {
                kHits.push_back("source " + kRels[f] + ":" + std::to_string(i + 1));
            }
        }
    }
    return true;
}

bool ReadPinsAndProject(const SourceInput& kIn, std::map<std::string, std::string, CultureLess>& kPins, XmlNode& kRoot,
                        std::string& sError)
{
    std::string sPins;
    std::string sProject;
    return ReadText(*kIn.pTree, "lib\\vendor-pins.props", sPins, sError) && VendorPins(sPins, kPins, sError) &&
           ReadText(*kIn.pTree, "BattlefrontII.vcxproj", sProject, sError) &&
           ParseXml(sProject, "BattlefrontII.vcxproj", kRoot, sError);
}

bool RunD3dxPins(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    std::map<std::string, std::string, CultureLess> kPins;
    XmlNode kRoot;
    if (!ReadPinsAndProject(kIn, kPins, kRoot, sError))
    {
        return false;
    }
    const char* const apcPins[] = {"D3dxNupkgSha256", "D3dxWin32Sha256", "D3dxX64Sha256"};
    for (size_t i = 0; i < 3; ++i)
    {
        if (!IsHex64(PinValue(kPins, apcPins[i])))
        {
            kHits.push_back(std::string("vendor-pins.props ") + apcPins[i] + " is not 64 hex digits");
        }
    }
    bool bImport = false;
    if (IsMsb(kRoot, "Project"))
    {
        for (size_t i = 0; i < kRoot.kChildren.size() && !bImport; ++i)
        {
            const XmlNode& kChild = kRoot.kChildren[i];
            bImport = IsMsb(kChild, "Import") && EqualNoCase(Attr(kChild, "Project"), "lib\\vendor-pins.props") &&
                      Attr(kChild, "Condition").empty();
        }
    }
    if (!bImport)
    {
        kHits.push_back("vcxproj does not import lib\\vendor-pins.props");
    }
    const std::vector<const XmlNode*> kVerifies = StageVendorChildren(kRoot, "VerifyFileHash");
    const char* const apcWant[][3] = {{"$(D3dxNupkg)", "$(D3dxNupkgSha256)", ""},
                                      {"$(D3dxDll)", "$(D3dxWin32Sha256)", "'$(Platform)'=='Win32'"},
                                      {"$(D3dxDll)", "$(D3dxX64Sha256)", "'$(Platform)'=='x64'"}};
    for (size_t w = 0; w < 3; ++w)
    {
        bool bHit = false;
        for (size_t i = 0; i < kVerifies.size() && !bHit; ++i)
        {
            const XmlNode& v = *kVerifies[i];
            bHit = EqualNoCase(Attr(v, "File"), apcWant[w][0]) && EqualNoCase(Attr(v, "Hash"), apcWant[w][1]) &&
                   EqualNoCase(Attr(v, "Algorithm"), "SHA256") && EqualNoCase(Attr(v, "Condition"), apcWant[w][2]);
        }
        if (!bHit)
        {
            kHits.push_back(std::string("StageVendor has no VerifyFileHash of ") + apcWant[w][0] + " against " +
                            apcWant[w][1]);
        }
    }
    const std::vector<const XmlNode*> kUnzips = StageVendorChildren(kRoot, "Unzip");
    bool bBadUnzip = kUnzips.empty();
    for (size_t i = 0; i < kUnzips.size(); ++i)
    {
        if (!EqualNoCase(Attr(*kUnzips[i], "SourceFiles"), "$(D3dxNupkg)") ||
            !EqualNoCase(Attr(*kUnzips[i], "Condition"), "'$(D3dxDllHash)' != '$(D3dxDllSha256)'"))
        {
            bBadUnzip = true;
        }
    }
    if (bBadUnzip)
    {
        kHits.push_back("StageVendor Unzip of $(D3dxNupkg) is not conditioned on the cached DLL hash");
    }
    return true;
}

bool RunSteamApi64Pins(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError)
{
    const std::string sSource = "$(SWBF2_VENDOR_X64)\\steam_api64.dll";
    std::map<std::string, std::string, CultureLess> kPins;
    XmlNode kRoot;
    if (!ReadPinsAndProject(kIn, kPins, kRoot, sError))
    {
        return false;
    }
    if (!IsHex64(PinValue(kPins, "SteamApi64Sha256")))
    {
        kHits.push_back("vendor-pins.props SteamApi64Sha256 is not 64 hex digits");
    }
    const std::vector<const XmlNode*> kSteps = StageVendorChildren(kRoot, 0);
    size_t uCopies = 0;
    for (size_t i = 0; i < kSteps.size(); ++i)
    {
        const XmlNode& s = *kSteps[i];
        if (!EqualNoCase(s.sLocal, "Copy") || !EqualNoCase(Attr(s, "SourceFiles"), sSource))
        {
            continue;
        }
        ++uCopies;
        bool bVerified = false;
        for (size_t j = 0; j < i; ++j)
        {
            const XmlNode& v = *kSteps[j];
            if (EqualNoCase(v.sLocal, "VerifyFileHash") && EqualNoCase(Attr(v, "File"), sSource) &&
                EqualNoCase(Attr(v, "Hash"), "$(SteamApi64Sha256)") && EqualNoCase(Attr(v, "Algorithm"), "SHA256") &&
                EqualNoCase(Attr(v, "Condition"), Attr(s, "Condition")))
            {
                bVerified = true;
            }
        }
        if (!bVerified)
        {
            kHits.push_back("StageVendor copies " + sSource +
                            " without a VerifyFileHash against $(SteamApi64Sha256) before it");
        }
    }
    if (uCopies == 0)
    {
        kHits.push_back("StageVendor has no Copy of " + sSource);
    }
    return true;
}

typedef bool (*RowFn)(const SourceInput& kIn, std::vector<std::string>& kHits, std::string& sError);

struct SourceRowDef
{
    const char* pcName;
    RowFn pfnRun;
};

const SourceRowDef kSourceRows[] = {
    {"redlog-vendor", RunRedlogVendor},
    {"new-size-t", 0},
    {"win32-callbacks", 0},
    {"galaxy-vtable", 0},
    {"fastcall-edx", 0},
    {"d3dx-decl", 0},
    {"gamma-get", 0},
    {"addr-order", 0},
    {"mi-api-ban", 0},
    {"crt-heap-ban", 0},
    {"snd-alloc-size-t", 0},
    {"galaxy-getter-cast", 0},
    {"galaxy-factory-seam", 0},
    {"steamid-abi", 0},
    {"handle-raw", 0},
    {"char-spacer", RunCharSpacer},
    {"offset-hack", RunOffsetHack},
    {"allow-x64-verdict", RunAllowVerdict},
    {"serial-value", RunSerialValue},
    {"x64-size-literal", RunSizeLiteral},
    {"unicows-staged", RunUnicowsTree},
    {"bink-gone", RunBinkTree},
    {"d3dx9-pins", RunD3dxPins},
    {"steam-api64-pins", RunSteamApi64Pins},
};

}

bool RepoTree::List(const std::string& sRelDir, std::vector<RepoEntry>& kOut) const
{
    return ListRepoFolder(sRelDir.empty() ? "." : sRelDir, "*", kOut);
}

bool RepoTree::Read(const std::string& sRel, std::string& sBytes) const
{
    return ReadRepoBytes(sRel, sBytes);
}

void MemoryTree::Add(const std::string& sRel, const std::string& sBytes)
{
    mkFiles[sRel] = sBytes;
}

bool MemoryTree::List(const std::string& sRelDir, std::vector<RepoEntry>& kOut) const
{
    kOut.clear();
    const std::string sPrefix = sRelDir.empty() ? std::string() : sRelDir + "\\";
    std::map<std::string, bool, CultureLess> kNames;
    bool bFound = sRelDir.empty();
    for (std::map<std::string, std::string>::const_iterator it = mkFiles.begin(); it != mkFiles.end(); ++it)
    {
        if (!StartsWithNoCase(it->first, sPrefix))
        {
            continue;
        }
        bFound = true;
        const std::string sRest = it->first.substr(sPrefix.size());
        const size_t uSlash = sRest.find('\\');
        kNames[uSlash == std::string::npos ? sRest : sRest.substr(0, uSlash)] = uSlash != std::string::npos;
    }
    for (std::map<std::string, bool, CultureLess>::const_iterator it = kNames.begin(); it != kNames.end(); ++it)
    {
        RepoEntry kEntry;
        kEntry.sName = it->first;
        kEntry.bDir = it->second;
        kOut.push_back(kEntry);
    }
    return bFound;
}

bool MemoryTree::Read(const std::string& sRel, std::string& sBytes) const
{
    for (std::map<std::string, std::string>::const_iterator it = mkFiles.begin(); it != mkFiles.end(); ++it)
    {
        if (EqualNoCase(it->first, sRel))
        {
            sBytes = it->second;
            return true;
        }
    }
    return false;
}

std::string DecodeText(const std::string& sBytes)
{
    if (sBytes.size() >= 3 && static_cast<unsigned char>(sBytes[0]) == 0xEF &&
        static_cast<unsigned char>(sBytes[1]) == 0xBB && static_cast<unsigned char>(sBytes[2]) == 0xBF)
    {
        return sBytes.substr(3);
    }
    if (sBytes.size() >= 2)
    {
        const unsigned char uA = static_cast<unsigned char>(sBytes[0]);
        const unsigned char uB = static_cast<unsigned char>(sBytes[1]);
        const bool bLittle = uA == 0xFF && uB == 0xFE;
        const bool bBig = uA == 0xFE && uB == 0xFF;
        if (bLittle || bBig)
        {
            std::wstring sWide;
            for (size_t i = 2; i + 1 < sBytes.size(); i += 2)
            {
                const unsigned int uLo = static_cast<unsigned char>(sBytes[bLittle ? i : i + 1]);
                const unsigned int uHi = static_cast<unsigned char>(sBytes[bLittle ? i + 1 : i]);
                sWide += static_cast<wchar_t>(uLo | (uHi << 8));
            }
            return Narrow(sWide);
        }
    }
    return sBytes;
}

size_t LineAt(const std::string& sText, size_t uAt)
{
    return 1 + static_cast<size_t>(std::count(sText.begin(), sText.begin() + (std::min)(uAt, sText.size()), '\n'));
}

std::string CollapseSpace(const std::string& sText)
{
    std::string sOut;
    for (size_t i = 0; i < sText.size(); ++i)
    {
        if (!IsSpace(sText[i]))
        {
            sOut += sText[i];
            continue;
        }
        sOut += ' ';
        while (i + 1 < sText.size() && IsSpace(sText[i + 1]))
        {
            ++i;
        }
    }
    return sOut;
}

std::string TrimSpace(const std::string& sText)
{
    size_t uBegin = 0;
    size_t uEnd = sText.size();
    while (uBegin < uEnd && IsSpace(sText[uBegin]))
    {
        ++uBegin;
    }
    while (uEnd > uBegin && IsSpace(sText[uEnd - 1]))
    {
        --uEnd;
    }
    return sText.substr(uBegin, uEnd - uBegin);
}

bool StartsWithNoCase(const std::string& sText, const std::string& sPrefix)
{
    return sText.size() >= sPrefix.size() && _strnicmp(sText.c_str(), sPrefix.c_str(), sPrefix.size()) == 0;
}

bool ContainsNoCase(const std::string& sText, const std::string& sNeedle)
{
    if (sNeedle.empty())
    {
        return true;
    }
    for (size_t i = 0; i + sNeedle.size() <= sText.size(); ++i)
    {
        size_t j = 0;
        while (j < sNeedle.size() && LowerAscii(sText[i + j]) == LowerAscii(sNeedle[j]))
        {
            ++j;
        }
        if (j == sNeedle.size())
        {
            return true;
        }
    }
    return false;
}

bool InSourceScope(const std::string& sRoot, const std::string& sRel, bool& bIn, std::string& sError)
{
    bIn = false;
    if (!SourceExtension(LeafName(sRel)))
    {
        return true;
    }
    const RegexPattern& kExclude = StartsWithNoCase(sRel, "tests\\") ? TestsThirdPartyPattern() : VendorPattern();
    bool bExcluded = false;
    if (!IsMatch(kExclude, FullPath(sRoot, sRel), bExcluded, sError))
    {
        return false;
    }
    bIn = !bExcluded;
    return true;
}

bool BlankComments(const std::string& sText, std::string& sOut, std::string& sError)
{
    sOut = sText;
    std::vector<Span> kSpans;
    if (!AllMatches(ByteCommentPattern(), sText, kSpans, sError))
    {
        return false;
    }
    for (size_t i = 0; i < kSpans.size(); ++i)
    {
        if (kSpans[i].uLength == 0 || sText[kSpans[i].uStart] != '/')
        {
            continue;
        }
        for (size_t j = kSpans[i].uStart; j < kSpans[i].uStart + kSpans[i].uLength; ++j)
        {
            if (sOut[j] != '\r' && sOut[j] != '\n')
            {
                sOut[j] = ' ';
            }
        }
    }
    return true;
}

bool ByteArrayDecls(const std::string& sText, std::vector<ByteDecl>& kOut, std::string& sError)
{
    std::string sBlank;
    if (!BlankComments(sText, sBlank, sError))
    {
        return false;
    }
    const int iList = ByteDeclPattern().GroupNumber("list");
    if (iList < 0)
    {
        sError = "ByteDeclRx has no list group";
        return false;
    }
    RegexMatcher kDecl(ByteDeclPattern(), sBlank);
    while (kDecl.FindNext())
    {
        const std::string sList = kDecl.GetCaptureGroup(static_cast<size_t>(iList));
        const size_t uListAt = kDecl.GetCaptureGroupBeginning(static_cast<size_t>(iList));
        size_t uPos = 0;
        while (uPos < sList.size())
        {
            RegexMatcher kItem(ByteDeclItemPattern(), sList);
            const bool bMatched = kItem.MatchAt(uPos);
            if (kItem.Failed())
            {
                sError = kItem.Error();
                return false;
            }
            if (!bMatched || kItem.GetMatchLength() == 0)
            {
                break;
            }
            const std::string sDims = kItem.GetCaptureGroup(3);
            if (!sDims.empty() && kItem.GetCaptureGroup(1).empty())
            {
                ByteDecl kOne;
                kOne.sName = kItem.GetCaptureGroup(2);
                for (size_t i = 0; i < sDims.size(); ++i)
                {
                    if (!IsSpace(sDims[i]))
                    {
                        kOne.sSize += sDims[i];
                    }
                }
                kOne.uLine = LineAt(sBlank, uListAt + kItem.GetCaptureGroupBeginning(2));
                kOut.push_back(kOne);
            }
            uPos = kItem.GetMatchBeginning() + kItem.GetMatchLength();
        }
    }
    if (kDecl.Failed())
    {
        sError = kDecl.Error();
        return false;
    }
    return true;
}

bool IsByteSpacer(const ByteDecl& kDecl, std::string& sError)
{
    static const RegexPattern kHex("0[xX][0-9A-Fa-f]", true);
    static const RegexPattern kSizeof(R"rx(\b(?:sizeof|offsetof)\b)rx", true);
    static const RegexPattern kName(R"rx((?i)pad|reserv|unmodel|unknown|^_*unk|spacer|filler|tail|unused|dummy|storage|align|^_+\w*\d)rx",
                                    true);
    return RegexIsMatch(kHex, kDecl.sSize, sError) || RegexIsMatch(kSizeof, kDecl.sSize, sError) ||
           RegexIsMatch(kName, kDecl.sName, sError);
}

bool VendorPins(const std::string& sPinsText, std::map<std::string, std::string, CultureLess>& kOut, std::string& sError)
{
    kOut.clear();
    XmlNode kRoot;
    if (!ParseXml(sPinsText, "lib\\vendor-pins.props", kRoot, sError))
    {
        return false;
    }
    for (size_t g = 0; g < kRoot.kChildren.size(); ++g)
    {
        const XmlNode& kGroup = kRoot.kChildren[g];
        if (!kGroup.bElement || !EqualNoCase(kGroup.sLocal, "PropertyGroup"))
        {
            continue;
        }
        for (size_t p = 0; p < kGroup.kChildren.size(); ++p)
        {
            if (kGroup.kChildren[p].bElement)
            {
                kOut[kGroup.kChildren[p].sLocal] = TrimSpace(InnerText(kGroup.kChildren[p]));
            }
        }
    }
    return true;
}

bool ScanRegexRow(const std::string& sRow, const std::string& sRoot, const std::vector<TextFile>& kFiles,
                  const std::vector<std::string>& kAllow, std::vector<std::string>& kHits, std::string& sError)
{
    const RegexRowDef* pRow = FindRegexRow(sRow);
    if (!pRow)
    {
        sError = "no regex row " + sRow;
        return false;
    }
    OrdinalCounts kLeft;
    const bool bSigned = !pRow->sAllow.empty();
    for (size_t i = 0; i < kAllow.size(); ++i)
    {
        if (!kAllow[i].empty())
        {
            ++kLeft[kAllow[i]];
        }
    }
    for (size_t f = 0; f < kFiles.size(); ++f)
    {
        const TextFile& kFile = kFiles[f];
        const bool bTest = StartsWithNoCase(kFile.sRel, "tests\\");
        if (bTest && !pRow->bTests)
        {
            continue;
        }
        const std::string sFull = FullPath(sRoot, kFile.sRel);
        bool bIn = true;
        if (pRow->pPath && !IsMatch(*pRow->pPath, sFull, bIn, sError))
        {
            return false;
        }
        if (!bIn)
        {
            continue;
        }
        std::vector<Span> kSpans;
        for (size_t r = 0; r < pRow->kRules.size(); ++r)
        {
            const RegexRuleDef& kRule = pRow->kRules[r];
            bool bRuleIn = true;
            if (kRule.pPath && !IsMatch(*kRule.pPath, sFull, bRuleIn, sError))
            {
                return false;
            }
            if (bRuleIn && !AllMatches(*kRule.pPattern, kFile.sText, kSpans, sError))
            {
                sError = kFile.sRel + ": " + sError;
                return false;
            }
        }
        std::stable_sort(kSpans.begin(), kSpans.end(),
                         [](const Span& kA, const Span& kB) { return kA.uStart < kB.uStart; });
        bool bHaveEnd = false;
        size_t uEnd = 0;
        std::set<size_t> kSeen;
        std::vector<std::string> kLines;
        for (size_t s = 0; s < kSpans.size(); ++s)
        {
            if (bHaveEnd && kSpans[s].uStart < uEnd)
            {
                uEnd = (std::max)(uEnd, kSpans[s].uEnd);
                continue;
            }
            bHaveEnd = true;
            uEnd = kSpans[s].uEnd;
            const size_t uLine = LineAt(kFile.sText, kSpans[s].uStart);
            if (!kSeen.insert(uLine).second)
            {
                continue;
            }
            if (bSigned)
            {
                if (kLines.empty())
                {
                    kLines = SplitOn(kFile.sText, '\n');
                }
                if (TakeAllow(kLeft, kFile.sRel + "|" + CollapseSpace(TrimSpace(kLines[uLine - 1]))))
                {
                    continue;
                }
            }
            const std::string sKey = kFile.sRel + ":" + std::to_string(uLine);
            kHits.push_back(pRow->sNote.empty() ? sKey : sKey + " " + pRow->sNote);
        }
    }
    if (bSigned)
    {
        AddUnused(kLeft, kHits);
    }
    return true;
}

bool ScanCharSpacer(const std::vector<TextFile>& kFiles, const std::vector<std::string>& kAllow,
                    std::vector<std::string>& kHits, std::string& sError)
{
    CultureCounts kLeft;
    for (size_t i = 0; i < kAllow.size(); ++i)
    {
        if (kAllow[i].empty())
        {
            continue;
        }
        std::string sKey;
        VerdictKey(kAllow[i], sKey);
        ++kLeft[sKey];
    }
    for (size_t f = 0; f < kFiles.size(); ++f)
    {
        if (StartsWithNoCase(kFiles[f].sRel, "tests\\"))
        {
            continue;
        }
        std::vector<ByteDecl> kDecls;
        if (!ByteArrayDecls(kFiles[f].sText, kDecls, sError))
        {
            sError = kFiles[f].sRel + ": " + sError;
            return false;
        }
        for (size_t d = 0; d < kDecls.size(); ++d)
        {
            const bool bSpacer = IsByteSpacer(kDecls[d], sError);
            if (!sError.empty())
            {
                return false;
            }
            if (!bSpacer || TakeAllow(kLeft, kFiles[f].sRel + "|" + kDecls[d].sName + "|" + kDecls[d].sSize))
            {
                continue;
            }
            kHits.push_back(kFiles[f].sRel + ":" + std::to_string(kDecls[d].uLine));
        }
    }
    AddUnused(kLeft, kHits);
    return true;
}

bool ScanOffsetHack(const std::vector<TextFile>& kFiles, const std::vector<std::string>& kAllow,
                    std::vector<std::string>& kHits, std::string& sError)
{
    OrdinalCounts kLeft;
    for (size_t i = 0; i < kAllow.size(); ++i)
    {
        if (kAllow[i].empty())
        {
            continue;
        }
        std::string sKey;
        VerdictKey(kAllow[i], sKey);
        ++kLeft[sKey];
    }
    for (size_t f = 0; f < kFiles.size(); ++f)
    {
        if (StartsWithNoCase(kFiles[f].sRel, "tests\\"))
        {
            continue;
        }
        std::string sBlank;
        if (!BlankComments(kFiles[f].sText, sBlank, sError))
        {
            sError = kFiles[f].sRel + ": " + sError;
            return false;
        }
        std::vector<Span> kSpans;
        const std::vector<RegexPattern>& kPatterns = OffsetPatterns();
        for (size_t p = 0; p < kPatterns.size(); ++p)
        {
            if (!AllMatches(kPatterns[p], sBlank, kSpans, sError))
            {
                sError = kFiles[f].sRel + ": " + sError;
                return false;
            }
        }
        std::stable_sort(kSpans.begin(), kSpans.end(), [](const Span& kA, const Span& kB) {
            return kA.uStart != kB.uStart ? kA.uStart < kB.uStart : kA.uLength > kB.uLength;
        });
        bool bHaveEnd = false;
        size_t uEnd = 0;
        for (size_t s = 0; s < kSpans.size(); ++s)
        {
            if (bHaveEnd && kSpans[s].uStart < uEnd)
            {
                continue;
            }
            bHaveEnd = true;
            uEnd = kSpans[s].uStart + kSpans[s].uLength;
            if (TakeAllow(kLeft, kFiles[f].sRel + "|" + CollapseSpace(kSpans[s].sText)))
            {
                continue;
            }
            kHits.push_back(kFiles[f].sRel + ":" + std::to_string(LineAt(sBlank, kSpans[s].uStart)));
        }
    }
    AddUnused(kLeft, kHits);
    return true;
}

std::vector<std::string> AllowVerdicts(const std::vector<std::string>& kSpacerAllow,
                                       const std::vector<std::string>& kOffsetAllow)
{
    std::vector<std::string> kHits;
    const std::vector<std::string>* const apLists[] = {&kSpacerAllow, &kOffsetAllow};
    for (size_t l = 0; l < 2; ++l)
    {
        for (size_t i = 0; i < apLists[l]->size(); ++i)
        {
            const std::string& sEntry = (*apLists[l])[i];
            std::string sKey;
            if (!sEntry.empty() && !VerdictKey(sEntry, sKey))
            {
                kHits.push_back("no verdict " + sEntry);
            }
        }
    }
    return kHits;
}

std::vector<std::string> SourceRowNames()
{
    std::vector<std::string> kNames;
    for (size_t i = 0; i < sizeof(kSourceRows) / sizeof(kSourceRows[0]); ++i)
    {
        kNames.push_back(kSourceRows[i].pcName);
    }
    return kNames;
}

bool ScanSourceRow(const std::string& sRow, const SourceInput& kIn, std::vector<std::string>& kHits,
                   std::string& sError)
{
    for (size_t i = 0; i < sizeof(kSourceRows) / sizeof(kSourceRows[0]); ++i)
    {
        if (sRow != kSourceRows[i].pcName)
        {
            continue;
        }
        if (kSourceRows[i].pfnRun)
        {
            return kSourceRows[i].pfnRun(kIn, kHits, sError);
        }
        const RegexRowDef* pRow = FindRegexRow(sRow);
        if (!pRow)
        {
            sError = "no regex row " + sRow;
            return false;
        }
        return RunRegexRow(*pRow, kIn, kHits, sError);
    }
    sError = "no hardening source row " + sRow;
    return false;
}

bool RunSourceRow(const std::string& sRow, std::vector<std::string>& kHits, std::string& sError)
{
    RepoTree kTree;
    SourceInput kIn;
    kIn.sRoot = RepoRoot();
    kIn.pTree = &kTree;
    kIn.kWin32Workers = Gate::SmokeWorkerFiles("Win32");
    kIn.kX64Workers = Gate::SmokeWorkerFiles("x64");
    if (kIn.sRoot.empty())
    {
        sError = "cannot resolve the repo root";
        return false;
    }
    if (!ReadAllowFile(kIn.kAllow, sError))
    {
        return false;
    }
    return ScanSourceRow(sRow, kIn, kHits, sError);
}

}
