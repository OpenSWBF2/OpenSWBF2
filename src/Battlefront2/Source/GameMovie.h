#pragma once

class RedMovie;

class GameMovie
{
public:
    static bool IsOpen();

    // FUNCTION: SWBF2 0x005343f0
    static void Create(const int defaultHeap);

    static void Destroy(bool bDestroyAll);

    static void DestroyKeepProperty();

    static void Open();
    static void Open(const char* const pszFile, const char* const pszName, const int iHeap1,
                     const int iHeap2, const bool bAllocTextureHeap = false,
                     const unsigned int uiReadBufferSize = 0xFFFFFFFFu,
                     const unsigned int uiReadSize = 0xFFFFFFFFu);

    static void Close();

    static void Render();
    static void EndFrame();

    static void Update(float fDeltaTime);

    static bool IsPlaying();
    static void StartInGameMoviePlay(const char* pcFile, const char* pcName);

    static void Play(const char* const pcProperties, const char* const pcPropertiesAtEnd,
                     unsigned int uiY, unsigned int uiX, unsigned int uiWidth,
                     unsigned int uiHeight, bool bLoop);
    static bool PropertiesPlaying(const char* const pcProperties, bool bCurrent);
    static void SetAudioBus(const char* const pcBusName);

    static void Stop();

    static RedMovie* sRedMovie;

    static bool sUseMovies;

    static char* sFilename;

    static int sDefaultHeap;
public:
    static int sDataHeap;
    static int sTextureHeap;
    static bool sMemoryReserved;
};

void UpdatInGameMovie();

void StopInGameMoviePlay();
void EnableSceneAndInterface(bool bEnable);
