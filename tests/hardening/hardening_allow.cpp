#include "hardening/hardening_allow.h"

#include "testutil.h"

namespace Hardening
{

const char* const kAllowSectionNames[] = {
    "ImportStaticAllow Win32",
    "ImportStaticAllow x64",
    "ImportDelayAllow Win32",
    "ImportDelayAllow x64",
    "MiDefines Debug",
    "MiDefines Release",
    "MiDefines Test",
    "MiDefines Asan",
    "SpacerAllow",
    "OffsetAllow",
    "SizeLiteralAllow",
    "SerialAllow",
    "AddrOrderAllow",
    "FastcallAllow",
    "BinkNameAllow",
};

const size_t kAllowSectionCount = sizeof(kAllowSectionNames) / sizeof(kAllowSectionNames[0]);

static bool KnownSection(const std::string& sName)
{
    for (size_t i = 0; i < kAllowSectionCount; ++i)
    {
        if (sName == kAllowSectionNames[i])
        {
            return true;
        }
    }
    return false;
}

bool ParseAllowText(const std::string& sText, AllowFile& kOut, std::string& sError)
{
    kOut.kSections.clear();
    size_t uPos = 0;
    size_t uLine = 0;
    while (uPos < sText.size())
    {
        ++uLine;
        size_t uEnd = sText.find('\n', uPos);
        const bool bTerminated = uEnd != std::string::npos;
        if (!bTerminated)
        {
            uEnd = sText.size();
        }
        std::string sLine = sText.substr(uPos, uEnd - uPos);
        if (!sLine.empty() && sLine[sLine.size() - 1] == '\r')
        {
            sLine.erase(sLine.size() - 1);
        }
        uPos = bTerminated ? uEnd + 1 : uEnd;
        const std::string sAt = "allow line " + std::to_string(uLine);
        if (sLine.empty())
        {
            sError = sAt + " is empty";
            return false;
        }
        if (sLine[0] == '[' && sLine[sLine.size() - 1] == ']')
        {
            const std::string sName = sLine.substr(1, sLine.size() - 2);
            if (!KnownSection(sName))
            {
                sError = sAt + " opens an unknown section " + sLine;
                return false;
            }
            if (FindAllow(kOut, sName))
            {
                sError = sAt + " opens section " + sLine + " a second time";
                return false;
            }
            AllowSection kSection;
            kSection.sName = sName;
            kOut.kSections.push_back(kSection);
            continue;
        }
        if (kOut.kSections.empty())
        {
            sError = sAt + " is an entry before any section";
            return false;
        }
        kOut.kSections.back().kEntries.push_back(sLine);
    }
    return true;
}

const std::vector<std::string>* FindAllow(const AllowFile& kFile, const std::string& sName)
{
    for (size_t i = 0; i < kFile.kSections.size(); ++i)
    {
        if (kFile.kSections[i].sName == sName)
        {
            return &kFile.kSections[i].kEntries;
        }
    }
    return 0;
}

bool ReadAllowFile(AllowFile& kOut, std::string& sError)
{
    std::string sText;
    if (!ReadRepoBytes("tests\\hardening\\allow.txt", sText))
    {
        sError = "cannot read tests\\hardening\\allow.txt";
        return false;
    }
    return ParseAllowText(sText, kOut, sError);
}

}
