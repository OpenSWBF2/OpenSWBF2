#ifdef OPENSWBF2_RIDERLINK

#define _CRT_RAND_S
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "RiderLink.h"
#include "RiderLinkProtocol.h"
#include "FLInputManager.h"
#include "GameLoop.h"
#include "GameState.h"
#include "HUDEditor.h"
#include "HUDManager.h"
#include "LoadUtil.h"
#include "NetGame.h"
#include "NetSetup.h"
#include "PauseMenu.h"
#include "PopupText.h"
#include "RedEngineFL/Util/RedLog.h"
#include "RedEngineFL/Util/RedLogTap.h"

using namespace RiderLinkProtocol;

namespace RiderLink
{

namespace
{

const unsigned int kMissionState = 0x8FF60339;
const unsigned int kSendCap = 8192;
const unsigned int kDirCap = MAX_PATH - 40;

bool sSwitch;
bool sHandsOff;
bool sHoldArg;
bool sPauseArg;
bool sDirTooLong;
char sacPublishDir[MAX_PATH];

bool sStarted;
bool sHelloHold;
bool sPauseEachMission;
bool sHold;
bool sInMission;
unsigned int suStepsPending;
unsigned int suSimFrames;
SOCKET sListen;
SOCKET sClient;
bool sClientOpen;
bool sAuthed;
HANDLE shPortFile;
CRITICAL_SECTION sLock;
Ring sRing;
unsigned char sRingData[kRingSize];
TapBuilder sTap;
LineAssembler sAsm;
WarnBudget sBudget;
char sacToken[kTokenLen + 1];
char sacSend[kSendCap];
unsigned int suSendLen;
unsigned int suSendPos;
char const* spcSentState;

LinkState CurrentLinkState()
{
    if (GameState::GetState() != kMissionState)
    {
        return StateIdle;
    }
    return GameLoop::sPauseMode || sHold || sHelloHold ? StatePause : StatePlay;
}

char const* CurrentState()
{
    return StateWord(CurrentLinkState());
}

bool GameHoldsPause()
{
    return PauseMenu::GetPaused(0) || PopupText::IsEnabled() ||
           (gEditor && gEditor->mMode >= HUD::Editor::mode_SelectElement);
}

bool MakeToken()
{
    unsigned int auWords[4];
    for (int i = 0; i < 4; ++i)
    {
        if (rand_s(&auWords[i]) != 0)
        {
            return false;
        }
    }
    snprintf(sacToken, kTokenLen + 1, "%08x%08x%08x%08x", auWords[0], auWords[1], auWords[2], auWords[3]);
    return strlen(sacToken) == kTokenLen;
}

bool PublishDir(wchar_t* pwcDir)
{
    if (sacPublishDir[0])
    {
        if (MultiByteToWideChar(CP_ACP, 0, sacPublishDir, -1, pwcDir, (int)kDirCap) == 0)
        {
            return false;
        }
        CreateDirectoryW(pwcDir, 0);
    }
    else
    {
        wchar_t awcLocal[MAX_PATH];
        const DWORD uLen = GetEnvironmentVariableW(L"LOCALAPPDATA", awcLocal, kDirCap);
        if (uLen == 0 || uLen >= kDirCap)
        {
            return false;
        }
        wsprintfW(pwcDir, L"%s\\OpenSWBF2", awcLocal);
        CreateDirectoryW(pwcDir, 0);
        lstrcatW(pwcDir, L"\\RiderLink");
        CreateDirectoryW(pwcDir, 0);
    }
    const DWORD uAttr = GetFileAttributesW(pwcDir);
    return uAttr != INVALID_FILE_ATTRIBUTES && (uAttr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool PublishPort(unsigned int uPort)
{
    wchar_t awcDir[MAX_PATH];
    if (sDirTooLong || !PublishDir(awcDir))
    {
        return false;
    }
    wchar_t awcPath[MAX_PATH];
    wsprintfW(awcPath, L"%s\\%lu.port", awcDir, GetCurrentProcessId());
    HANDLE hFile = CreateFileW(awcPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, 0, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    char acLine[64];
    const int iLen = snprintf(acLine, sizeof(acLine), "%u %s\n", uPort, sacToken);
    DWORD uWritten = 0;
    if (iLen <= 0 || !WriteFile(hFile, acLine, (DWORD)iLen, &uWritten, 0) || uWritten != (DWORD)iLen)
    {
        CloseHandle(hFile);
        return false;
    }
    shPortFile = hFile;
    return true;
}

void QueueLine(char const* pcPrefix, char const* pcBody)
{
    EnterCriticalSection(&sLock);
    RingPush(sRing, pcPrefix, pcBody, (unsigned int)strlen(pcBody));
    LeaveCriticalSection(&sLock);
}

void CloseClient()
{
    EnterCriticalSection(&sLock);
    sAuthed = false;
    RingClear(sRing);
    ResetTap(sTap);
    LeaveCriticalSection(&sLock);
    closesocket(sClient);
    sClientOpen = false;
    suSendLen = 0;
    suSendPos = 0;
    const unsigned int uSuppressed = sBudget.uSuppressed;
    ResetBudget(sBudget);
    if (uSuppressed > 0)
    {
        LOG(LogNet, Warning, "RiderLink suppressed %u more bad inputs from the client", uSuppressed);
    }
}

void Authenticate()
{
    char acHello[96];
    snprintf(acHello, sizeof(acHello), "HELLO %u %lu %s %s", kVersion, GetCurrentProcessId(), PlatformWord(),
             ConfigWord());
    char const* pcState = CurrentState();
    EnterCriticalSection(&sLock);
    RingClear(sRing);
    ResetTap(sTap);
    RingPush(sRing, "", acHello, (unsigned int)strlen(acHello));
    RingPush(sRing, "STATE ", pcState, (unsigned int)strlen(pcState));
    sAuthed = true;
    LeaveCriticalSection(&sLock);
    spcSentState = pcState;
    suSendLen = 0;
    suSendPos = 0;
    if (sHelloHold)
    {
        sHelloHold = false;
        if (GameState::GetState() == kMissionState && !sHold)
        {
            GameLoop::sPauseMode = false;
        }
    }
    LOG(LogNet, Inform, "RiderLink client connected");
}

void Refuse(Message eCommand, Refusal eRefusal)
{
    if (SpendWarning(sBudget))
    {
        LOG(LogNet, Warning, "RiderLink refused %s: %s", CommandWord(eCommand), RefusalText(eRefusal));
    }
}

void Play(char const* pcMission)
{
    if (!LoadUtil::HasMission(pcMission))
    {
        if (SpendWarning(sBudget))
        {
            LOG(LogNet, Warning, "RiderLink: no mission named %s", pcMission);
        }
        return;
    }
    LOG(LogNet, Inform, "RiderLink play %s", pcMission);
    GameLoop::SetNextMission(pcMission);
    FLInputManager::Instance()->SetAutoAcquire(true);
    GameState::SetState(kMissionState);
}

void Command(Message eCommand, char const* pcMission)
{
    const Refusal eRefusal = Check(eCommand, CurrentLinkState(), NetGame::IsEnabled(), GameHoldsPause());
    if (eRefusal != RefuseNone)
    {
        Refuse(eCommand, eRefusal);
        return;
    }
    switch (eCommand)
    {
    case MsgPause:
        sHold = true;
        suStepsPending = 0;
        GameLoop::sPauseMode = true;
        LOG(LogNet, Inform, "RiderLink pause sim_frames=%u", suSimFrames);
        break;
    case MsgStep:
        sHold = true;
        ++suStepsPending;
        break;
    case MsgResume:
        sHold = false;
        suStepsPending = 0;
        GameLoop::sPauseMode = false;
        LOG(LogNet, Inform, "RiderLink resume sim_frames=%u", suSimFrames);
        break;
    case MsgStop:
        sHold = false;
        suStepsPending = 0;
        LOG(LogNet, Inform, "RiderLink stop sim_frames=%u", suSimFrames);
        NetSetup::RequestQuit(0);
        break;
    case MsgPlay:
        Play(pcMission);
        break;
    default:
        break;
    }
}

void ApplyHold(bool bMission)
{
    if (!bMission)
    {
        return;
    }
    if (sHold && suStepsPending > 0)
    {
        --suStepsPending;
        GameLoop::sPauseMode = false;
        LOG(LogNet, Inform, "RiderLink step sim_frames=%u", suSimFrames + 1);
    }
    else if (sHold || sHelloHold)
    {
        GameLoop::sPauseMode = true;
    }
    if (!GameLoop::sPauseMode)
    {
        ++suSimFrames;
    }
}

void BadInput(char const* pcWhat)
{
    if (SpendWarning(sBudget))
    {
        LOG(LogNet, Warning, "RiderLink ignored %s from the client", pcWhat);
    }
}

void OnLine(char const* pcLine, unsigned int uLen)
{
    char acToken[kTokenLen + 1];
    char acMission[kMissionCap];
    unsigned int uVersion = 0;
    const Message eMsg = Parse(pcLine, uLen, acToken, uVersion, acMission);
    if (eMsg == MsgEmpty)
    {
        return;
    }
    if (sAuthed)
    {
        if (eMsg >= MsgPlay && eMsg <= MsgStop)
        {
            Command(eMsg, acMission);
            return;
        }
        BadInput(eMsg == MsgUnknown ? "an unknown message"
                                    : (eMsg == MsgBadCommand ? "a malformed command" : "a line that is not valid now"));
        return;
    }
    if (eMsg == MsgHello && TokenEqual(acToken, sacToken))
    {
        Authenticate();
        return;
    }
    CloseClient();
    if (eMsg == MsgHelloVersion)
    {
        LOG(LogNet, Warning, "RiderLink closed a client that speaks protocol %u, this game speaks %u", uVersion,
            kVersion);
        return;
    }
    LOG(LogNet, Warning, "RiderLink closed a client that sent %s",
        eMsg == MsgHello ? "a wrong token" : (eMsg == MsgBadHello ? "a bad HELLO" : "a line that is not HELLO"));
}

void Accept()
{
    for (;;)
    {
        const SOCKET hNew = accept(sListen, 0, 0);
        if (hNew == INVALID_SOCKET)
        {
            return;
        }
        if (sClientOpen && sAuthed)
        {
            closesocket(hNew);
            LOG(LogNet, Warning, "RiderLink refused a second client");
            continue;
        }
        if (sClientOpen)
        {
            CloseClient();
            LOG(LogNet, Warning, "RiderLink replaced a client that had not sent HELLO");
        }
        u_long uOn = 1;
        ioctlsocket(hNew, FIONBIO, &uOn);
        sClient = hNew;
        sClientOpen = true;
        ResetAssembler(sAsm);
        ResetBudget(sBudget);
    }
}

void Receive()
{
    char acBuf[1024];
    while (sClientOpen)
    {
        const int iGot = recv(sClient, acBuf, (int)sizeof(acBuf), 0);
        if (iGot == 0)
        {
            CloseClient();
            LOG(LogNet, Inform, "RiderLink client disconnected");
            return;
        }
        if (iGot < 0)
        {
            const int iError = WSAGetLastError();
            if (iError != WSAEWOULDBLOCK)
            {
                CloseClient();
                LOG(LogNet, Inform, "RiderLink client dropped (%d)", iError);
            }
            return;
        }
        unsigned int uPos = 0;
        while (sClientOpen)
        {
            const TakeResult eResult = Take(sAsm, acBuf, (unsigned int)iGot, uPos);
            if (eResult == TakeNeedMore)
            {
                break;
            }
            if (eResult == TakeLine)
            {
                OnLine(sAsm.acLine, sAsm.uLen);
            }
            else if (sAuthed)
            {
                BadInput("an oversized line");
            }
            else
            {
                CloseClient();
                LOG(LogNet, Warning, "RiderLink closed a client that sent an oversized line");
            }
        }
    }
}

void QueueState()
{
    if (!sAuthed)
    {
        return;
    }
    char const* pcState = CurrentState();
    if (spcSentState && strcmp(pcState, spcSentState) == 0)
    {
        return;
    }
    QueueLine("STATE ", pcState);
    spcSentState = pcState;
}

void Flush()
{
    while (sClientOpen && sAuthed)
    {
        if (suSendPos == suSendLen)
        {
            EnterCriticalSection(&sLock);
            suSendLen = RingRead(sRing, sacSend, kSendCap, spcSentState ? spcSentState : "Idle");
            LeaveCriticalSection(&sLock);
            suSendPos = 0;
            if (suSendLen == 0)
            {
                return;
            }
        }
        const int iSent = send(sClient, &sacSend[suSendPos], (int)(suSendLen - suSendPos), 0);
        if (iSent < 0)
        {
            const int iError = WSAGetLastError();
            if (iError != WSAEWOULDBLOCK)
            {
                CloseClient();
                LOG(LogNet, Inform, "RiderLink client dropped (%d)", iError);
            }
            return;
        }
        suSendPos += (unsigned int)iSent;
    }
}

}

void NoteSwitch()
{
    sSwitch = true;
}

void NoteHandsOff()
{
    sHandsOff = true;
}

void NoteHold(bool bEveryMission)
{
    if (bEveryMission)
    {
        sPauseArg = true;
    }
    else
    {
        sHoldArg = true;
    }
}

void SetPublishDir(char const* pcDir)
{
    const size_t uLen = strlen(pcDir);
    sDirTooLong = uLen >= kDirCap;
    if (sDirTooLong)
    {
        sacPublishDir[0] = 0;
        return;
    }
    memcpy(sacPublishDir, pcDir, uLen + 1);
}

void __cdecl Tap(char const* pcText)
{
    EnterCriticalSection(&sLock);
    if (sAuthed)
    {
        RiderLinkProtocol::Tap(sTap, sRing, pcText, (unsigned int)GetCurrentThreadId());
    }
    LeaveCriticalSection(&sLock);
}

void Start(bool bNoRender)
{
    if (sStarted || !ShouldStart(bNoRender, sHandsOff, sSwitch))
    {
        return;
    }
    WSADATA kWsa;
    if (WSAStartup(0x0202, &kWsa) != 0)
    {
        LOG(LogNet, Warning, "RiderLink could not start Winsock, the link is off");
        return;
    }
    if (!MakeToken())
    {
        LOG(LogNet, Warning, "RiderLink could not make a token, the link is off");
        return;
    }
    const SOCKET hListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hListen == INVALID_SOCKET)
    {
        LOG(LogNet, Warning, "RiderLink could not open a socket, the link is off");
        return;
    }
    BOOL bExclusive = TRUE;
    sockaddr_in kAddr;
    memset(&kAddr, 0, sizeof(kAddr));
    kAddr.sin_family = AF_INET;
    kAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    kAddr.sin_port = 0;
    int iAddrLen = (int)sizeof(kAddr);
    u_long uOn = 1;
    if (setsockopt(hListen, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char const*)&bExclusive, (int)sizeof(bExclusive)) != 0 ||
        bind(hListen, (sockaddr*)&kAddr, iAddrLen) != 0 || listen(hListen, 1) != 0 ||
        getsockname(hListen, (sockaddr*)&kAddr, &iAddrLen) != 0 || ioctlsocket(hListen, FIONBIO, &uOn) != 0)
    {
        LOG(LogNet, Warning, "RiderLink could not listen on 127.0.0.1 (%d), the link is off", WSAGetLastError());
        closesocket(hListen);
        return;
    }
    const unsigned int uPort = ntohs(kAddr.sin_port);
    LOG(LogNet, Inform, "RiderLink listening 127.0.0.1:%u", uPort);
    if (!PublishPort(uPort))
    {
        LOG(LogNet, Warning, "RiderLink could not write its port file, the link is off");
        closesocket(hListen);
        return;
    }
    InitializeCriticalSection(&sLock);
    RingInit(sRing, sRingData, kRingSize);
    ResetTap(sTap);
    ResetAssembler(sAsm);
    ResetBudget(sBudget);
    sListen = hListen;
    sHelloHold = sHoldArg && sSwitch;
    sPauseEachMission = sPauseArg && sSwitch;
    sStarted = true;
    RedWarning::g_fnLineTap = Tap;
}

void Service()
{
    if (!sStarted)
    {
        return;
    }
    const bool bMission = GameState::GetState() == kMissionState;
    if (!bMission)
    {
        sHold = false;
        sInMission = false;
        suStepsPending = 0;
        suSimFrames = 0;
    }
    else if (!sInMission)
    {
        sInMission = true;
        sHold = sPauseEachMission;
    }
    if (bMission && (sHold || sHelloHold))
    {
        GameLoop::sPauseMode = true;
    }
    Accept();
    Receive();
    ApplyHold(bMission);
    QueueState();
    Flush();
}

void Stop()
{
    if (!sStarted)
    {
        return;
    }
    RedWarning::g_fnLineTap = 0;
    Flush();
    if (sClientOpen)
    {
        char acBuf[1024];
        while (recv(sClient, acBuf, (int)sizeof(acBuf), 0) > 0)
        {
        }
        shutdown(sClient, SD_SEND);
        closesocket(sClient);
        sClientOpen = false;
    }
    EnterCriticalSection(&sLock);
    sAuthed = false;
    LeaveCriticalSection(&sLock);
    closesocket(sListen);
    CloseHandle(shPortFile);
    shPortFile = 0;
    sStarted = false;
}

}

#endif
