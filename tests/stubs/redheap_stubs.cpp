#include <stdio.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"
#include "RedEngineFL/Util/RedLog.h"
#include "testutil.h"

namespace GameMemory { int LuaHeap = -1; }

static int siTestHeap = -1;

void __cdecl LuaScript_CallWARNINGF(char const* pcMessage)
{
    LOG(LogLua, Warning, "Lua error: %.240s", pcMessage ? pcMessage : "(null)");
}

static void TestHeapFailure(int iHeap, unsigned int uSize)
{
    if (iHeap != siTestHeap)
    {
        return;
    }
    char acWhy[128];
    _snprintf(acWhy, sizeof(acWhy) - 1, "heap T1 exhausted: a request of %u bytes failed", uSize);
    acWhy[sizeof(acWhy) - 1] = 0;
    HarnessError(acWhy);
}

extern "C" void Startup_RedInitHeap()
{
    RedMiConfigure(1000);
    char* pBase = (char*)0x20;
    char* pEnd = pBase + 0x40000000;
    char* pLuaStart = pEnd - 0x1000000;
    char* pStart = (char*)RedInitHeap(pBase, pEnd, 9);
    siTestHeap = RedCreateHeap(pStart, pLuaStart, 0x10, "T1");
    GameMemory::LuaHeap = RedCreateHeap(pLuaStart, pEnd, 4, "Lua");
    OnFailure = TestHeapFailure;
    RedSetCurrentHeap(0);
}
