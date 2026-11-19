#include "third_party/doctest/doctest.h"

#include <string.h>

#include "RedEngineFL/movie/pc/pcRedMoviePlayer.h"
#include "testutil.h"

namespace
{
struct NullMovieStorage
{
    alignas(16) unsigned char acPlayer[sizeof(RedMoviePlayer)];
    unsigned char acBefore[sizeof(RedMoviePlayer)];

    NullMovieStorage()
    {
        memset(acPlayer, 0xA5, sizeof(acPlayer));
        memcpy(acBefore, acPlayer, sizeof(acBefore));
    }

    RedMoviePlayer& Player()
    {
        return *reinterpret_cast<RedMoviePlayer*>(acPlayer);
    }

    bool Untouched() const
    {
        return memcmp(acPlayer, acBefore, sizeof(acPlayer)) == 0;
    }
};
}

TEST_CASE("null movie player fails every open and warns once" * doctest::test_suite("unit"))
{
    LogCapture kLog;
    NullMovieStorage kStore;
    CHECK_FALSE(kStore.Player().Open("x.bik", 2, 0, 0, 0, 0x40000, 0x20000, 2));
    CHECK_FALSE(kStore.Player().Open("x.bik", 2, 0, 0, 0, 0x40000, 0x20000, 2));
    CHECK(kStore.Untouched());
    CHECK(RedMoviePlayer::sReadSize == 0);
    CHECK(kLog.Count("OPENSWBF2 MOVIE off: no movie decoder in this build") == 1);
    CHECK(kLog.Count("OPENSWBF2 MOVIE") == 1);
}

TEST_CASE("null movie player leaves a never-opened player alone" * doctest::test_suite("unit"))
{
    LogCapture kLog;
    NullMovieStorage kStore;
    RedMoviePlayer& kPlayer = kStore.Player();
    kPlayer.Update();
    kPlayer.Render(0, 0, 0, 640, 480, 0x80);
    kPlayer.SeekSegment(0, true);
    kPlayer.SetGain(1.0f);
    kPlayer.EndFrame();
    kPlayer.Flush();
    kPlayer.Close();
    CHECK(kPlayer.GetPlaybackTime() == 0.0f);
    CHECK(kStore.Untouched());
    CHECK(kLog.Count("OPENSWBF2 MOVIE") == 0);
}
