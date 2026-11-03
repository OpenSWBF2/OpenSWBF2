#include "RedEngineFL/Util/ext/IAllocator.h"

void util::IAllocator::AllocateEx(Buffer& rBuf, int&)
{
    Allocate(rBuf);
}

void util::IAllocator::DeallocateEx(Buffer& rBuf, int&)
{
    Deallocate(rBuf);
}

void util::IAllocator::ClearMemHash()
{
}

unsigned int util::IAllocator::GetMemHash()
{
    return 0;
}

void util::IAllocator::SetMaxMem(int)
{
}
