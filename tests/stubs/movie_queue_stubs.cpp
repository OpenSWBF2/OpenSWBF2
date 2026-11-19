#include "Battlefront2/Source/GameMovie.h"
#include "Battlefront2/Source/GameSoundEngine.h"
#include "RedEngineFL/Interface/RedInterfaceScreen.h"
#include "RedEngineFL/movie/RedMovie.h"
#include "SoundFL/source/sndqueue.h"

namespace MovieQueueStub
{
int sPlayCalls;
int sGamePlayCalls;
bool sOpen;
unsigned int sKnownHash;
}

int g_InGameMoviePlayerState;
int g_InGameMoviePlayerFrame;
char ingame_movie_idname[256];
char g_InGameMovieFilename[256];
Snd::Queue<unsigned int, 12> g_InGameMovieQueue;

RedMovie* GameMovie::sRedMovie;

bool GameMovie::IsOpen()
{
    return GameMovie::sRedMovie != 0 && MovieQueueStub::sOpen;
}

void GameMovie::Create(const int)
{
}

void GameMovie::Open(const char* const, const char* const, const int, const int, const bool, const unsigned int,
                     const unsigned int)
{
}

void GameMovie::Play(const char* const, const char* const, unsigned int, unsigned int, unsigned int, unsigned int,
                     bool)
{
    ++MovieQueueStub::sGamePlayCalls;
}

void GameMovie::Update(float)
{
}

bool GameMovie::IsPlaying()
{
    return false;
}

void EnableSceneAndInterface(bool)
{
}

void StopInGameMoviePlay()
{
}

namespace GameSoundEngine
{
void StopAll()
{
}
}

const RedMovie::Properties* __cdecl RedMovie::Properties::FindByHashID(unsigned int uiHashID)
{
    static unsigned char sProperties[sizeof(RedMovie::Properties)];
    return uiHashID == MovieQueueStub::sKnownHash ? reinterpret_cast<const RedMovie::Properties*>(sProperties) : 0;
}

void RedMovie::Play(const Properties* const, bool)
{
    ++MovieQueueStub::sPlayCalls;
}

RedInterfaceScreen RedInterfaceScreen::s_screenFull;

RedInterfaceScreen::RedInterfaceScreen()
{
}

RedInterfaceScreen::~RedInterfaceScreen()
{
}
