#include "GameMovie.h"

#include "RedEngineFL/movie/RedMovie.h"
#include "RedEngineFL/Interface/RedInterfaceScreen.h"
#include "SoundFL/source/sndqueue.h"
#include "GameMemory.h"
#include "GameSoundEngine.h"

// FUNCTION: SWBF2 0x00534d40
void UpdatInGameMovie()
{
    if (g_InGameMoviePlayerState == 0)
    {
        if (GameMovie::IsOpen())
        {
            StopInGameMoviePlay();
        }
    }
    else if (g_InGameMoviePlayerState == 1)
    {
        if (g_InGameMoviePlayerFrame == 0)
        {
            GameSoundEngine::StopAll();
            GameMovie::Create(GameMemory::RunTimeHeap);
            GameMovie::Open(g_InGameMovieFilename, ingame_movie_idname,
                            GameMemory::RunTimeHeap, GameMemory::RunTimeHeap);
            g_InGameMoviePlayerState = 2;
            EnableSceneAndInterface(false);
        }
        ++g_InGameMoviePlayerFrame;
    }
    else if (g_InGameMoviePlayerState == 2)
    {
        if (g_InGameMoviePlayerFrame == 4)
        {
            GameMovie::Play(ingame_movie_idname, "", 0, 0,
                            RedInterfaceScreen::GetScreenWidth(),
                            RedInterfaceScreen::GetScreenHeight(), false);
            RedMovie* const pMovie = GameMovie::sRedMovie;
            const bool bOpen = GameMovie::IsOpen();
            while (!g_InGameMovieQueue.IsEmpty())
            {
                const unsigned int* pName = g_InGameMovieQueue.IsEmpty()
                    ? 0
                    : (const unsigned int*)(g_InGameMovieQueue.QueueBase::Get(
                        sizeof(unsigned int), g_InGameMovieQueue.mElements, 12));
                const RedMovie::Properties* pProperty = RedMovie::Properties::FindByHashID(*pName);
                if (pProperty && bOpen)
                {
                    pMovie->Play(pProperty, true);
                }
            }
            g_InGameMoviePlayerState = 3;
        }
        ++g_InGameMoviePlayerFrame;
    }
    else if (g_InGameMoviePlayerState == 3)
    {
        if (GameMovie::IsOpen())
        {
            GameMovie::Update(1.0f);
        }
        ++g_InGameMoviePlayerFrame;
        if (!GameMovie::IsPlaying())
        {
            StopInGameMoviePlay();
        }
    }
}
