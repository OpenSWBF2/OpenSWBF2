#include "third_party/doctest/doctest.h"

#include "Battlefront2/Source/GameVoiceChatSlots.h"
#include <stdio.h>
#include <string.h>
#include <string>

#include "testutil.h"

namespace
{

bool ReadSource(const char* pcPath, std::string& sOut)
{
    FILE* pFile = fopen(pcPath, "rb");
    if (!pFile)
    {
        return false;
    }
    char acBuf[4096];
    size_t uiRead;
    while ((uiRead = fread(acBuf, 1, sizeof(acBuf), pFile)) > 0)
    {
        sOut.append(acBuf, uiRead);
    }
    fclose(pFile);
    return true;
}

}

TEST_CASE("voice chat mixer clear touches one connection" * doctest::test_suite("regression"))
{
    const unsigned int uiConnBytes = sizeof(Snd::StreamNodeConnection);
    unsigned int auBuffer[(4 * 2 * sizeof(Snd::StreamNodeConnection) + 16) / sizeof(unsigned int)];
    for (unsigned int uiSlots = 1; uiSlots <= 4; ++uiSlots)
    {
        const unsigned int uiTotal = uiSlots * 2 * uiConnBytes + 16;
        memset(auBuffer, 0xCD, sizeof(auBuffer));
        Snd::StreamNodeConnection* pConnections = (Snd::StreamNodeConnection*)auBuffer;
        for (unsigned int i = 0; i < uiSlots; ++i)
        {
            GameVoiceChat::ClearMixerConnection(pConnections, i);
        }
        const unsigned char* pucBytes = (const unsigned char*)auBuffer;
        bool bConnZero = true;
        for (unsigned int i = 0; i < uiSlots * uiConnBytes; ++i)
        {
            if (pucBytes[i] != 0)
            {
                bConnZero = false;
            }
        }
        bool bTailIntact = true;
        for (unsigned int i = uiSlots * uiConnBytes; i < uiTotal; ++i)
        {
            if (pucBytes[i] != 0xCD)
            {
                bTailIntact = false;
            }
        }
        CAPTURE(uiSlots);
        CHECK(bConnZero);
        CHECK(bTailIntact);
    }
}

TEST_CASE("voice chat decoder clears through the helper" * doctest::test_suite("regression"))
{
    std::string sDecoder;
    if (!ReadSource("../src/Battlefront2/Source/GameVoiceChatDecoder.cpp", sDecoder))
    {
        SkipCase("source not found");
    }
    CHECK(sDecoder.find("GameVoiceChat::ClearMixerConnection(mMixerConnections, i);") != std::string::npos);
    CHECK(sDecoder.find("memset(&mMixerConnections") == std::string::npos);
}
