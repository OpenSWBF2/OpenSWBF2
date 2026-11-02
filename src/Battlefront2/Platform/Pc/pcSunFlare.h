#pragma once

#include "RedEngineFL/Graphics/RedColor.h"

class PblVector3;

struct CompressedPos
{
    short x;
    short y;
    short z;
    short w;

    void SetPos(float fX, float fY, float fZ);

    static void __cdecl SetBoundingBox(const class PblBox* pBox);

    static PblVector3 s_vMin;
    static PblVector3 s_vScale;
    static PblVector3 s_vScaleInv;
};

 

struct FlareVertex
{
    struct Pos
    {
        float x;
        float y;
        float z;
    } pos;
    RedColor color;
    struct UV
    {
        float u;
        float v;
    } uv;
};

struct FlareCompressedVertex
{
    CompressedPos pos;
    RedColor color;
    struct UV
    {
        short u;
        short v;
    } uv;
};
