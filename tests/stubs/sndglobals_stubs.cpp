#include "SoundFL/source/sndglobals.h"
#include "SoundFL/source/sndvoicebase.h"

#include "RedEngineFL/Memory/RedMemory.h"

namespace
{

void* __cdecl StubSndAlloc(size_t uiSize)
{
    return RedAllocFromHeap(__RedCurrHeap, (unsigned int)uiSize, 0);
}

void __cdecl StubSndFree(void* p)
{
    RedFreeToHeap(__RedCurrHeap, p, 0);
}

}

namespace Snd
{
namespace Globals
{

void* (__cdecl* smAlloc)(size_t) = StubSndAlloc;
void (__cdecl* smFree)(void*) = StubSndFree;

}

void* __cdecl Alloc(unsigned int uiSize)
{
    return Globals::smAlloc(uiSize);
}

void* __cdecl AllocTemp(unsigned int uiSize)
{
    return Globals::smAlloc(uiSize);
}

void __cdecl Free(void* p)
{
    Globals::smFree(p);
}

void __cdecl FreeTemp(void* p)
{
    Globals::smFree(p);
}

VoiceBase::~VoiceBase()
{
}

}
