#include "unit/crt_trig.h"

#include <stdio.h>

bool CrtTrigInProcess(CrtTrigReference& kRef, std::string& sWhy)
{
    kRef.vEntries.clear();
    sWhy = "the x86 crt runs only in the Win32 child";
    return false;
}

int CrtSimWritePins(const std::string&)
{
    printf("gen-corpus: simmath pins come from the Win32 build\n");
    return 0;
}

int CrtTrigRefWriteFile(const char*)
{
    printf("crt-trig-ref: the reference comes from the Win32 build\n");
    return 2;
}
