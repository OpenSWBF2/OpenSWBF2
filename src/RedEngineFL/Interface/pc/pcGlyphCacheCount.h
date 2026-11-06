#pragma once

template <class T>
inline unsigned int GlyphCacheCount(unsigned int uiFontPages, T* const* ppShader)
{
    unsigned int i = 0;
    while (ppShader && i < uiFontPages && ppShader[i])
    {
        ++i;
    }
    return i;
}
