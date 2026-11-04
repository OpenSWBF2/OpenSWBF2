#pragma once

namespace GameMemory
{

const unsigned int kApplicationHeapSize = 0xF40000;
const unsigned int kGamespyHeapSize = 0x200000;
const unsigned int kLuaHeapSize = 0x1000000;
const unsigned int kArtistHeapSize = 0x2000;
const unsigned int kTempLoadBlockSize = 0x200000;
const unsigned int kTempLoadHeapSize = 0x1FFF00;
const unsigned int kHeapAlignment = 0x10;
const unsigned int kLuaHeapAlignment = 4;
const unsigned int kHeapStartAlignment = 0x40;

void InitApplicationMemory(char* pStart, char* pEnd);

void BuildHeaps(bool bMission);

void ReleaseTempHeap();

extern int ApplicationHeap;
extern int GamespyHeap;
extern int RunTimeHeap;
extern int TempLoadHeap;
extern int LuaHeap;
extern int ArtistHeap;
extern int TEMP_GamespyHeap;
}
