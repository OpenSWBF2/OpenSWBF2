#pragma once

#include "PebbleFL/Common/PblVector.h"

class DropShadowRenderable;

class RedDropShadowManager
{
public:
    static void Init();
    static void Cleanup();

    static void AddShadow(unsigned int uiTexture, const PblVector3& vPos,
                          const PblVector3& vNormal, const PblVector3& vDir,
                          float fStart, float fEnd, float, float fAlpha);

    static void Render(unsigned int uiFlags);

private:
    static DropShadowRenderable* _Instance;
};
