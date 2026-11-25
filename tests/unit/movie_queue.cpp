#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "Battlefront2/Source/GameMovie.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/movie/RedMovie.h"
#include "SoundFL/source/sndqueue.h"

namespace MovieQueueStub
{
extern int sPlayCalls;
extern int sGamePlayCalls;
extern bool sOpen;
extern unsigned int sKnownHash;
}

namespace
{
struct MovieQueueFixture
{
    RedMovie* pRedMovie;
    alignas(16) unsigned char acMovie[sizeof(RedMovie)];

    MovieQueueFixture() : pRedMovie(GameMovie::sRedMovie)
    {
        MovieQueueStub::sPlayCalls = 0;
        MovieQueueStub::sGamePlayCalls = 0;
        MovieQueueStub::sOpen = false;
        MovieQueueStub::sKnownHash = PblHash("queued_movie");
        Drain();
        g_InGameMoviePlayerState = 2;
        g_InGameMoviePlayerFrame = 4;
    }

    ~MovieQueueFixture()
    {
        Drain();
        g_InGameMoviePlayerState = 0;
        g_InGameMoviePlayerFrame = 0;
        GameMovie::sRedMovie = pRedMovie;
    }

    static void Drain()
    {
        unsigned int uiName = 0;
        while (!g_InGameMovieQueue.IsEmpty())
        {
            g_InGameMovieQueue.Get(uiName);
        }
    }

    static void Queue(const char* pcName)
    {
        CHECK(g_InGameMovieQueue.Add((unsigned int)PblHash(pcName)));
    }
};
}

TEST_CASE_NAMED(FMovieQueueTest1, "Unit::MovieQueue::queued in-game movies are drained without Play when no movie exists", "[EngineFilter]")
{
    MovieQueueFixture f;
    GameMovie::sRedMovie = 0;
    f.Queue("queued_movie");
    f.Queue("queued_movie");

    UpdatInGameMovie();

    CHECK(MovieQueueStub::sGamePlayCalls == 1);
    CHECK(MovieQueueStub::sPlayCalls == 0);
    CHECK(g_InGameMovieQueue.IsEmpty());
    CHECK(g_InGameMoviePlayerState == 3);
    CHECK(g_InGameMoviePlayerFrame == 5);
}

TEST_CASE_NAMED(FMovieQueueTest2, "Unit::MovieQueue::queued in-game movies are drained without Play when the movie failed to open", "[EngineFilter]")
{
    MovieQueueFixture f;
    GameMovie::sRedMovie = reinterpret_cast<RedMovie*>(f.acMovie);
    f.Queue("queued_movie");
    f.Queue("queued_movie");

    UpdatInGameMovie();

    CHECK(MovieQueueStub::sPlayCalls == 0);
    CHECK(g_InGameMovieQueue.IsEmpty());
    CHECK(g_InGameMoviePlayerState == 3);
}

TEST_CASE_NAMED(FMovieQueueTest3, "Unit::MovieQueue::queued in-game movies play once per known name when the movie is open", "[EngineFilter]")
{
    MovieQueueFixture f;
    GameMovie::sRedMovie = reinterpret_cast<RedMovie*>(f.acMovie);
    MovieQueueStub::sOpen = true;
    f.Queue("queued_movie");
    f.Queue("unknown_movie");
    f.Queue("queued_movie");

    UpdatInGameMovie();

    CHECK(MovieQueueStub::sPlayCalls == 2);
    CHECK(g_InGameMovieQueue.IsEmpty());
    CHECK(g_InGameMoviePlayerState == 3);
}

TEST_CASE_NAMED(FMovieQueueTest4, "Unit::MovieQueue::the in-game movie queue drain waits for frame 4", "[EngineFilter]")
{
    MovieQueueFixture f;
    GameMovie::sRedMovie = reinterpret_cast<RedMovie*>(f.acMovie);
    MovieQueueStub::sOpen = true;
    g_InGameMoviePlayerFrame = 3;
    f.Queue("queued_movie");

    UpdatInGameMovie();

    CHECK(MovieQueueStub::sGamePlayCalls == 0);
    CHECK(MovieQueueStub::sPlayCalls == 0);
    CHECK_FALSE(g_InGameMovieQueue.IsEmpty());
    CHECK(g_InGameMoviePlayerState == 2);
    CHECK(g_InGameMoviePlayerFrame == 4);
}
