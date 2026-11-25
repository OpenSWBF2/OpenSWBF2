#include "third_party/doctest/doctest.h"

#include <limits>

#include "RedEngineFL/movie/RedMovieGain.h"
#include "SoundFL/source/sndbus.h"

TEST_CASE("movie gain maps the bink volume to the stream amplitude" * doctest::test_suite("unit"))
{
    CHECK(MovieStreamGain(0.58f) == 0.5799560546875f);
    CHECK(MovieStreamGain(0.5f) == 0.499969482421875f);
    CHECK(MovieStreamGain(1.0f) == 0.999969482421875f);
    CHECK(MovieStreamGain(2.0f) == 0.999969482421875f);
    CHECK(MovieStreamGain(std::numeric_limits<float>::infinity()) == 0.999969482421875f);
    CHECK(MovieStreamGain(0.0f) == 0.0f);
    CHECK(MovieStreamGain(0.000001f) == 0.0f);
    CHECK(MovieStreamGain(-0.25f) == 0.0f);
    CHECK(MovieStreamGain(-std::numeric_limits<float>::infinity()) == 0.0f);
    CHECK(MovieStreamGain(std::numeric_limits<float>::quiet_NaN()) == 0.0f);
}

TEST_CASE("movie bus gain follows a rebuilt bus tree and never reads a destroyed bus" * doctest::test_suite("unit"))
{
    const unsigned int uiMovies = 0xF4EB2B40u;
    Snd::Bus* pFirst = Snd::Bus::smRootBus.AddSubBus(uiMovies);
    REQUIRE(pFirst);
    pFirst->SetGain(0.5f, true);
    float fGain = -1.0f;
    CHECK(MovieBusGain(uiMovies, &fGain));
    CHECK(fGain == 0.5f);

    Snd::Bus::smRootBus.DestroySubBuses();
    fGain = -1.0f;
    CHECK_FALSE(MovieBusGain(uiMovies, &fGain));
    CHECK(fGain == -1.0f);

    Snd::Bus* pOther = Snd::Bus::smRootBus.AddSubBus(0x12345678u);
    REQUIRE(pOther);
    pOther->SetGain(0.25f, true);
    Snd::Bus* pSecond = Snd::Bus::smRootBus.AddSubBus(uiMovies);
    REQUIRE(pSecond);
    pSecond->SetGain(0.75f, true);
    CHECK(MovieBusGain(uiMovies, &fGain));
    CHECK(fGain == 0.75f);
    CHECK_FALSE(MovieBusGain(0, &fGain));
    CHECK(MovieBusGain(Snd::Bus::smRootBus.mHashID, &fGain));
    CHECK(fGain == 1.0f);

    Snd::Bus::smRootBus.DestroySubBuses();
}
