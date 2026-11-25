#include "harness/minidump.h"

#include <windows.h>
#include <dbghelp.h>
#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{

template <typename T>
bool ReadAt(const std::string& sBytes, unsigned long long ullOffset, T& kOut)
{
    if (ullOffset > sBytes.size() || sBytes.size() - ullOffset < sizeof(T))
    {
        return false;
    }
    memcpy(&kOut, sBytes.data() + ullOffset, sizeof(T));
    return true;
}

std::string Hex(unsigned long long ullValue)
{
    char acText[32];
    _snprintf(acText, sizeof(acText) - 1, "0x%llX", ullValue);
    acText[sizeof(acText) - 1] = 0;
    return acText;
}

bool Truncated(unsigned long long ullOffset, std::string& sWhy)
{
    sWhy = "truncated at offset " + Hex(ullOffset);
    return false;
}

bool ReadModuleName(const std::string& sBytes, RVA uRva, std::string& sName, std::string& sWhy)
{
    ULONG32 uLength = 0;
    if (!ReadAt(sBytes, uRva, uLength))
    {
        return Truncated(uRva, sWhy);
    }
    const unsigned long long ullStart = (unsigned long long)uRva + sizeof(ULONG32);
    if (ullStart > sBytes.size() || sBytes.size() - ullStart < uLength)
    {
        sWhy = "truncated module name at offset " + Hex(uRva);
        return false;
    }
    std::wstring sWide(uLength / sizeof(wchar_t), L'\0');
    if (!sWide.empty())
    {
        memcpy(&sWide[0], sBytes.data() + ullStart, sWide.size() * sizeof(wchar_t));
    }
    const size_t uSlash = sWide.find_last_of(L"\\/");
    if (uSlash != std::wstring::npos)
    {
        sWide.erase(0, uSlash + 1);
    }
    sName.clear();
    if (!sWide.empty())
    {
        const int iLen = WideCharToMultiByte(CP_UTF8, 0, sWide.c_str(), (int)sWide.size(), 0, 0, 0, 0);
        if (iLen > 0)
        {
            sName.resize(iLen);
            WideCharToMultiByte(CP_UTF8, 0, sWide.c_str(), (int)sWide.size(), &sName[0], iLen, 0, 0);
        }
    }
    return true;
}

}

bool ReadMinidumpException(const std::string& sBytes, MinidumpException& kOut, std::string& sWhy)
{
    kOut = MinidumpException();
    MINIDUMP_HEADER kHeader;
    if (!ReadAt(sBytes, 0, kHeader) || kHeader.Signature != MINIDUMP_SIGNATURE)
    {
        sWhy = "no MDMP signature";
        return false;
    }
    bool bException = false;
    bool bModules = false;
    RVA uExceptionRva = 0;
    RVA uModulesRva = 0;
    for (ULONG32 i = 0; i < kHeader.NumberOfStreams; ++i)
    {
        const unsigned long long ullEntry =
            (unsigned long long)kHeader.StreamDirectoryRva + (unsigned long long)i * sizeof(MINIDUMP_DIRECTORY);
        MINIDUMP_DIRECTORY kEntry;
        if (!ReadAt(sBytes, ullEntry, kEntry))
        {
            return Truncated(ullEntry, sWhy);
        }
        if (kEntry.StreamType == ExceptionStream && !bException)
        {
            bException = true;
            uExceptionRva = kEntry.Location.Rva;
        }
        else if (kEntry.StreamType == ModuleListStream && !bModules)
        {
            bModules = true;
            uModulesRva = kEntry.Location.Rva;
        }
    }
    if (!bException)
    {
        return true;
    }
    MINIDUMP_EXCEPTION_STREAM kException;
    if (!ReadAt(sBytes, uExceptionRva, kException))
    {
        return Truncated(uExceptionRva, sWhy);
    }
    kOut.bHasException = true;
    kOut.uCode = kException.ExceptionRecord.ExceptionCode;
    kOut.ullAddress = kException.ExceptionRecord.ExceptionAddress;
    if (!bModules)
    {
        sWhy = "no ModuleListStream";
        return false;
    }
    ULONG32 uModules = 0;
    if (!ReadAt(sBytes, uModulesRva, uModules))
    {
        return Truncated(uModulesRva, sWhy);
    }
    for (ULONG32 i = 0; i < uModules; ++i)
    {
        const unsigned long long ullModule =
            (unsigned long long)uModulesRva + sizeof(ULONG32) + (unsigned long long)i * sizeof(MINIDUMP_MODULE);
        MINIDUMP_MODULE kModule;
        if (!ReadAt(sBytes, ullModule, kModule))
        {
            return Truncated(ullModule, sWhy);
        }
        if (kOut.ullAddress < kModule.BaseOfImage || kOut.ullAddress - kModule.BaseOfImage >= kModule.SizeOfImage)
        {
            continue;
        }
        if (!ReadModuleName(sBytes, kModule.ModuleNameRva, kOut.sModule, sWhy))
        {
            return false;
        }
        kOut.bHasModule = true;
        kOut.ullBase = kModule.BaseOfImage;
        kOut.uSize = kModule.SizeOfImage;
        break;
    }
    return true;
}

bool DumpMatchesFatal(const std::string& sBytes, const std::string& sFatalLine, std::string& sWhy)
{
    MinidumpException kDump;
    std::string sRead;
    if (!ReadMinidumpException(sBytes, kDump, sRead))
    {
        sWhy = "dump: " + sRead;
        return false;
    }
    if (sFatalLine.empty())
    {
        sWhy = "no FATAL line";
        return false;
    }
    static const std::regex kException(
        "FATAL: exception 0x([0-9A-F]{8}) at 0x([0-9A-F]{1,16}) \\(([^()\\r\\n]+)\\+0x([0-9A-F]{1,16})\\)");
    std::smatch kMatch;
    if (std::regex_search(sFatalLine, kMatch, kException))
    {
        if (!kDump.bHasException)
        {
            sWhy = "the dump has no exception stream";
            return false;
        }
        const unsigned long ulCode = strtoul(kMatch[1].str().c_str(), 0, 16);
        const unsigned long long ullAddress = _strtoui64(kMatch[2].str().c_str(), 0, 16);
        const std::string sModule = kMatch[3].str();
        const unsigned long long ullOffset = _strtoui64(kMatch[4].str().c_str(), 0, 16);
        std::string sBad;
        if (kDump.uCode != ulCode)
        {
            sBad += "; code dump " + Hex(kDump.uCode) + " FATAL " + Hex(ulCode);
        }
        if (kDump.ullAddress != ullAddress)
        {
            sBad += "; address dump " + Hex(kDump.ullAddress) + " FATAL " + Hex(ullAddress);
        }
        const std::string sDumpModule = kDump.bHasModule ? kDump.sModule : "none";
        if (_stricmp(sDumpModule.c_str(), sModule.c_str()) != 0)
        {
            sBad += "; module dump " + sDumpModule + " FATAL " + sModule;
        }
        if (!kDump.bHasModule)
        {
            sBad += "; offset dump none FATAL " + Hex(ullOffset);
        }
        else if (kDump.ullAddress - kDump.ullBase != ullOffset)
        {
            sBad += "; offset dump " + Hex(kDump.ullAddress - kDump.ullBase) + " FATAL " + Hex(ullOffset);
        }
        if (!sBad.empty())
        {
            sWhy = sBad.substr(2);
            return false;
        }
        return true;
    }
    if (sFatalLine.compare(0, 20, "FATAL: check failed:") == 0)
    {
        if (!kDump.bHasException || kDump.uCode != 0xE0000004u)
        {
            sWhy = "a check failure dump needs exception 0xE0000004, the dump has " +
                   (kDump.bHasException ? Hex(kDump.uCode) : std::string("none"));
            return false;
        }
        return true;
    }
    if (kDump.bHasException)
    {
        sWhy = "the dump holds exception " + Hex(kDump.uCode) + ", the FATAL line names none";
        return false;
    }
    return true;
}
