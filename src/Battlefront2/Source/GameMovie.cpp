#include "GameMovie.h"

#include <string.h>

#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/movie/RedMovie.h"
#include "SoundFL/source/sndbus.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblUnicode.h"
#include "GameMemory.h"
#include "LoadUtil.h"
#include "GameState.h"
#include "CameraManager.h"
#include "Character.h"
#include "HUDManager.h"
#include "Battlefront2/Platform/Pc/pcSpawnDisplay.h"
#include "RedEngineFL/Graphics/PC/pcRedRenderer.h"
#include "RedEngineFL/Application/pc/pcFLRender.h"
#include "RedEngineFL/Interface/RedInterfaceScreen.h"
#include "SoundFL/source/sndqueue.h"
#include "GameSoundEngine.h"
#include "PebbleFL/Common/PblTime.h"
#ifdef OPENSWBF2_TEST
#include "CommandLine.h"
#include "GameLoop.h"
#include "RedEngineFL/Util/RedLog.h"
#endif

#include "PebbleFL/Common/PblMath.h"
#include "RedEngineFL/Interface/Red3DModelElement.h"
#include "RedEngineFL/Interface/RedBitmapElement.h"
#include "RedEngineFL/Interface/RedBorderElement.h"
#include "RedEngineFL/Interface/RedHotSpot.h"
#include "RedEngineFL/Interface/RedIFScreen.h"
#include "FLJoystick.h"
#include "RedEngineFL/input/RedKeyboard.h"

static const float g_MovieStopDelayTime = 2.0f;

// GLOBAL: SWBF2 0x01e56278
unsigned __int64 g_InGameMovieStartTime;
float g_InGameMovieStartSystemTime;

extern float g_fFixedRate;

// GLOBAL: SWBF2 0x01e56274
RedMovie* GameMovie::sRedMovie;

// GLOBAL: SWBF2 0x01e56136
bool GameMovie::sMemoryReserved;

// GLOBAL: SWBF2 0x01e56137

bool g_InGameMoviePlayerUpdateEnable;

// GLOBAL: SWBF2 0x007e67c8
int GameMovie::sDefaultHeap = -1;

// GLOBAL: SWBF2 0x007e67cc
int GameMovie::sDataHeap = -1;

// GLOBAL: SWBF2 0x007e68e0
int GameMovie::sTextureHeap = -1;

// GLOBAL: SWBF2 0x01e56284
char* GameMovie::sFilename;

// GLOBAL: SWBF2 0x01faa680
unsigned int gMovieOpenFailures;

#ifdef OPENSWBF2_TEST
static bool sbMovieTestOwner;

static bool MovieTestBlocks()
{
    return (gMovieTestFile[0] || gMovieLoops > 0) && !sbMovieTestOwner;
}
#endif

// FUNCTION: SWBF2 0x005343f0
void GameMovie::Create(const int defaultHeap)
{
    if (GameMovie::sUseMovies)
    {

        sDefaultHeap = defaultHeap == -1 ? GameMemory::RunTimeHeap : defaultHeap;
        const int iOldHeap = RedSetCurrentHeap(sDefaultHeap);

        sRedMovie = new RedMovie;

        RedSetCurrentHeap(iOldHeap);

        if (GameState::GetState() != 0x11E1FC01 /* "shell" */)
        {

            Snd::Bus* pBus = Snd::Bus::FindByHashID(0xF4EB2B40 /* "shellmovies" */);
            if (!pBus)
            {
                pBus = &Snd::Bus::smRootBus;
            }

            sRedMovie->SetAudioBus(pBus->mHashID);
        }

        sMemoryReserved = (sRedMovie != 0);
    }
}

// FUNCTION: SWBF2 0x005344c0
void GameMovie::Destroy(bool bDestroyAll)
{
    if (bDestroyAll)
    {
        RedMovie::DestroyAll();
    }

    if (sRedMovie)
    {
        if (sRedMovie->IsOpen())
        {
            Close();
        }

        sFilename = 0;
        int iOldHeap = RedSetCurrentHeap(sDefaultHeap);
        delete sRedMovie;
        RedSetCurrentHeap(iOldHeap);

        sMemoryReserved = false;
        sDefaultHeap = -1;
        sRedMovie = 0;
    }
}

// FUNCTION: SWBF2 0x00534540
void GameMovie::DestroyKeepProperty()
{
    Destroy(false);
}

// FUNCTION: SWBF2 0x00534550
void GameMovie::Open()
{
#ifdef OPENSWBF2_TEST
    if (MovieTestBlocks())
    {
        return;
    }
#endif
    if (GameMovie::sUseMovies)
    {
        Open(sFilename, sFilename + strlen(sFilename), GameMovie::sDataHeap, GameMovie::sTextureHeap);
    }
}

// FUNCTION: SWBF2 0x00534590
void GameMovie::Open(const char* const pszFile, const char* const pszName, const int iHeap1,
                     const int iHeap2, const bool, const unsigned int uiReadBufferSize,
                     const unsigned int uiReadSize)
{
    if (!GameMovie::sUseMovies)
    {
        return;
    }

#ifdef OPENSWBF2_TEST
    if (MovieTestBlocks())
    {
        return;
    }
#endif

    if (sFilename != pszFile)
    {
        const char* pszPath = pszFile;
        bool bDiscPrefix = _strnicmp(pszPath, "dc:", 3) == 0;
        if (bDiscPrefix)
        {
            pszPath += 3;
        }

        char szFullName[260];
        LoadUtil::MakeFullName(pszPath, LoadUtil::FILE_TYPE_NONE, szFullName, 128, bDiscPrefix);

        if (sFilename)
        {
            RedFreeToHeap(sDefaultHeap, sFilename, 0);
            sFilename = 0;
        }

        sFilename = (char*)RedAllocFromHeap(sDefaultHeap, strlen(szFullName) + strlen(pszName) + 2, 0);
        strlcpy(sFilename, szFullName, strlen(szFullName) + 1);
        strlcpy(sFilename + strlen(szFullName) + 1, pszName, strlen(pszName) + 1);
    }

    GameMovie::sDataHeap = iHeap1 == -1 ? sDefaultHeap : iHeap1;
    GameMovie::sTextureHeap = iHeap2 == -1 ? sDefaultHeap : iHeap2;

    if (sRedMovie && !GameMemory::TempLoadHeap)
    {
        if (!sRedMovie->Open(sFilename, GameMovie::sDataHeap, GameMovie::sTextureHeap, PblHash(pszName), uiReadBufferSize,
                             uiReadSize, 3))
        {
            ++gMovieOpenFailures;
        }
    }
}

// FUNCTION: SWBF2 0x00534750
void GameMovie::Close()
{
#ifdef OPENSWBF2_TEST
    if (MovieTestBlocks())
    {
        return;
    }
#endif
    if (GameMovie::sUseMovies)
    {
        if (sRedMovie && sRedMovie->IsOpen())
        {
            sRedMovie->Close();
        }

        if (sFilename)
        {
            RedFreeToHeap(sDefaultHeap, sFilename, 0);
            sFilename = 0;
        }
    }
}

// FUNCTION: SWBF2 0x00534a30
void GameMovie::Render()
{
#ifdef OPENSWBF2_TEST
    static unsigned int suHeldRenders;
    if (gMovieFault == kMovieFaultHold && ++suHeldRenders % 3 != 0)
    {
        return;
    }
#endif
    if (GameMovie::sUseMovies && sRedMovie && sRedMovie->IsOpen() && sRedMovie->IsPlaying())
    {
        sRedMovie->Render();
    }
}

// FUNCTION: SWBF2 0x00534a70
void GameMovie::EndFrame()
{
    if (GameMovie::sUseMovies && sRedMovie && sRedMovie->IsOpen() && sRedMovie->IsPlaying())
    {
        sRedMovie->EndFrame();
    }
}

// FUNCTION: SWBF2 0x00534960
void GameMovie::Stop()
{
#ifdef OPENSWBF2_TEST
    if (MovieTestBlocks())
    {
        return;
    }
#endif
    if (GameMovie::sUseMovies && sRedMovie && sRedMovie->IsOpen() && sRedMovie->IsPlaying())
    {
        sRedMovie->Stop(true);
        FLRenderer::EnableCursor(true);
    }
}

// FUNCTION: SWBF2 0x00534730
bool GameMovie::IsOpen()
{
    if (GameMovie::sUseMovies && GameMovie::sRedMovie)
    {
        return GameMovie::sRedMovie->IsOpen();
    }
    return false;
}

extern int g_InGameMoviePlayerFrame;
// GLOBAL: SWBF2 0x01e56280
bool g_InGameMovieSpawnDisplayActive[1];

extern char ingame_movie_idname[256];
extern char g_InGameMovieFilename[256];
extern Snd::Queue<unsigned int, 12> g_InGameMovieQueue;

extern int g_InGameMoviePlayerState;

// FUNCTION: SWBF2 0x00534ab0
void EnableSceneAndInterface(bool bEnable)
{
    for (unsigned int i = 0; i < CameraManager::sInstance->m_iNumCam; ++i)
    {
        unsigned int iView = CameraManager::sInstance->m_hView[i];
        FLRenderer::SetSceneEnable(iView, bEnable);
        FLRenderer::SetInterfaceEnable(iView, bEnable);
    }
    HUD::gScreenGroup->Enable(bEnable);

    SpawnDisplay* pSpawnDisplay = SpawnDisplay::Instance(0);
    if (!pSpawnDisplay)
    {
        return;
    }
    const unsigned int numCameras = CameraManager::sInstance->m_iNumCam;
    for (unsigned int i = 0; i < numCameras; ++i)
    {
        if (bEnable)
        {
            if (g_InGameMovieSpawnDisplayActive[i])
            {
                Character* pCharacter = Character::FindPlayer(i);
                SpawnDisplay::Instance(0)->Show(pCharacter);
                g_InGameMovieSpawnDisplayActive[i] = false;
            }
        }
        else
        {
            SpawnDisplay* pDisplay = SpawnDisplay::Instance(0);
            Character* pCharacter = Character::FindPlayer(i);
            g_InGameMovieSpawnDisplayActive[i] = pDisplay->mMode == SpawnDisplay::eCHANGECLASS;
            pDisplay->Hide(pCharacter);
        }
    }
}

#ifdef OPENSWBF2_TEST
static bool sbMovieTestStarted;

static void MovieTestOpen(const char* pcFile)
{
    sbMovieTestOwner = true;
    if (gMovieLocal)
    {
        const size_t uLen = strlen(pcFile);
        if (GameMovie::sFilename)
        {
            RedFreeToHeap(GameMovie::sDefaultHeap, GameMovie::sFilename, 0);
        }
        GameMovie::sFilename = (char*)RedAllocFromHeap(GameMovie::sDefaultHeap, uLen + 2, 0);
        if (GameMovie::sFilename)
        {
            memcpy(GameMovie::sFilename, pcFile, uLen + 1);
            GameMovie::sFilename[uLen + 1] = 0;
            GameMovie::Open(GameMovie::sFilename, GameMovie::sFilename + uLen + 1, GameMemory::RunTimeHeap,
                            GameMemory::RunTimeHeap);
        }
    }
    else
    {
        char acPath[64];
        strcpy(acPath, "movies\\");
        strcat(acPath, pcFile);
        GameMovie::Open(acPath, "", GameMemory::RunTimeHeap, GameMemory::RunTimeHeap);
    }
    sbMovieTestOwner = false;
}

static void MovieTestStart()
{
    if (sbMovieTestStarted || !GameMovie::sRedMovie || GameMemory::TempLoadHeap)
    {
        return;
    }
    sbMovieTestStarted = true;
    MovieTestOpen(gMovieTestFile);
    LOG(LogCore, Inform, "OPENSWBF2 MOVIETEST play file=%s seg=0x%08X frames=%d open=%d", gMovieTestFile,
        gMovieTestSegment, gMovieTestFrames, GameMovie::sRedMovie->IsOpen() ? 1 : 0);
    if (!GameMovie::sRedMovie->IsOpen() || !GameMovie::sRedMovie->GetInfo()->FindSegmentByHashID(gMovieTestSegment))
    {
        LOG(LogCore, Warning, "OPENSWBF2 MOVIETEST no segment 0x%08X in %s", gMovieTestSegment, gMovieTestFile);
        gRedMovieTestEnd = true;
        return;
    }
    const int iOldHeap = RedSetCurrentHeap(GameMovie::sDefaultHeap);
    RedMovie::Properties* pProperties = new RedMovie::Properties;
    new RedMovie::SegmentInfo(gMovieTestSegment, 0.0f, 0, pProperties->mList);
    RedSetCurrentHeap(iOldHeap);
    FLRenderer::EnableCursor(false);
    GameMovie::sRedMovie->SetDimensions(0, 0, RedInterfaceScreen::GetScreenWidth(), RedInterfaceScreen::GetScreenHeight(),
                                        true);
    GameMovie::sRedMovie->Play(pProperties, false);
    GameMovie::sRedMovie->SetLooping(false);
}

static void MovieLoopsStart()
{
    if (sbMovieTestStarted || !GameMovie::sRedMovie || GameMemory::TempLoadHeap)
    {
        return;
    }
    sbMovieTestStarted = true;
    MovieTestOpen("shell.mvs");
    const RedMovie::Properties* pProperties = RedMovie::Properties::FindByHashID(0xB4A21ED9);
    if (GameMovie::sRedMovie->IsOpen() && pProperties)
    {
        FLRenderer::EnableCursor(false);
        GameMovie::sRedMovie->SetDimensions(0, 0, RedInterfaceScreen::GetScreenWidth(),
                                            RedInterfaceScreen::GetScreenHeight(), true);
        GameMovie::sRedMovie->Play(pProperties, false);
        GameMovie::sRedMovie->SetLooping(true);
    }
    LOG(LogCore, Inform, "OPENSWBF2 MOVIE loops start properties=0xB4A21ED9 found=%d open=%d playing=%d",
        pProperties ? 1 : 0, GameMovie::sRedMovie->IsOpen() ? 1 : 0, GameMovie::sRedMovie->IsPlaying() ? 1 : 0);
}

static void MovieSkipUpdate()
{
    static bool sbSkipped;
    if (sbSkipped || gMovieSkip <= 0 || g_InGameMoviePlayerState != 3 || gRedMovieTestFrameNum < (unsigned int)gMovieSkip)
    {
        return;
    }
    sbSkipped = true;
    LOG(LogCore, Inform, "OPENSWBF2 MOVIE skip frame=%u", gRedMovieTestFrameNum);
    StopInGameMoviePlay();
}

static void MovieTestUpdate()
{
    static bool sbPosted;
    MovieSkipUpdate();
    if (sbPosted || GameState::GetState() != 0x11E1FC01)
    {
        return;
    }
    if (gMovieTestFile[0])
    {
        MovieTestStart();
    }
    else if (gMovieLoops > 0)
    {
        MovieLoopsStart();
    }
    if (gMovieTestFile[0] && (gRedMovieTestEnd || gRedMovieNotifyEnds != 0))
    {
        sbPosted = true;
        LOG(LogCore, Inform, "OPENSWBF2 MOVIETEST done frames=%u", gRedMovieTestFrames);
        GameLoop::PostRunComplete();
    }
    else if (gMovieLoops > 0 && gRedMovieNotifyEnds >= (unsigned int)gMovieLoops)
    {
        sbPosted = true;
        LOG(LogCore, Inform, "OPENSWBF2 MOVIE loops=%d", gMovieLoops);
        GameLoop::PostRunComplete();
    }
}
#endif

// FUNCTION: SWBF2 0x005347a0
void GameMovie::Play(const char* const pcProperties, const char* const pcPropertiesAtEnd,
                     unsigned int uiY, unsigned int uiX, unsigned int uiWidth,
                     unsigned int uiHeight, bool bLoop)
{
    if (GameMovie::sUseMovies && sRedMovie && sRedMovie->IsOpen())
    {
#ifdef OPENSWBF2_TEST
        if (MovieTestBlocks())
        {
            return;
        }
#endif
        const RedMovie::Properties* pProperties =
            RedMovie::Properties::FindByHashID(PblHash(pcProperties));
        const RedMovie::Properties* pPropertiesAtEnd =
            (pcPropertiesAtEnd && strlen(pcPropertiesAtEnd))
                ? RedMovie::Properties::FindByHashID(PblHash(pcPropertiesAtEnd))
                : 0;

        if (pProperties)
        {
            FLRenderer::EnableCursor(false);
            sRedMovie->SetDimensions(uiX, uiY, uiWidth, uiHeight, true);
            sRedMovie->Play(pProperties, false);
            sRedMovie->SetLooping(bLoop);
            if (pPropertiesAtEnd)
            {
                sRedMovie->Play(pPropertiesAtEnd, true);
            }
        }
    }
}

// FUNCTION: SWBF2 0x00534880
bool GameMovie::IsPlaying()
{
    if (g_InGameMoviePlayerState == 1 || g_InGameMoviePlayerState == 2)
    {
        return true;
    }

    if (GameMovie::sUseMovies && sRedMovie && sRedMovie->IsOpen())
    {
        return sRedMovie->IsPlaying();
    }

    return false;
}

// FUNCTION: SWBF2 0x005348c0
bool GameMovie::PropertiesPlaying(const char* const pcProperties, bool bCurrent)
{
    if (!GameMovie::sUseMovies || !sRedMovie || !sRedMovie->IsOpen())
    {
        return false;
    }

    if (bCurrent)
    {
        const RedMovie::Properties* pCurrent = sRedMovie->GetProperties();
        return pCurrent ? pCurrent->GetHashID() == PblHash(pcProperties) : false;
    }

    const RedMovie::Properties* pProperties =
        RedMovie::Properties::FindByHashID(PblHash(pcProperties));
    if (pProperties)
    {
        return sRedMovie->GetPropertiesScheduled(pProperties);
    }
    return false;
}

// FUNCTION: SWBF2 0x005349b0
void GameMovie::SetAudioBus(const char* const pcBusName)
{
    if (GameMovie::sUseMovies && sRedMovie)
    {
        sRedMovie->SetAudioBus(PblHash(pcBusName));
    }
}

// FUNCTION: SWBF2 0x005349f0
void GameMovie::Update(float fDeltaTime)
{
#ifdef OPENSWBF2_TEST
    MovieTestUpdate();
#endif
    if (GameMovie::sUseMovies && sRedMovie && sRedMovie->IsOpen())
    {
        sRedMovie->Update(fDeltaTime);
    }
}

// FUNCTION: SWBF2 0x00534bd0
void StartInGameMoviePlay(const char* pcFile, const char* pcName)
{
    if (g_InGameMoviePlayerState)
    {
        return;
    }

    g_InGameMoviePlayerUpdateEnable = true;
    g_InGameMoviePlayerState = 1;
    const unsigned int uiView = CameraManager::sInstance->m_hView[0];
    g_InGameMoviePlayerFrame = 0;
    FLRenderer::SetSceneEnable(uiView, false);
    FLRenderer::SetInterfaceEnable(uiView, false);
    strcpy(g_InGameMovieFilename, "Movies\\");
    strcat(g_InGameMovieFilename, pcFile);
    strcpy(ingame_movie_idname, pcName);
    unsigned __int64 perfCount = PblTime::GetTickCount();
    g_InGameMovieStartTime = perfCount;
    g_InGameMovieStartSystemTime = GameState::GetSystemTimeBase()->GetTime();
}

// FUNCTION: SWBF2 0x00534ca0
void StopInGameMoviePlay()
{
    if (g_InGameMoviePlayerState)
    {
        const float fElapsed = (float)(PblTime::GetTickCount() - g_InGameMovieStartTime);
        const float fSeconds = g_fFixedRate > 0.0f
            ? GameState::GetSystemTimeBase()->GetTime() - g_InGameMovieStartSystemTime
            : fElapsed / (float)PblTime::ticksPerSecond;
        if (fSeconds > g_MovieStopDelayTime)
        {
            g_InGameMoviePlayerState = 0;
            g_InGameMoviePlayerFrame = 0;
            g_InGameMoviePlayerUpdateEnable = false;
            EnableSceneAndInterface(true);
            GameMovie::Stop();
            GameMovie::Close();
            GameMovie::DestroyKeepProperty();
        }
    }
}

// GLOBAL: SWBF2 0x01e5616c
int g_InGameMoviePlayerState;

// GLOBAL: SWBF2 0x01e56270
int g_InGameMoviePlayerFrame;

// SYNTHETIC: SWBF2 0x00403da0
// `dynamic initializer for 'g_InGameMovieQueue''
// SYNTHETIC: SWBF2 0x00768a80
// `dynamic atexit destructor for 'g_InGameMovieQueue''

// GLOBAL: SWBF2 0x01ec0c44
// g_InGameMovieQueue
Snd::Queue<unsigned int, 12> g_InGameMovieQueue;
