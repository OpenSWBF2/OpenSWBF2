#include "third_party/doctest/doctest.h"

#include <windows.h>
#include "RedEngineFL/Graphics/PC/pcGammaPassPS.h"

namespace
{

bool Contains(DWORD dwValue)
{
    const unsigned int uiCount = sizeof(kGammaPassPS) / sizeof(kGammaPassPS[0]);
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        if (kGammaPassPS[i] == dwValue)
        {
            return true;
        }
    }
    return false;
}

unsigned int Fnv1a()
{
    unsigned int uiHash = 2166136261u;
    const unsigned int uiCount = sizeof(kGammaPassPS) / sizeof(kGammaPassPS[0]);
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        uiHash = (uiHash ^ (unsigned int)kGammaPassPS[i]) * 16777619u;
    }
    return uiHash;
}

}

TEST_CASE("gamma pass shader bytecode" * doctest::test_suite("unit"))
{
    const unsigned int uiCount = sizeof(kGammaPassPS) / sizeof(kGammaPassPS[0]);
    CHECK(kGammaPassPS[0] == 0xFFFF0200);
    CHECK(kGammaPassPS[uiCount - 1] == 0x0000FFFF);
    CHECK(uiCount == 116);
    CHECK(Fnv1a() == 0x3BCEEC5Bu);
    CHECK(Contains(0x3F7F0000));
    CHECK(Contains(0x3B000000));
    CHECK(Contains(0x3F000000));
}
