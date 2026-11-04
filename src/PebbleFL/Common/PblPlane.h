#pragma once

#include "PblVector.h"

class PblPlane : private PblVector4
{
public:
    // FUNCTION: SWBF2 0x006afc90
    PblPlane() {}

    PblPlane(float a, float b, float c, float d) : PblVector4(a, b, c, d) {}

    float GetA() const { return x; }
    float GetB() const { return y; }
    float GetC() const { return z; }
    float GetD() const { return w; }
};
