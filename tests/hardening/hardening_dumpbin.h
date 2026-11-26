#pragma once
#include <functional>
#include <string>
#include <vector>

namespace Hardening
{

struct DumpbinRun
{
    bool bStarted;
    unsigned long uCode;
    std::string sText;
    std::string sErr;
};

typedef std::function<void(const char* pcData, size_t uSize)> DumpbinChunkFn;

bool Dumpbin(const std::string& sPlatform, const std::vector<std::string>& kArgs, DumpbinRun& kRun,
             std::string& sError, const DumpbinChunkFn& fnChunk = DumpbinChunkFn());

}
