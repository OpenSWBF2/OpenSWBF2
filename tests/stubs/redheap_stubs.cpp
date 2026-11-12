#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryMi.h"
#include "RedEngineFL/Util/RedLog.h"

namespace GameMemory { int LuaHeap = -1; }

void __cdecl LuaScript_CallWARNINGF(char const* pcMessage)
{
    LOG(LogLua, Warning, "OPENSWBF2 LUA error: %.240s", pcMessage ? pcMessage : "(null)");
}

extern "C" void Startup_RedInitHeap()
{
    RedMiConfigure(1000);
    char* pBase = (char*)0x20;
    char* pEnd = pBase + 0x4000000;
    char* pLuaStart = pEnd - 0x1000000;
    char* pStart = (char*)RedInitHeap(pBase, pEnd, 9);
    RedCreateHeap(pStart, pLuaStart, 0x10, "T1");
    GameMemory::LuaHeap = RedCreateHeap(pLuaStart, pEnd, 4, "Lua");
    RedSetCurrentHeap(0);
}
