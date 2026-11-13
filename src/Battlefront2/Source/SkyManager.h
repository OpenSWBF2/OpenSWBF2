#pragma once

class PblFileChunk;

namespace SkyManager
{
    void Init();
    void Reset();
    void Read(PblFileChunk& chunk);
    void Activate();
}
