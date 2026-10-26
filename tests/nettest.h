#pragma once
#include "Battlefront2/Source/NetComm.h"

extern int gExitCalls;
extern int gLastExitCode;

void NetTestReset();

int NetCountPackets(int iHeld);

NetPacket* NetMakePacket(const unsigned char* pPayload, int iLen);

void NetStampAndQueue(NetPacket* pPacket, unsigned short uGroup, unsigned char uType, unsigned char uSrc,
                      unsigned char uIdx, unsigned char uCount, unsigned char uCompressed);

void NetDeliverDatagram(const unsigned char* pData, unsigned int uiSize, unsigned int uiPrefix);

bool __fastcall RecvGroup(NetPktGroup* pGroup, NetPacket* pPacket);
