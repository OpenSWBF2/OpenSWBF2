#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "RedEngineFL/movie/pc/pcRedMovieClock.h"

TEST_CASE_NAMED(FMovieClockTest1, "Unit::MovieClock::movie clock frame 1 is due at once and frame 2 at played 1600 for 48000 Hz at 30 fps", "[SmokeFilter]")
{
    CHECK(MovieFrameDue(0, 48000, 30, 1, 1));
    CHECK_FALSE(MovieFrameDue(1599, 48000, 30, 1, 2));
    CHECK(MovieFrameDue(1600, 48000, 30, 1, 2));
    CHECK_FALSE(MovieFrameDue(3199, 48000, 30, 1, 3));
    CHECK(MovieFrameDue(3200, 48000, 30, 1, 3));
}

TEST_CASE_NAMED(FMovieClockTest2, "Unit::MovieClock::movie clock frame 300 boundary for 44100 Hz at 2997 over 100 fps", "[SmokeFilter]")
{
    CHECK_FALSE(MovieFrameDue(439969, 44100, 2997, 100, 300));
    CHECK(MovieFrameDue(439970, 44100, 2997, 100, 300));
}

TEST_CASE_NAMED(FMovieClockTest3, "Unit::MovieClock::movie clock frame 7314 of a retail rate movie keeps 64 bit width", "[SmokeFilter]")
{
    CHECK_FALSE(MovieFrameDue(1601, 48000, 5000000, 166833, 2));
    CHECK(MovieFrameDue(1602, 48000, 5000000, 166833, 2));
    CHECK_FALSE(MovieFrameDue(11712477ull, 48000, 5000000, 166833, 7314));
    CHECK(MovieFrameDue(11712478ull, 48000, 5000000, 166833, 7314));
}

TEST_CASE_NAMED(FMovieClockTest4, "Unit::MovieClock::movie clock compares exactly at the header caps", "[SmokeFilter]")
{
    CHECK_FALSE(MovieFrameDueTicks(319989999999ull, 10000000ull, 0xFFFFFFFFu, 0xFFFFFFFFu, 32000));
    CHECK(MovieFrameDueTicks(319990000000ull, 10000000ull, 0xFFFFFFFFu, 0xFFFFFFFFu, 32000));
    CHECK_FALSE(MovieFrameDueTicks(1145288820872ull, 4294967296ull, 4294967280u, 35791394u, 32000));
    CHECK(MovieFrameDueTicks(1145288820873ull, 4294967296ull, 4294967280u, 35791394u, 32000));
    CHECK_FALSE(MovieFrameDue(1535951999ull, 48000, 0xFFFFFFFFu, 0xFFFFFFFFu, 32000));
    CHECK(MovieFrameDue(1535952000ull, 48000, 0xFFFFFFFFu, 0xFFFFFFFFu, 32000));
    CHECK(MovieFrameDue(0xFFFFFFFFFFFFFFFFull, 48000, 1, 1, 0xFFFFFFFFu));
    CHECK_FALSE(MovieFrameDueTicks(0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull, 1, 0xFFFFFFFFu, 3));
    CHECK(MovieFrameDueTicks(0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFu, 0xFFFFFFFFu, 2));
}

TEST_CASE_NAMED(FMovieClockTest5, "Unit::MovieClock::movie clock wide product matches the exact 128 bit result", "[SmokeFilter]")
{
    const MovieWide kMax = MovieMulWide(0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull);
    CHECK(kMax.uHi == 0xFFFFFFFFFFFFFFFEull);
    CHECK(kMax.uLo == 1ull);
    const MovieWide kCap = MovieMulWide(31999ull * 35791394ull, 4294967296ull);
    CHECK(kCap.uHi == 266ull);
    CHECK(kCap.uLo == 12144088190570987520ull);
    const MovieWide kSmall = MovieMulWide(48000ull, 166833ull);
    CHECK(kSmall.uHi == 0ull);
    CHECK(kSmall.uLo == 8007984000ull);
    CHECK(MovieWideAtLeast(kMax, kCap));
    CHECK_FALSE(MovieWideAtLeast(kCap, kMax));
    CHECK(MovieWideAtLeast(kCap, kCap));
}

TEST_CASE_NAMED(FMovieClockTest6, "Unit::MovieClock::movie clock never stalls on a zero rate or fps", "[SmokeFilter]")
{
    CHECK(MovieFrameDue(0, 0, 30, 1, 9));
    CHECK(MovieFrameDue(0, 48000, 0, 1, 9));
    CHECK(MovieFrameDue(0, 48000, 30, 0, 9));
    CHECK(MovieFrameDue(0, 48000, 30, 1, 0));
    CHECK(MovieFrameDueTicks(0, 0, 30, 1, 9));
    CHECK(MovieFrameDueTicks(0, 10000000ull, 0, 1, 9));
    CHECK(MovieFrameDueTicks(0, 10000000ull, 30, 0, 9));
    CHECK(MovieFrameDueTicks(0, 10000000ull, 30, 1, 0));
}

TEST_CASE_NAMED(FMovieClockTest7, "Unit::MovieClock::movie clock timer variant paces on the 10 MHz performance counter", "[SmokeFilter]")
{
    CHECK(MovieFrameDueTicks(0, 10000000ull, 30, 1, 1));
    CHECK_FALSE(MovieFrameDueTicks(333333ull, 10000000ull, 30, 1, 2));
    CHECK(MovieFrameDueTicks(333334ull, 10000000ull, 30, 1, 2));
    CHECK_FALSE(MovieFrameDueTicks(333665ull, 10000000ull, 5000000, 166833, 2));
    CHECK(MovieFrameDueTicks(333666ull, 10000000ull, 5000000, 166833, 2));
    CHECK_FALSE(MovieFrameDueTicks(2440099457ull, 10000000ull, 5000000, 166833, 7314));
    CHECK(MovieFrameDueTicks(2440099458ull, 10000000ull, 5000000, 166833, 7314));
}

TEST_CASE_NAMED(FMovieClockTest8, "Unit::MovieClock::movie clock cursor advance covers no wrap and an exact wrap and a wrap past zero", "[SmokeFilter]")
{
    CHECK(MovieCursorAdvance(100, 100, 96000) == 0);
    CHECK(MovieCursorAdvance(100, 4100, 96000) == 4000);
    CHECK(MovieCursorAdvance(95000, 0, 96000) == 1000);
    CHECK(MovieCursorAdvance(95000, 3000, 96000) == 4000);
    CHECK(MovieCursorAdvance(95999, 95998, 96000) == 95999);
}

TEST_CASE_NAMED(FMovieClockTest9, "Unit::MovieClock::movie playback time follows the audio clock through a retail length movie and stops when the audio stops", "[SmokeFilter]")
{
    const unsigned int uRate = 48000;
    const unsigned int uFpsNum = 5000000;
    const unsigned int uFpsDen = 166833;
    const unsigned int uFrames = 7314;
    const double dPeriod = (double)uFpsDen / (double)uFpsNum;
    unsigned int uFrame = 1;
    unsigned int uShown = 0;
    double dWorstLead = 0.0;
    double dLeastLead = 1.0;
    unsigned long long uPlayed = 0;
    for (; uShown < uFrames; uPlayed += 800)
    {
        if (MovieFrameDue(uPlayed, uRate, uFpsNum, uFpsDen, uFrame))
        {
            ++uShown;
            if (uFrame < uFrames)
            {
                ++uFrame;
            }
        }
        const double dLead = (double)MoviePlaybackTime(uFpsNum, uFpsDen, uFrame) - (double)uPlayed / (double)uRate;
        dWorstLead = dLead > dWorstLead ? dLead : dWorstLead;
        dLeastLead = dLead < dLeastLead ? dLead : dLeastLead;
    }
    CHECK(uFrame == uFrames);
    CHECK(dLeastLead > 0.0);
    CHECK(dWorstLead <= 2.0 * dPeriod + 1e-4);
    unsigned int uHeldFrame = 300;
    float fFirst = 0.0f;
    for (int i = 0; i < 100; ++i)
    {
        if (MovieFrameDue(479000ull, uRate, uFpsNum, uFpsDen, uHeldFrame))
        {
            ++uHeldFrame;
        }
        if (i == 0)
        {
            fFirst = MoviePlaybackTime(uFpsNum, uFpsDen, uHeldFrame);
        }
    }
    CHECK(uHeldFrame == 301);
    CHECK(MoviePlaybackTime(uFpsNum, uFpsDen, uHeldFrame) == fFirst);
    CHECK(MoviePlaybackTime(uFpsNum, uFpsDen, 300) == doctest::Approx(300.0 * dPeriod).epsilon(1e-6));
    CHECK(MoviePlaybackTime(uFpsNum, uFpsDen, 7314) == doctest::Approx(7314.0 * dPeriod).epsilon(1e-6));
}
