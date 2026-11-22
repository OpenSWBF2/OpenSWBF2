#pragma once

#include <stddef.h>

class MemoryPool;

struct MemPoolInfo
{
    unsigned short mCount;
    unsigned short mBin;
    unsigned int mTotal;
};
 
 
 

namespace LuaMemory
{

void CreatePools(bool bMission);

MemoryPool* LocatePool(unsigned int uiSize, void* pBlock);

void Reset();

#ifdef OPENSWBF2_TEST
bool FormatLuaSpillLine(int iBin, unsigned int uCount, char* pcOut, unsigned int uSize);

unsigned int GetSpillCount(unsigned int uiBin);

void LogMemReportLines();

const unsigned int kLuaHistMaxBin = 640;
const unsigned int kLuaHistRows = kLuaHistMaxBin + 2;

unsigned int LuaHistRow(unsigned int uiSize);

bool FormatLuaHistLine(unsigned int uRow, unsigned int uPeak, unsigned int uCalls, char* pcOut, unsigned int uSize);

unsigned int GetHistPeak(unsigned int uRow);

unsigned int GetHistCalls(unsigned int uRow);

void NoteHistAlloc(unsigned int uiNewSize, bool bOldFreed, unsigned int uiOldSize);

void NoteHistFree(unsigned int uiSize);

void LogLuaHistLines();

void ResetTestCounters();
#else
inline void NoteHistAlloc(unsigned int, bool, unsigned int)
{
}

inline void NoteHistFree(unsigned int)
{
}
#endif

}

void Lua_Red_Free(void* pBlock, unsigned int uiSize);
void* Lua_Red_realloc(void* pBlock, unsigned int uiOldSize, unsigned int uiNewSize);
struct lua_State;
int Lua_Red_Panic(lua_State* L);

void __cdecl LuaScript_CallWARNINGF(char const* pcMessage);
