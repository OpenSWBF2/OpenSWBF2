#pragma once
#include <memory>
#include <string>
#include <vector>

namespace Hardening
{

typedef bool (*BeforeFn)(const std::string& sText, size_t uAt);

class RegexPattern
{
public:
    RegexPattern(const std::string& sPattern, bool bIgnoreCase, BeforeFn pfnBefore = 0);
    bool Valid() const;
    const std::string& Error() const;
    const std::string& Source() const;
    int GroupNumber(const char* pcName) const;

private:
    friend class RegexMatcher;
    std::string msSource;
    std::string msError;
    std::shared_ptr<void> mpCode;
    BeforeFn mpfnBefore;
};

class RegexMatcher
{
public:
    RegexMatcher(const RegexPattern& kPattern, const std::string& sText);
    bool FindNext();
    bool MatchAt(size_t uPos);
    size_t GetMatchBeginning() const;
    size_t GetMatchLength() const;
    bool GroupMatched(size_t uIndex) const;
    size_t GetCaptureGroupBeginning(size_t uIndex) const;
    std::string GetCaptureGroup(size_t uIndex) const;
    bool Failed() const;
    const std::string& Error() const;

private:
    bool Search(size_t uFrom, bool bAnchored);

    const RegexPattern& mkPattern;
    const std::string& msText;
    std::shared_ptr<void> mpData;
    size_t muNext;
    bool mbFailed;
    std::string msError;
    std::vector<size_t> mkBegin;
    std::vector<size_t> mkLength;
    std::vector<bool> mkMatched;
};

bool RegexIsMatch(const RegexPattern& kPattern, const std::string& sText, std::string& sError);

}
