#include "third_party/doctest/doctest.h"

#include <new>
#include <stdint.h>
#include <string.h>

#include "RedEngineFL/movie/pc/pcRedMovieLayout.h"

TEST_CASE("movie layout holds every pointer array" * doctest::test_suite("regression"))
{
    const unsigned int uiBufferSize = 0x23;
    const unsigned int uiNumTextures = 3;
    const unsigned int uiNumBitmaps = 5;
    const size_t uTextureSlots = MovieLayout::TextureSlotCount(uiNumTextures);
    CHECK(uTextureSlots == 4 * uiNumTextures);
    const size_t uTextureOffset = MovieLayout::TextureArrayOffset(uiBufferSize);
    const size_t uBitmapOffset = MovieLayout::BitmapArrayOffset(uiBufferSize, uiNumTextures);
    const size_t uBlockSize = MovieLayout::BlockSize(uiBufferSize, uiNumTextures, uiNumBitmaps);
    CHECK(uTextureOffset == uiBufferSize);
    CHECK(uBitmapOffset == uTextureOffset + uTextureSlots * sizeof(RedTexture*));
    CHECK(uBlockSize == uBitmapOffset + uiNumBitmaps * sizeof(RedBitmapElement*));
    char* pcBlock = new char[uBlockSize];
    memset(pcBlock, 0, uBlockSize);
    RedTexture** ppTextures = new (pcBlock + uTextureOffset) RedTexture*[uTextureSlots];
    RedBitmapElement** ppBitmaps = new (pcBlock + uBitmapOffset) RedBitmapElement*[uiNumBitmaps];
    for (size_t i = 0; i < uTextureSlots; ++i)
    {
        ppTextures[i] = (RedTexture*)(uintptr_t)(0x1000 + 0x10 * i);
    }
    for (unsigned int i = 0; i < uiNumBitmaps; ++i)
    {
        ppBitmaps[i] = (RedBitmapElement*)(uintptr_t)(0x2000 + 0x10 * i);
    }
    pcBlock[0] = 0x5A;
    pcBlock[uiBufferSize - 1] = 0x6B;
    for (size_t i = 0; i < uTextureSlots; ++i)
    {
        CHECK(ppTextures[i] == (RedTexture*)(uintptr_t)(0x1000 + 0x10 * i));
    }
    for (unsigned int i = 0; i < uiNumBitmaps; ++i)
    {
        CHECK(ppBitmaps[i] == (RedBitmapElement*)(uintptr_t)(0x2000 + 0x10 * i));
    }
    CHECK(pcBlock[0] == 0x5A);
    CHECK(pcBlock[uiBufferSize - 1] == 0x6B);
    delete[] pcBlock;
}
