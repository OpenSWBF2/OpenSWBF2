#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>

#include "testutil.h"
#include "Battlefront2/Source/HUDItem.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Interface/pc/pcGlyphCacheCount.h"

namespace
{

std::string ReadRepoTextLf(const char* pcRelPath)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRelPath, sText));
    std::string sOut;
    for (size_t i = 0; i < sText.size(); ++i)
    {
        if (sText[i] != '\r')
        {
            sOut += sText[i];
        }
    }
    return sOut;
}

int CountText(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    const std::string sNeedle(pcNeedle);
    for (size_t uiAt = sText.find(sNeedle); uiAt != std::string::npos; uiAt = sText.find(sNeedle, uiAt + 1))
    {
        ++iCount;
    }
    return iCount;
}

struct GlyphShaderProbe
{
    int miId;
};

class HudItemProbe : public HUD::Item
{
public:

    HudItemProbe() : HUD::Item(0, 0, 0) {}

    virtual bool SetProperty(const unsigned int, const void*) { return false; }
    virtual bool GetProperty(const unsigned int, void*) { return false; }
    virtual const char* PropertyTranslateEnum(const unsigned int, const unsigned int) { return 0; }
    virtual bool ReadData(PblConfig&, PblConfig::Data&) { return false; }
    virtual void PostReadSetup() {}
    virtual void WriteData(HUD::File&, unsigned int) {}
};

char sacHudNameArena[0x1000];

struct ScopedHudNameHeap
{
    ScopedHudNameHeap()
        : miPrev(__RedCurrHeap),
          miHeap(RedCreateHeap(sacHudNameArena, sacHudNameArena + sizeof(sacHudNameArena), 0x10, "HudName"))
    {
        if (miHeap >= 0)
        {
            __RedCurrHeap = miHeap;
        }
    }

    ~ScopedHudNameHeap()
    {
        __RedCurrHeap = miPrev;
        if (miHeap >= 0)
        {
            RedDestroyHeap(miHeap);
        }
    }

    int miPrev;
    int miHeap;
};

}

TEST_CASE("fake d3d device starts with one reference (source pin: renderer-bound class)" * doctest::test_suite("regression"))
{
    std::string sHeader = ReadRepoTextLf("src\\RedEngineFL\\Graphics\\PC\\pcRedRenderer.h");
    CHECK(sHeader.find("class FakeD3DDevice : public IDirect3DDevice9\n"
                       "{\n"
                       "public:\n"
                       "\n"
                       "    FakeD3DDevice() : refCount(1) {}\n") != std::string::npos);
}

TEST_CASE("glyph cache walks stop at the allocated count when the font gains pages" * doctest::test_suite("regression"))
{
    GlyphShaderProbe akShader[5] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };
    GlyphShaderProbe* apShader[6] = { &akShader[0], &akShader[1], 0, &akShader[2], &akShader[3], &akShader[4] };
    CHECK(GlyphCacheCount(5u, apShader) == 2u);
    CHECK(GlyphCacheCount(2u, apShader) == 2u);
    CHECK(GlyphCacheCount(1u, apShader) == 1u);
    CHECK(GlyphCacheCount(0u, apShader) == 0u);
    CHECK(GlyphCacheCount(5u, (GlyphShaderProbe* const*)0) == 0u);
}

TEST_CASE("glyph cache element routes every shader walk through the allocated count (source pin)" * doctest::test_suite("regression"))
{
    std::string sCache = ReadRepoTextLf("src\\RedEngineFL\\Interface\\pc\\pcRedGlyphCacheElement.cpp");
    CHECK(CountText(sCache, "GlyphCacheCount(m_pFont, m_ppInterfaceShader)") == 6);
    CHECK(CountText(sCache, "->GetNumTextures()") == 2);
    CHECK(sCache.find("    if (uiPage >= GlyphCacheCount(m_pFont, m_ppInterfaceShader))\n"
                      "    {\n"
                      "        return;\n"
                      "    }\n"
                      "\n"
                      "    Quad& q = s_aQuads[s_uiNumQuads++];\n") != std::string::npos);
    CHECK(sCache.find("    m_ppInterfaceShader = (pcInterfaceShader**)RedAllocFromHeap(\n"
                      "        __RedCurrHeap, (uiNumTextures + 1) * sizeof(pcInterfaceShader*), 0);\n"
                      "    m_ppInterfaceShader[uiNumTextures] = 0;\n") != std::string::npos);
}

TEST_CASE("hud item set name after an aliased name keeps the display name valid" * doctest::test_suite("regression"))
{
    char acLong[300];
    memset(acLong, 'b', sizeof(acLong) - 1);
    acLong[sizeof(acLong) - 1] = 0;

    bool bFirstAliased = false;
    bool bSecondAliased = false;
    bool bSecondNameRead = false;
    bool bDisplayRead = false;
    bool bNameKept = false;
    {
        ScopedHudNameHeap heap;
        REQUIRE(heap.miHeap >= 0);
        HudItemProbe item;
        item.SetName("a");
        bFirstAliased = item.GetDisplayName() == item.GetName();
        void* pNeighbour = RedAllocFromHeap(heap.miHeap, 16, 0);
        item.SetName(acLong);
        RedFreeToHeap(heap.miHeap, pNeighbour, 0);
        bSecondAliased = item.GetDisplayName() == item.GetName();
        if (!bSecondAliased)
        {
            item.mNameDisplay = item.mName;
        }
        bSecondNameRead = strcmp(item.GetName(), acLong) == 0;
        item.SetDisplayName("c");
        bDisplayRead = strcmp(item.GetDisplayName(), "c") == 0;
        bNameKept = strcmp(item.GetName(), acLong) == 0;
    }
    CHECK(bFirstAliased);
    CHECK(bSecondAliased);
    CHECK(bSecondNameRead);
    CHECK(bDisplayRead);
    CHECK(bNameKept);
}

TEST_CASE("snddsbufferrenderer reset gain copies the target gain (source pin)" * doctest::test_suite("regression"))
{
    std::string sSource = ReadRepoTextLf("src\\SoundFL\\source\\win\\snddsbufferrenderer.cpp");
    const size_t uiStart = sSource.find("void DSBufferRenderer::ResetGain()\n{\n");
    REQUIRE(uiStart != std::string::npos);
    const size_t uiEnd = sSource.find("\n}\n", uiStart);
    REQUIRE(uiEnd != std::string::npos);
    const std::string sBody = sSource.substr(uiStart, uiEnd - uiStart);
    CHECK(CountText(sBody, "    for (unsigned int i = 0; i < mNumChannels; ++i)\n"
                           "    {\n"
                           "        mGainCurrent[i] = mGainTarget[i];\n"
                           "    }") == 1);
    CHECK(CountText(sBody, "[i - ") == 0);
    CHECK(CountText(sBody, "mGainCurrent[") == 1);
}

TEST_CASE("voice chat encoder closes its source before freeing its buffers (source pin)" * doctest::test_suite("regression"))
{
    std::string sSource = ReadRepoTextLf("src\\Battlefront2\\Source\\GameVoiceChatEncoder.cpp");
    const size_t uiStart = sSource.find("void GameVoiceChat::Encoder::Close()\n{\n");
    REQUIRE(uiStart != std::string::npos);
    const size_t uiEnd = sSource.find("\n}\n", uiStart);
    REQUIRE(uiEnd != std::string::npos);
    const std::string sBody = sSource.substr(uiStart, uiEnd - uiStart);
    CHECK(CountText(sBody, "mSource.Close();") == 1);
    CHECK(CountText(sBody, "mEncoder.SetPackets(0, 0, 0);") == 1);
    CHECK(CountText(sBody, "FreeBuffers();") == 1);
    const size_t uiSourceClose = sBody.find("mSource.Close();");
    const size_t uiDetach = sBody.find("mEncoder.SetPackets(0, 0, 0);");
    const size_t uiFree = sBody.find("FreeBuffers();");
    REQUIRE(uiSourceClose != std::string::npos);
    REQUIRE(uiDetach != std::string::npos);
    REQUIRE(uiFree != std::string::npos);
    CHECK(uiSourceClose < uiFree);
    CHECK(uiDetach < uiFree);
}

TEST_CASE("dsound wave format cbSize matches the struct passed (source pin)" * doctest::test_suite("regression"))
{
    std::string sSource = ReadRepoTextLf("src\\SoundFL\\source\\win\\snddsbuffer.cpp");
    const size_t uiStart = sSource.find("DSBuffer::WaveFormat::WaveFormat(");
    REQUIRE(uiStart != std::string::npos);
    const size_t uiEnd = sSource.find("\n}\n", uiStart);
    REQUIRE(uiEnd != std::string::npos);
    const std::string sBody = sSource.substr(uiStart, uiEnd - uiStart);
    const size_t uiTag = sBody.find("    Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;\n");
    REQUIRE(uiTag != std::string::npos);
    const std::string sExtensible = sBody.substr(uiTag);
    CHECK(CountText(sExtensible,
                    "    Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);\n") == 1);
    CHECK(CountText(sExtensible, "cbSize") == 1);
    CHECK(CountText(sBody, "cbSize = 40") == 0);
}
