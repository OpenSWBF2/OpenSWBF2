#include "third_party/doctest/doctest.h"

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
