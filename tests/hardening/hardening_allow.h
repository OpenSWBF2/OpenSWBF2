#pragma once
#include <string>
#include <vector>

namespace Hardening
{

struct AllowSection
{
    std::string sName;
    std::vector<std::string> kEntries;
};

struct AllowFile
{
    std::vector<AllowSection> kSections;
};

extern const char* const kAllowSectionNames[];
extern const size_t kAllowSectionCount;

bool ParseAllowText(const std::string& sText, AllowFile& kOut, std::string& sError);
const std::vector<std::string>* FindAllow(const AllowFile& kFile, const std::string& sName);
bool ReadAllowFile(AllowFile& kOut, std::string& sError);

}
