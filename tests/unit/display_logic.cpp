#include "third_party/doctest/doctest.h"

#include <d3d9.h>

#include "testutil.h"
#include "RedEngineFL/Graphics/PC/pcDisplayLogic.h"

using namespace DisplayLogic;

namespace
{

DisplayModeRec M(unsigned int w, unsigned int h, unsigned int r, unsigned int a = 0)
{
    DisplayModeRec k = {w, h, r, a};
    return k;
}

}

TEST_CASE("display logic mode list keeps one adapter" * doctest::test_suite("unit"))
{
    DisplayModeRec src[] = {M(1920, 1080, 60, 0), M(1280, 720, 60, 1), M(1920, 1080, 60, 1),
                            M(1920, 1080, 144, 1), M(1920, 1080, 144, 1)};
    DisplayModeRec out[8];
    CHECK(BuildModeList(src, 5, 1, out, 8) == 3);
    CHECK(out[0].w == 1280);
    CHECK(out[1].refresh == 60);
    CHECK(out[2].refresh == 144);
    CHECK(BuildModeList(src, 5, 0, out, 8) == 1);
}

TEST_CASE("display logic mode list collapses duplicates and has no cap" * doctest::test_suite("unit"))
{
    DisplayModeRec src[300];
    for (unsigned int i = 0; i < 200; i++)
    {
        src[i] = M(640 + i, 480 + i, 60);
    }
    DisplayModeRec out[300];
    CHECK(BuildModeList(src, 200, 0, out, 300) == 200);
    CHECK(BuildModeList(src, 200, 0, 0, 0) == 200);
    DisplayModeRec dup[] = {M(800, 600, 60), M(800, 600, 60), M(800, 600, 75)};
    CHECK(BuildModeList(dup, 3, 0, out, 300) == 2);
}

TEST_CASE("display logic mode list respects the output cap" * doctest::test_suite("unit"))
{
    DisplayModeRec src[] = {M(640, 480, 60), M(800, 600, 60), M(1024, 768, 60)};
    DisplayModeRec out[4] = {M(7, 7, 7), M(7, 7, 7), M(7, 7, 7), M(7, 7, 7)};
    CHECK(BuildModeList(src, 3, 0, out, 2) == 3);
    CHECK(out[1].w == 800);
    CHECK(out[2].w == 7);
}

TEST_CASE("display logic unique resolutions" * doctest::test_suite("unit"))
{
    DisplayModeRec src[] = {M(640, 480, 60), M(800, 600, 60), M(800, 600, 75), M(1920, 1080, 60)};
    DisplayModeRec out[4];
    CHECK(UniqueResolutions(src, 4, 800, 600, out, 4) == 2);
    CHECK(out[0].w == 800);
    CHECK(out[1].w == 1920);
}

TEST_CASE("display logic desktop default exact and nearest" * doctest::test_suite("unit"))
{
    unsigned int w = 0, h = 0;
    DisplayModeRec a[] = {M(1280, 1024, 60), M(2560, 1440, 60), M(1920, 1080, 60)};
    DesktopDefault(a, 3, 2560, 1440, w, h);
    CHECK(w == 2560);
    CHECK(h == 1440);
    DisplayModeRec b[] = {M(1280, 1024, 60), M(1920, 1080, 60)};
    DesktopDefault(b, 2, 2560, 1440, w, h);
    CHECK(w == 1920);
    CHECK(h == 1080);
    DisplayModeRec c[] = {M(640, 480, 60)};
    DesktopDefault(c, 1, 3840, 2160, w, h);
    CHECK(w == 640);
    CHECK(h == 480);
}

TEST_CASE("display logic desktop default empty list" * doctest::test_suite("unit"))
{
    unsigned int w = 1, h = 1;
    DesktopDefault(0, 0, 1920, 1080, w, h);
    CHECK(w == 800);
    CHECK(h == 600);
}

TEST_CASE("display logic desktop default zero desktop" * doctest::test_suite("unit"))
{
    unsigned int w = 1, h = 1;
    DisplayModeRec a[] = {M(1920, 1080, 60)};
    DesktopDefault(a, 1, 0, 0, w, h);
    CHECK(w == 800);
    CHECK(h == 600);
}

TEST_CASE("display logic desktop default nothing fits" * doctest::test_suite("unit"))
{
    unsigned int w = 1, h = 1;
    DisplayModeRec a[] = {M(1920, 1080, 60)};
    DesktopDefault(a, 1, 1024, 768, w, h);
    CHECK(w == 800);
    CHECK(h == 600);
}

TEST_CASE("display logic refresh windowed or unrequested" * doctest::test_suite("unit"))
{
    DisplayModeRec a[] = {M(1920, 1080, 60), M(1920, 1080, 144)};
    CHECK(PickRefresh(a, 2, 1920, 1080, 144, true) == 0);
    CHECK(PickRefresh(a, 2, 1920, 1080, 0, false) == 0);
    CHECK(PickRefresh(a, 2, 1920, 1080, 144, false) == 144);
    CHECK(PickRefresh(a, 2, 1280, 720, 60, false) == 0);
}

TEST_CASE("display logic refresh fallback" * doctest::test_suite("unit"))
{
    DisplayModeRec a[] = {M(1920, 1080, 60), M(1920, 1080, 144), M(1280, 720, 240)};
    LogCapture capture;
    CHECK(PickRefresh(a, 3, 1920, 1080, 100, false) == 144);
    CHECK(capture.Contains("display: refresh 100Hz not listed"));
}

TEST_CASE("display logic adapter name beats ordinal" * doctest::test_suite("unit"))
{
    AdapterRec ads[] = {{0, "\\\\.\\DISPLAY1"}, {1, "\\\\.\\DISPLAY2"}};
    CHECK(ResolveAdapter(0, false, "\\\\.\\display2", ads, 2, false) == 1);
}

TEST_CASE("display logic adapter fallback" * doctest::test_suite("unit"))
{
    AdapterRec ads[] = {{0, "\\\\.\\DISPLAY1"}, {1, "\\\\.\\DISPLAY2"}};
    {
        LogCapture capture;
        CHECK(ResolveAdapter(1, false, "\\\\.\\DISPLAY9", ads, 2, false) == 1);
        CHECK(capture.Contains("display: adapter name"));
    }
    {
        LogCapture capture;
        CHECK(ResolveAdapter(5, false, "\\\\.\\DISPLAY9", ads, 2, false) == 0);
        CHECK(capture.Contains("not found, using adapter 0"));
    }
    {
        LogCapture capture;
        CHECK(ResolveAdapter(1, true, "\\\\.\\DISPLAY1", ads, 2, false) == 1);
        CHECK(!capture.Contains("display:"));
    }
    {
        LogCapture capture;
        CHECK(ResolveAdapter(5, true, "", ads, 2, false) == 0);
        CHECK(capture.Contains("display: adapter 5"));
    }
}

TEST_CASE("display logic adapter norender is silent" * doctest::test_suite("unit"))
{
    AdapterRec ads[] = {{0, "a"}};
    LogCapture capture;
    CHECK(ResolveAdapter(5, true, "x", ads, 1, true) == 0);
    CHECK(!capture.Contains("display:"));
}

TEST_CASE("display logic usable uncached" * doctest::test_suite("unit"))
{
    CHECK(UsableUncached(true, false, true, 0, 0));
    CHECK(UsableUncached(false, true, true, 0, 0));
    CHECK(UsableUncached(true, false, false, 4, 0));
    CHECK(!UsableUncached(true, false, false, 0, 0));
    CHECK(!UsableUncached(false, false, true, 4, 0));
}

TEST_CASE("display logic effective vsync precedence" * doctest::test_suite("unit"))
{
    CHECK(EffectiveVsync(true, 2, false) == 2);
    CHECK(EffectiveVsync(true, 0, true) == 0);
    CHECK(EffectiveVsync(false, 1, false) == 0);
    CHECK(EffectiveVsync(false, 0, true) == 1);
}

TEST_CASE("display logic vsync off and on" * doctest::test_suite("unit"))
{
    const unsigned long caps[] = {0, 0x1, 0x3, 0x80000001ul, 0x80000003ul};
    for (int w = 0; w < 2; ++w)
    {
        for (unsigned long c : caps)
        {
            bool fb = true;
            bool lacks = c != 0 && !(c & kIntervalImmediate);
            CHECK(ResolveVsyncInterval(0, c, w != 0, &fb) == (lacks ? kIntervalDefault : kIntervalImmediate));
            CHECK(fb == lacks);
            fb = true;
            CHECK(ResolveVsyncInterval(1, c, w != 0, &fb) == kIntervalDefault);
            CHECK(!fb);
        }
    }
}

TEST_CASE("display logic vsync half needs fullscreen and caps" * doctest::test_suite("unit"))
{
    const unsigned long caps[] = {0, 0x1, 0x3, 0x80000001ul, 0x80000003ul};
    for (int w = 0; w < 2; ++w)
    {
        for (unsigned long c : caps)
        {
            bool fb = false;
            bool ok = !w && (c & 2);
            CHECK(ResolveVsyncInterval(2, c, w != 0, &fb) == (ok ? kIntervalTwo : kIntervalDefault));
            CHECK(fb == !ok);
        }
    }
}

TEST_CASE("display logic vsync unknown request and null flag" * doctest::test_suite("unit"))
{
    bool fb = true;
    CHECK(ResolveVsyncInterval(7, 0x80000003ul, false, &fb) == kIntervalDefault);
    CHECK(!fb);
    CHECK(ResolveVsyncInterval(7, 0, true, nullptr) == kIntervalDefault);
    CHECK(ResolveVsyncInterval(0, 0x1, true, nullptr) == kIntervalDefault);
    CHECK(ResolveVsyncInterval(2, 0, true, nullptr) == kIntervalDefault);
    CHECK(ResolveVsyncInterval(2, 3, false, nullptr) == kIntervalTwo);
}

TEST_CASE("reset step maps every code" * doctest::test_suite("unit"))
{
    CHECK(ResetStep(S_OK, 0, 0xFFFFFFFFu, true, kRuntimeBounds) == kResetDone);
    CHECK(ResetStep(S_OK, 99, 0xFFFFFFFFu, false, kStartupBounds) == kResetDone);
    CHECK(ResetStep(0x0876017C, 1, 0, false, kRuntimeBounds) == kResetDone);
    CHECK(ResetStep(S_FALSE, 1, 0, false, kStartupBounds) == kResetDone);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 1, 0, true, kRuntimeBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 3, 100, false, kStartupBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DRIVERINTERNALERROR, 1, 0, true, kRuntimeBounds) == kResetFatal);
    CHECK(ResetStep(E_FAIL, 1, 0, true, kRuntimeBounds) == kResetFatal);
    CHECK(ResetStep(D3DERR_DEVICEREMOVED, 1, 0, true, kStartupBounds) == kResetFatal);
    CHECK(ResetStep(D3DERR_INVALIDCALL, 1, 0, true, kRuntimeBounds) == kResetRevert);
}

TEST_CASE("reset step lost device waits without limit" * doctest::test_suite("unit"))
{
    CHECK(kRuntimeBounds.uiMaxMs == 0);
    CHECK(ResetStep(D3DERR_DEVICELOST, 1, 0xFFFFFFFFu, false, kRuntimeBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DEVICELOST, 19, 0xFFFFFFFFu, true, kRuntimeBounds) == kResetWait);
    CHECK(CoopStep(D3DERR_DEVICELOST, 0, kRuntimeBounds) == kResetWait);
    CHECK(CoopStep(D3DERR_DEVICELOST, 30000, kRuntimeBounds) == kResetWait);
    CHECK(CoopStep(D3DERR_DEVICELOST, 0xFFFFFFFFu, kRuntimeBounds) == kResetWait);
}

TEST_CASE("coop step maps every code" * doctest::test_suite("unit"))
{
    CHECK(CoopStep(S_OK, 0, kRuntimeBounds) == kResetNow);
    CHECK(CoopStep(S_OK, 0xFFFFFFFFu, kStartupBounds) == kResetNow);
    CHECK(CoopStep(0x08760868, 0, kRuntimeBounds) == kResetNow);
    CHECK(CoopStep(D3DERR_DEVICENOTRESET, 0, kRuntimeBounds) == kResetNow);
    CHECK(CoopStep(D3DERR_DEVICENOTRESET, 0xFFFFFFFFu, kStartupBounds) == kResetNow);
    CHECK(CoopStep(D3DERR_DRIVERINTERNALERROR, 0, kRuntimeBounds) == kResetFatal);
    CHECK(CoopStep(D3DERR_DEVICEREMOVED, 0, kStartupBounds) == kResetFatal);
    CHECK(CoopStep(E_FAIL, 0, kRuntimeBounds) == kResetFatal);
}

TEST_CASE("coop step startup lost device times out without revert" * doctest::test_suite("unit"))
{
    CHECK(CoopStep(D3DERR_DEVICELOST, 29999, kStartupBounds) == kResetWait);
    CHECK(CoopStep(D3DERR_DEVICELOST, 30000, kStartupBounds) == kResetFatal);
    CHECK(CoopStep(D3DERR_DEVICELOST, 0xFFFFFFFFu, kStartupBounds) == kResetFatal);
}

TEST_CASE("reset step startup timeout is fatal" * doctest::test_suite("unit"))
{
    CHECK(kStartupBounds.uiMaxMs == 30000);
    CHECK(ResetStep(D3DERR_DEVICELOST, 5, 29999, false, kStartupBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DEVICELOST, 5, 30000, false, kStartupBounds) == kResetFatal);
    CHECK(ResetStep(D3DERR_DEVICELOST, 5, 30000, true, kStartupBounds) == kResetRevert);
    CHECK(ResetStep(D3DERR_DEVICELOST, 500, 29999, false, kStartupBounds) == kResetWait);
}

TEST_CASE("reset step runtime failure bound" * doctest::test_suite("unit"))
{
    CHECK(kRuntimeBounds.uiMaxFailures == 20);
    CHECK(ResetStep(D3DERR_DEVICELOST, 19, 0xFFFFFFFFu, false, kRuntimeBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DEVICELOST, 20, 0, false, kRuntimeBounds) == kResetFatal);
    CHECK(ResetStep(D3DERR_DEVICELOST, 20, 0, true, kRuntimeBounds) == kResetRevert);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 19, 0, false, kRuntimeBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 20, 0, false, kRuntimeBounds) == kResetFatal);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 20, 0, true, kRuntimeBounds) == kResetRevert);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 1, 30000, false, kStartupBounds) == kResetFatal);
}

TEST_CASE("reset step reverts once then fatal" * doctest::test_suite("unit"))
{
    const long codes[] = {D3DERR_INVALIDCALL, D3DERR_NOTAVAILABLE, E_OUTOFMEMORY,
                          D3DERR_OUTOFVIDEOMEMORY};
    for (long hr : codes)
    {
        CHECK(ResetStep(hr, 1, 0, true, kRuntimeBounds) == kResetRevert);
        CHECK(ResetStep(hr, 1, 0, false, kRuntimeBounds) == kResetFatal);
    }
    const long unknown = (long)0x88760999;
    CHECK(ResetStep(unknown, 1, 0, true, kRuntimeBounds) == kResetFatal);
    CHECK(ResetStep(unknown, 1, 0, false, kRuntimeBounds) == kResetFatal);
}
