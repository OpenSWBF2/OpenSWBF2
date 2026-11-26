#define PCRE2_CODE_UNIT_WIDTH 8
#define PCRE2_STATIC
#include "third_party/pcre2/src/pcre2.h"

#include "hardening/hardening_regex.h"

namespace Hardening
{

namespace
{

std::string PcreMessage(int iCode)
{
    PCRE2_UCHAR acBuf[256];
    if (pcre2_get_error_message(iCode, acBuf, sizeof(acBuf)) < 0)
    {
        return "PCRE2 error " + std::to_string(iCode);
    }
    return std::string(reinterpret_cast<const char*>(acBuf));
}

pcre2_code* Code(const std::shared_ptr<void>& pCode)
{
    return static_cast<pcre2_code*>(pCode.get());
}

}

RegexPattern::RegexPattern(const std::string& sPattern, bool bIgnoreCase, BeforeFn pfnBefore)
    : msSource(sPattern), mpfnBefore(pfnBefore)
{
    int iError = 0;
    PCRE2_SIZE uOffset = 0;
    pcre2_code* pCode = pcre2_compile(reinterpret_cast<PCRE2_SPTR>(sPattern.c_str()), sPattern.size(),
                                      bIgnoreCase ? PCRE2_CASELESS : 0, &iError, &uOffset, 0);
    if (!pCode)
    {
        msError = "pattern " + sPattern + " at offset " + std::to_string(uOffset) + ": " + PcreMessage(iError);
        return;
    }
    mpCode = std::shared_ptr<void>(pCode, [](void* p) { pcre2_code_free(static_cast<pcre2_code*>(p)); });
}

bool RegexPattern::Valid() const
{
    return mpCode != nullptr;
}

const std::string& RegexPattern::Error() const
{
    return msError;
}

const std::string& RegexPattern::Source() const
{
    return msSource;
}

int RegexPattern::GroupNumber(const char* pcName) const
{
    if (!mpCode)
    {
        return -1;
    }
    return pcre2_substring_number_from_name(Code(mpCode), reinterpret_cast<PCRE2_SPTR>(pcName));
}

RegexMatcher::RegexMatcher(const RegexPattern& kPattern, const std::string& sText)
    : mkPattern(kPattern), msText(sText), muNext(0), mbFailed(!kPattern.Valid()), msError(kPattern.Error())
{
    if (!mbFailed)
    {
        pcre2_match_data* pData = pcre2_match_data_create_from_pattern(Code(kPattern.mpCode), 0);
        mpData = std::shared_ptr<void>(pData, [](void* p) { pcre2_match_data_free(static_cast<pcre2_match_data*>(p)); });
        if (!pData)
        {
            mbFailed = true;
            msError = "pattern " + kPattern.msSource + ": no match data";
        }
    }
}

bool RegexMatcher::Search(size_t uFrom, bool bAnchored)
{
    mkBegin.clear();
    mkLength.clear();
    mkMatched.clear();
    if (mbFailed)
    {
        return false;
    }
    pcre2_match_data* pData = static_cast<pcre2_match_data*>(mpData.get());
    size_t uPos = uFrom;
    while (uPos <= msText.size())
    {
        const int iResult = pcre2_match(Code(mkPattern.mpCode), reinterpret_cast<PCRE2_SPTR>(msText.data()), msText.size(),
                                        uPos, bAnchored ? PCRE2_ANCHORED : 0, pData, 0);
        if (iResult == PCRE2_ERROR_NOMATCH)
        {
            return false;
        }
        if (iResult <= 0)
        {
            mbFailed = true;
            msError = "pattern " + mkPattern.msSource + " at byte " + std::to_string(uPos) + ": " +
                      (iResult == 0 ? std::string("ovector too small") : PcreMessage(iResult));
            return false;
        }
        const PCRE2_SIZE* pVector = pcre2_get_ovector_pointer(pData);
        const size_t uAt = static_cast<size_t>(pVector[0]);
        if (mkPattern.mpfnBefore && !mkPattern.mpfnBefore(msText, uAt))
        {
            if (bAnchored)
            {
                return false;
            }
            uPos = uAt + 1;
            continue;
        }
        const uint32_t uPairs = pcre2_get_ovector_count(pData);
        for (uint32_t i = 0; i < uPairs; ++i)
        {
            const bool bSet = i < static_cast<uint32_t>(iResult) && pVector[2 * i] != PCRE2_UNSET;
            mkMatched.push_back(bSet);
            mkBegin.push_back(bSet ? static_cast<size_t>(pVector[2 * i]) : 0);
            mkLength.push_back(bSet ? static_cast<size_t>(pVector[2 * i + 1] - pVector[2 * i]) : 0);
        }
        return true;
    }
    return false;
}

bool RegexMatcher::FindNext()
{
    if (muNext > msText.size() || !Search(muNext, false))
    {
        muNext = msText.size() + 1;
        return false;
    }
    muNext = mkLength[0] == 0 ? mkBegin[0] + 1 : mkBegin[0] + mkLength[0];
    return true;
}

bool RegexMatcher::MatchAt(size_t uPos)
{
    return uPos <= msText.size() && Search(uPos, true);
}

size_t RegexMatcher::GetMatchBeginning() const
{
    return mkBegin.empty() ? 0 : mkBegin[0];
}

size_t RegexMatcher::GetMatchLength() const
{
    return mkLength.empty() ? 0 : mkLength[0];
}

bool RegexMatcher::GroupMatched(size_t uIndex) const
{
    return uIndex < mkMatched.size() && mkMatched[uIndex];
}

size_t RegexMatcher::GetCaptureGroupBeginning(size_t uIndex) const
{
    return GroupMatched(uIndex) ? mkBegin[uIndex] : 0;
}

std::string RegexMatcher::GetCaptureGroup(size_t uIndex) const
{
    return GroupMatched(uIndex) ? msText.substr(mkBegin[uIndex], mkLength[uIndex]) : std::string();
}

bool RegexMatcher::Failed() const
{
    return mbFailed;
}

const std::string& RegexMatcher::Error() const
{
    return msError;
}

bool RegexIsMatch(const RegexPattern& kPattern, const std::string& sText, std::string& sError)
{
    RegexMatcher kMatcher(kPattern, sText);
    const bool bFound = kMatcher.FindNext();
    if (kMatcher.Failed())
    {
        sError = kMatcher.Error();
    }
    return bFound;
}

}
