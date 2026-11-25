#include "game/riderlink_client.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{

unsigned int sNextFolder;

bool ReadWholeFile(const std::string& sPath, std::string& sOut)
{
    HANDLE hFile = CreateFileA(sPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    char acBuf[256];
    DWORD uRead = 0;
    const BOOL bOk = ReadFile(hFile, acBuf, sizeof(acBuf), &uRead, 0);
    CloseHandle(hFile);
    if (!bOk)
    {
        return false;
    }
    sOut.assign(acBuf, uRead);
    return true;
}

}

RiderLinkClient::RiderLinkClient()
    : mPid(0), mPort(0), mProcess(0), mEvent(0), mSocket(INVALID_SOCKET), mConnected(false), mWsa(false)
{
    WSADATA kWsa;
    mWsa = WSAStartup(0x0202, &kWsa) == 0;
    char acLocal[MAX_PATH];
    const DWORD uLen = GetEnvironmentVariableA("LOCALAPPDATA", acLocal, MAX_PATH);
    std::string sRoot = (uLen > 0 && uLen < MAX_PATH) ? std::string(acLocal) : std::string(".");
    sRoot += "\\OpenSWBF2";
    CreateDirectoryA(sRoot.c_str(), 0);
    sRoot += "\\RiderLinkTest";
    CreateDirectoryA(sRoot.c_str(), 0);
    char acLeaf[48];
    _snprintf(acLeaf, sizeof(acLeaf) - 1, "\\%lu.%u", GetCurrentProcessId(), sNextFolder++);
    acLeaf[sizeof(acLeaf) - 1] = 0;
    mFolder = sRoot + acLeaf;
    CreateDirectoryA(mFolder.c_str(), 0);
}

RiderLinkClient::~RiderLinkClient()
{
    Disconnect();
    if (mProcess)
    {
        CloseHandle(mProcess);
    }
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((mFolder + "\\*.port").c_str(), &kFind);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            DeleteFileA((mFolder + "\\" + kFind.cFileName).c_str());
        } while (FindNextFileA(hFind, &kFind));
        FindClose(hFind);
    }
    RemoveDirectoryA(mFolder.c_str());
    if (mWsa)
    {
        WSACleanup();
    }
}

const std::string& RiderLinkClient::Folder() const
{
    return mFolder;
}

std::string RiderLinkClient::Args() const
{
    return "-riderlink -riderlinkdir \"" + mFolder + "\"";
}

void RiderLinkClient::FindChild()
{
    if (mProcess)
    {
        return;
    }
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        return;
    }
    const DWORD uSelf = GetCurrentProcessId();
    PROCESSENTRY32 kEntry;
    kEntry.dwSize = sizeof(kEntry);
    for (BOOL bMore = Process32First(hSnap, &kEntry); bMore && !mProcess; bMore = Process32Next(hSnap, &kEntry))
    {
        if (kEntry.th32ParentProcessID == uSelf && _stricmp(kEntry.szExeFile, "BattlefrontII.exe") == 0)
        {
            mProcess = OpenProcess(SYNCHRONIZE, FALSE, kEntry.th32ProcessID);
            if (mProcess)
            {
                mPid = kEntry.th32ProcessID;
            }
        }
    }
    CloseHandle(hSnap);
}

bool RiderLinkClient::ReadPortFile()
{
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((mFolder + "\\*.port").c_str(), &kFind);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    const std::string sName = kFind.cFileName;
    FindClose(hFind);
    const unsigned long uPid = strtoul(sName.c_str(), 0, 10);
    if (mPid != 0 && uPid != mPid)
    {
        return false;
    }
    std::string sText;
    if (!ReadWholeFile(mFolder + "\\" + sName, sText) || sText.empty() || sText[sText.size() - 1] != '\n')
    {
        return false;
    }
    const size_t uSpace = sText.find(' ');
    if (uSpace == std::string::npos)
    {
        return false;
    }
    mPortText = sText;
    mPort = (unsigned int)strtoul(sText.c_str(), 0, 10);
    mToken = sText.substr(uSpace + 1, sText.size() - uSpace - 2);
    if (mPid == 0)
    {
        mPid = uPid;
        mProcess = OpenProcess(SYNCHRONIZE, FALSE, uPid);
    }
    return true;
}

bool RiderLinkClient::WaitForPortFile(const std::string& sRunDir)
{
    const DWORD uFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE;
    HANDLE ahChange[2];
    ahChange[0] = FindFirstChangeNotificationA(mFolder.c_str(), FALSE, uFilter);
    ahChange[1] = FindFirstChangeNotificationA(sRunDir.c_str(), FALSE, uFilter);
    if (ahChange[0] == INVALID_HANDLE_VALUE || ahChange[1] == INVALID_HANDLE_VALUE)
    {
        printf("riderlink client: cannot watch %s or %s\n", mFolder.c_str(), sRunDir.c_str());
        return false;
    }
    bool bFound = false;
    for (;;)
    {
        FindChild();
        if (ReadPortFile())
        {
            bFound = true;
            break;
        }
        HANDLE ahWait[3] = { ahChange[0], ahChange[1], mProcess };
        const DWORD uCount = mProcess ? 3 : 2;
        const DWORD uWoke = WaitForMultipleObjects(uCount, ahWait, FALSE, INFINITE);
        if (uWoke == WAIT_OBJECT_0 + 2)
        {
            printf("riderlink client: the game ended before it published a port file\n");
            break;
        }
        if (uWoke == WAIT_OBJECT_0 || uWoke == WAIT_OBJECT_0 + 1)
        {
            FindNextChangeNotification(ahChange[uWoke - WAIT_OBJECT_0]);
        }
        else
        {
            printf("riderlink client: wait failed (%lu)\n", GetLastError());
            break;
        }
    }
    FindCloseChangeNotification(ahChange[0]);
    FindCloseChangeNotification(ahChange[1]);
    return bFound;
}

bool RiderLinkClient::PortFileExists() const
{
    WIN32_FIND_DATAA kFind;
    HANDLE hFind = FindFirstFileA((mFolder + "\\*.port").c_str(), &kFind);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    FindClose(hFind);
    return true;
}

const std::string& RiderLinkClient::PortFileText() const
{
    return mPortText;
}

unsigned long RiderLinkClient::Pid() const
{
    return mPid;
}

unsigned int RiderLinkClient::Port() const
{
    return mPort;
}

const std::string& RiderLinkClient::Token() const
{
    return mToken;
}

std::vector<unsigned long> RiderLinkClient::LocalAddresses()
{
    std::vector<unsigned long> kOut;
    char acHost[256];
    if (gethostname(acHost, sizeof(acHost)) != 0)
    {
        return kOut;
    }
    addrinfo kHints;
    memset(&kHints, 0, sizeof(kHints));
    kHints.ai_family = AF_INET;
    kHints.ai_socktype = SOCK_STREAM;
    addrinfo* pList = 0;
    if (getaddrinfo(acHost, 0, &kHints, &pList) != 0)
    {
        return kOut;
    }
    for (addrinfo* pAt = pList; pAt; pAt = pAt->ai_next)
    {
        const sockaddr_in* pAddr = (const sockaddr_in*)pAt->ai_addr;
        const unsigned long uAddr = ntohl(pAddr->sin_addr.s_addr);
        if ((uAddr >> 24) != 127)
        {
            kOut.push_back(uAddr);
        }
    }
    freeaddrinfo(pList);
    return kOut;
}

int RiderLinkClient::ConnectTo(unsigned long uAddr)
{
    SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hSocket == INVALID_SOCKET)
    {
        return WSAGetLastError();
    }
    sockaddr_in kAddr;
    memset(&kAddr, 0, sizeof(kAddr));
    kAddr.sin_family = AF_INET;
    kAddr.sin_addr.s_addr = htonl(uAddr);
    kAddr.sin_port = htons((unsigned short)mPort);
    const int iResult = connect(hSocket, (sockaddr*)&kAddr, (int)sizeof(kAddr));
    const int iError = iResult == 0 ? 0 : WSAGetLastError();
    closesocket(hSocket);
    return iError;
}

bool RiderLinkClient::Connect()
{
    Disconnect();
    SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hSocket == INVALID_SOCKET)
    {
        return false;
    }
    sockaddr_in kAddr;
    memset(&kAddr, 0, sizeof(kAddr));
    kAddr.sin_family = AF_INET;
    kAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    kAddr.sin_port = htons((unsigned short)mPort);
    if (connect(hSocket, (sockaddr*)&kAddr, (int)sizeof(kAddr)) != 0)
    {
        printf("riderlink client: connect failed (%d)\n", WSAGetLastError());
        closesocket(hSocket);
        return false;
    }
    WSAEVENT hEvent = WSACreateEvent();
    if (hEvent == WSA_INVALID_EVENT || WSAEventSelect(hSocket, hEvent, FD_READ | FD_CLOSE) != 0)
    {
        if (hEvent != WSA_INVALID_EVENT)
        {
            WSACloseEvent(hEvent);
        }
        closesocket(hSocket);
        return false;
    }
    mSocket = hSocket;
    mEvent = hEvent;
    mConnected = true;
    mBuffer.clear();
    return true;
}

void RiderLinkClient::Disconnect()
{
    if (mSocket != INVALID_SOCKET)
    {
        closesocket(mSocket);
        mSocket = INVALID_SOCKET;
    }
    if (mEvent)
    {
        WSACloseEvent(mEvent);
        mEvent = 0;
    }
    mConnected = false;
}

bool RiderLinkClient::SendBytes(const std::string& sBytes)
{
    size_t uSent = 0;
    while (mConnected && uSent < sBytes.size())
    {
        const int iSent = send(mSocket, sBytes.data() + uSent, (int)(sBytes.size() - uSent), 0);
        if (iSent > 0)
        {
            uSent += (size_t)iSent;
            continue;
        }
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            return false;
        }
        WSANETWORKEVENTS kEvents;
        WSAEventSelect(mSocket, mEvent, FD_READ | FD_CLOSE | FD_WRITE);
        HANDLE ahWait[2] = { mEvent, mProcess };
        const DWORD uWoke = WaitForMultipleObjects(mProcess ? 2 : 1, ahWait, FALSE, INFINITE);
        WSAEnumNetworkEvents(mSocket, mEvent, &kEvents);
        WSAEventSelect(mSocket, mEvent, FD_READ | FD_CLOSE);
        if (uWoke != WAIT_OBJECT_0)
        {
            return false;
        }
    }
    return uSent == sBytes.size();
}

bool RiderLinkClient::SendLine(const std::string& sLine)
{
    return SendBytes(sLine + "\n");
}

RiderLinkClient::Wake RiderLinkClient::Pump()
{
    for (;;)
    {
        char acBuf[4096];
        const int iGot = recv(mSocket, acBuf, (int)sizeof(acBuf), 0);
        if (iGot > 0)
        {
            mBuffer.append(acBuf, (size_t)iGot);
            return WakeData;
        }
        if (iGot == 0 || WSAGetLastError() != WSAEWOULDBLOCK)
        {
            Disconnect();
            return WakeClosed;
        }
        HANDLE ahWait[2] = { mEvent, mProcess };
        const DWORD uWoke = WaitForMultipleObjects(mProcess ? 2 : 1, ahWait, FALSE, INFINITE);
        if (uWoke == WAIT_OBJECT_0)
        {
            WSANETWORKEVENTS kEvents;
            WSAEnumNetworkEvents(mSocket, mEvent, &kEvents);
            continue;
        }
        if (uWoke == WAIT_OBJECT_0 + 1)
        {
            const int iLast = recv(mSocket, acBuf, (int)sizeof(acBuf), 0);
            if (iLast > 0)
            {
                mBuffer.append(acBuf, (size_t)iLast);
                return WakeData;
            }
            if (iLast == 0)
            {
                Disconnect();
                return WakeClosed;
            }
        }
        return WakeEnded;
    }
}

RiderLinkClient::Wake RiderLinkClient::ReadLineUntil(const std::function<bool(const std::string&)>& fnWant,
                                                     std::string& sLine)
{
    for (;;)
    {
        size_t uEnd;
        while ((uEnd = mBuffer.find('\n')) != std::string::npos)
        {
            sLine = mBuffer.substr(0, uEnd);
            mBuffer.erase(0, uEnd + 1);
            if (fnWant(sLine))
            {
                return WakeData;
            }
        }
        if (!mConnected)
        {
            return WakeClosed;
        }
        const Wake eWake = Pump();
        if (eWake != WakeData)
        {
            return eWake;
        }
    }
}

RiderLinkClient::Wake RiderLinkClient::ReadUntilClosed(std::string& sAll)
{
    for (;;)
    {
        sAll += mBuffer;
        mBuffer.clear();
        if (!mConnected)
        {
            return WakeClosed;
        }
        const Wake eWake = Pump();
        if (eWake != WakeData)
        {
            sAll += mBuffer;
            mBuffer.clear();
            return eWake;
        }
    }
}
