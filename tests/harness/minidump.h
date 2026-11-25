#pragma once
#include <string>

struct MinidumpException
{
    bool bHasException = false;
    unsigned int uCode = 0;
    unsigned long long ullAddress = 0;
    bool bHasModule = false;
    std::string sModule;
    unsigned long long ullBase = 0;
    unsigned int uSize = 0;
};

bool ReadMinidumpException(const std::string& sBytes, MinidumpException& kOut, std::string& sWhy);
bool DumpMatchesFatal(const std::string& sBytes, const std::string& sFatalLine, std::string& sWhy);
