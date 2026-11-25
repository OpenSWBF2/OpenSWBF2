#include "third_party/doctest/doctest.h"

#include <string>
#include <vector>

#include "testutil.h"
#include "RedEngineFL/movie/pc/pcRedMovieAudioQueue.h"

namespace
{

struct FakeQueue
{
    unsigned int uRoom;
    std::vector<short> vGot;
};

unsigned int FakeQueueTake(void* pContext, const short* pSamples, unsigned int uFrames)
{
    FakeQueue* p = (FakeQueue*)pContext;
    const unsigned int uTake = uFrames < p->uRoom ? uFrames : p->uRoom;
    p->vGot.insert(p->vGot.end(), pSamples, pSamples + (size_t)uTake * 2);
    p->uRoom -= uTake;
    return uTake;
}

std::vector<short> Ramp(unsigned int uFrames, int iStart)
{
    std::vector<short> v;
    for (unsigned int i = 0; i < uFrames; ++i)
    {
        v.push_back((short)(iStart + (int)i));
        v.push_back((short)-(iStart + (int)i));
    }
    return v;
}

struct HoldFixture
{
    std::vector<short> vStore;
    MovieAudioHold kHold;
    FakeQueue kQueue;

    HoldFixture() : vStore((48000 + 100000) * 2, (short)0x5A5A)
    {
        kHold.pHold = vStore.data();
        kHold.uChannels = 2;
        kHold.uHeld = 0;
        kHold.ullDropped = 0;
        kQueue.uRoom = 0;
    }

    bool TailUntouched() const
    {
        for (size_t i = 48000 * 2; i < vStore.size(); ++i)
        {
            if (vStore[i] != (short)0x5A5A)
            {
                return false;
            }
        }
        return true;
    }
};

}

TEST_CASE("movie audio hold fills to exactly 48000 frames and counts the rest as dropped" * doctest::test_suite("unit"))
{
    HoldFixture f;
    const std::vector<short> vFirst = Ramp(30000, 0);
    const std::vector<short> vSecond = Ramp(20000, -20000);
    MovieAudioFeed(f.kHold, FakeQueueTake, &f.kQueue, vFirst.data(), 30000);
    CHECK(f.kHold.uHeld == 30000);
    CHECK(f.kHold.ullDropped == 0);
    MovieAudioFeed(f.kHold, FakeQueueTake, &f.kQueue, vSecond.data(), 20000);
    CHECK(f.kHold.uHeld == 48000);
    CHECK(f.kHold.ullDropped == 2000);
    MovieAudioFeed(f.kHold, FakeQueueTake, &f.kQueue, vSecond.data(), 10);
    CHECK(f.kHold.uHeld == 48000);
    CHECK(f.kHold.ullDropped == 2010);
    CHECK(f.kQueue.vGot.empty());
    CHECK(f.vStore[0] == 0);
    CHECK(f.vStore[2 * 29999] == 29999);
    CHECK(f.vStore[2 * 30000] == -20000);
    CHECK(f.vStore[2 * 30000 + 1] == 20000);
    CHECK(f.vStore[2 * 47999] == -2001);
    CHECK(f.vStore[2 * 47999 + 1] == 2001);
    CHECK(f.TailUntouched());
}

TEST_CASE("movie audio queue takes the hold first and keeps the remainder in order" * doctest::test_suite("unit"))
{
    HoldFixture f;
    const std::vector<short> vOld = Ramp(10, 0);
    const std::vector<short> vNew = Ramp(5, 100);
    MovieAudioFeed(f.kHold, FakeQueueTake, &f.kQueue, vOld.data(), 10);
    REQUIRE(f.kHold.uHeld == 10);
    f.kQueue.uRoom = 4;
    MovieAudioFeed(f.kHold, FakeQueueTake, &f.kQueue, vNew.data(), 5);
    CHECK(f.kQueue.vGot == Ramp(4, 0));
    CHECK(f.kHold.uHeld == 11);
    const short asWant[11] = {4, 5, 6, 7, 8, 9, 100, 101, 102, 103, 104};
    for (int i = 0; i < 11; ++i)
    {
        CHECK(f.vStore[2 * i] == asWant[i]);
        CHECK(f.vStore[2 * i + 1] == -asWant[i]);
    }
    f.kQueue.vGot.clear();
    f.kQueue.uRoom = 1000;
    MovieAudioFeed(f.kHold, FakeQueueTake, &f.kQueue, 0, 0);
    CHECK(f.kHold.uHeld == 0);
    REQUIRE(f.kQueue.vGot.size() == 22);
    for (int i = 0; i < 11; ++i)
    {
        CHECK(f.kQueue.vGot[2 * i] == asWant[i]);
    }
    f.kQueue.vGot.clear();
    f.kQueue.uRoom = 3;
    MovieAudioFeed(f.kHold, FakeQueueTake, &f.kQueue, vNew.data(), 5);
    CHECK(f.kQueue.vGot == Ramp(3, 100));
    CHECK(f.kHold.uHeld == 2);
    CHECK(f.vStore[0] == 103);
    CHECK(f.vStore[2] == 104);
    CHECK(f.kHold.ullDropped == 0);
    CHECK(f.TailUntouched());
}

TEST_CASE("movie audio starves at the start threshold only while not playing" * doctest::test_suite("unit"))
{
    CHECK(MovieAudioStartFrames(96000, 4) == 12000);
    CHECK(MovieAudioStartFrames(96002, 4) == 12001);
    CHECK(MovieAudioStartFrames(88200, 2) == 22050);
    CHECK(MovieAudioStarved(0, 11999, 0, false, 12000));
    CHECK_FALSE(MovieAudioStarved(0, 12000, 0, false, 12000));
    CHECK_FALSE(MovieAudioStarved(500, 500, 12000, false, 12000));
    CHECK(MovieAudioStarved(500, 500, 11999, false, 12000));
    CHECK_FALSE(MovieAudioStarved(11999, 12000, 0, true, 12000));
    CHECK(MovieAudioStarved(12000, 12000, 0, true, 12000));
    CHECK(MovieAudioStarved(13000, 12000, 0, true, 12000));
}

TEST_CASE("movie audio counts played bytes across a wrap and at the end and not on a stop" * doctest::test_suite("unit"))
{
    CHECK(MovieAudioPlayedStep(10000, 95000, 96000, 6000, 3000, false) == 4000);
    CHECK(MovieAudioPlayedStep(10000, 1000, 96000, 6000, 5000, false) == 4000);
    CHECK(MovieAudioPlayedStep(1000, 1000, 96000, 500, 5000, false) == 1000);
    CHECK(MovieAudioPlayedStep(10000, 1000, 96000, 0, 2000, false) == 10000);
    CHECK(MovieAudioPlayedStep(0, 1000, 96000, 0, 1000, false) == 0);
    CHECK(MovieAudioPlayedStep(10000, 1000, 96000, 0, 0, true) == 0);
    CHECK(MovieAudioPlayedStep(10000, 1000, 96000, 6000, 5000, true) == 0);
}

TEST_CASE("movie audio renderer sets the hardware gain flag before the base update that flush clears" * doctest::test_suite("unit"))
{
    std::string sAudio;
    REQUIRE(ReadRepoText("src/RedEngineFL/movie/pc/pcRedMovieAudio.cpp", sAudio));
    const size_t uBody = sAudio.find("void MovieAudioRenderer::UpdateObject()");
    REQUIRE(uBody != std::string::npos);
    const size_t uEnd = sAudio.find("\n}", uBody);
    REQUIRE(uEnd != std::string::npos);
    const std::string sBody = sAudio.substr(uBody, uEnd - uBody);
    const size_t uSet = sBody.find("Snd::DSBufferRenderer::mFlags |= kMovieHardwareGain;");
    const size_t uBase = sBody.find("Snd::DSBufferRenderer::UpdateObject();");
    REQUIRE(uBase != std::string::npos);
    CHECK(uSet != std::string::npos);
    CHECK(uSet < uBase);
    CHECK(sAudio.find("const unsigned char kMovieHardwareGain = 0x10;") != std::string::npos);
    std::string sRenderer;
    REQUIRE(ReadRepoText("src/SoundFL/source/win/snddsbufferrenderer.cpp", sRenderer));
    const size_t uFlush = sRenderer.find("void DSBufferRenderer::Flush()");
    REQUIRE(uFlush != std::string::npos);
    const size_t uFlushEnd = sRenderer.find("\n}", uFlush);
    REQUIRE(uFlushEnd != std::string::npos);
    const std::string sFlush = sRenderer.substr(uFlush, uFlushEnd - uFlush);
    CHECK(sFlush.find("mFlags = (unsigned char)(mFlags & 8u);") != std::string::npos);
    CHECK(sRenderer.find("if ((mFlags & 0x10) != 0)") != std::string::npos);
}
