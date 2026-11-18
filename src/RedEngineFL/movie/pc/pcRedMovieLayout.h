#pragma once

#include <stddef.h>

class RedTexture;
class RedBitmapElement;

namespace MovieLayout
{

inline size_t TextureSlotCount(unsigned int uiNumTextures)
{
    return 4 * (size_t)uiNumTextures;
}

inline size_t TextureArrayOffset(unsigned int uiBufferSize)
{
    return uiBufferSize;
}

inline size_t BitmapArrayOffset(unsigned int uiBufferSize, unsigned int uiNumTextures)
{
    return TextureArrayOffset(uiBufferSize) + sizeof(RedTexture*) * TextureSlotCount(uiNumTextures);
}

inline size_t BlockSize(unsigned int uiBufferSize, unsigned int uiNumTextures, unsigned int uiNumBitmaps)
{
    return BitmapArrayOffset(uiBufferSize, uiNumTextures) + sizeof(RedBitmapElement*) * uiNumBitmaps;
}

}
