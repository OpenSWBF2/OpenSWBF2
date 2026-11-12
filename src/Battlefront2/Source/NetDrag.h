#pragma once

class NetPacket;

class NetDrag
{
public:
    static void Create(bool bMission);
    static void Reset();
    static void BeginShell();
    static void EndShell();
    static void Update();
    static void Send(NetPacket* pPacket, int iDest);
    static NetPacket* ReceiveLocal();
    static void FreePackets();
};
