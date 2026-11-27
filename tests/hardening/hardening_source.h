#pragma once
#include <map>
#include <string>
#include <vector>

#include "hardening/hardening_allow.h"
#include "tables/tables_readers.h"
#include "testutil.h"

namespace Hardening
{

struct TextFile
{
    std::string sRel;
    std::string sText;
};

struct ByteDecl
{
    std::string sName;
    std::string sSize;
    size_t uLine;
};

struct CultureLess
{
    bool operator()(const std::string& sA, const std::string& sB) const
    {
        return Tables::CultureCompare(sA, sB, true) < 0;
    }
};

class TreeSource
{
public:
    virtual ~TreeSource()
    {
    }
    virtual bool List(const std::string& sRelDir, std::vector<RepoEntry>& kOut) const = 0;
    virtual bool Read(const std::string& sRel, std::string& sBytes) const = 0;
};

class RepoTree : public TreeSource
{
public:
    bool List(const std::string& sRelDir, std::vector<RepoEntry>& kOut) const override;
    bool Read(const std::string& sRel, std::string& sBytes) const override;
};

class MemoryTree : public TreeSource
{
public:
    void Add(const std::string& sRel, const std::string& sBytes);
    bool List(const std::string& sRelDir, std::vector<RepoEntry>& kOut) const override;
    bool Read(const std::string& sRel, std::string& sBytes) const override;

private:
    std::map<std::string, std::string> mkFiles;
};

struct SourceInput
{
    std::string sRoot;
    const TreeSource* pTree;
    AllowFile kAllow;
    std::vector<std::string> kWin32Workers;
    std::vector<std::string> kX64Workers;
};

std::string DecodeText(const std::string& sBytes);
size_t LineAt(const std::string& sText, size_t uAt);
std::string CollapseSpace(const std::string& sText);
std::string TrimSpace(const std::string& sText);
bool StartsWithNoCase(const std::string& sText, const std::string& sPrefix);
bool ContainsNoCase(const std::string& sText, const std::string& sNeedle);

bool InSourceScope(const std::string& sRoot, const std::string& sRel, bool& bIn, std::string& sError);
bool BlankComments(const std::string& sText, std::string& sOut, std::string& sError);
bool ByteArrayDecls(const std::string& sText, std::vector<ByteDecl>& kOut, std::string& sError);
bool IsByteSpacer(const ByteDecl& kDecl, std::string& sError);
bool VendorPins(const std::string& sPinsText, std::map<std::string, std::string, CultureLess>& kOut,
                std::string& sError);

bool ScanRegexRow(const std::string& sRow, const std::string& sRoot, const std::vector<TextFile>& kFiles,
                  const std::vector<std::string>& kAllow, std::vector<std::string>& kHits, std::string& sError);
bool ScanCharSpacer(const std::vector<TextFile>& kFiles, const std::vector<std::string>& kAllow,
                    std::vector<std::string>& kHits, std::string& sError);
bool ScanOffsetHack(const std::vector<TextFile>& kFiles, const std::vector<std::string>& kAllow,
                    std::vector<std::string>& kHits, std::string& sError);
std::vector<std::string> AllowVerdicts(const std::vector<std::string>& kSpacerAllow,
                                       const std::vector<std::string>& kOffsetAllow);

std::vector<std::string> SourceRowNames();
bool ScanSourceRow(const std::string& sRow, const SourceInput& kIn, std::vector<std::string>& kHits,
                   std::string& sError);
bool RunSourceRow(const std::string& sRow, std::vector<std::string>& kHits, std::string& sError);

}
