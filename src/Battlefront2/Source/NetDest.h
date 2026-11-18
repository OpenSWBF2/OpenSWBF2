#pragma once

#include "NetComm.h"

struct NetDest
{
    bool connected;
    NetPktGroup outQueue;

    NetSocketPeerRecord mAddress;
    NetArch arch;
    float lastRecv;
    PipeType pipe;
    int ping;
    int PLKills;
    int PLDeaths;
    int PLCPs;
    int upBandwidth;
    int downBandwidth;
    int nextGroupNum;

    void Init()
    {
        ping = 250;
        PLDeaths = 0;
        PLKills = 0;
        upBandwidth = 0x10000;
        downBandwidth = 0x10000;
        pipe = PIPE_INTERNET;
        nextGroupNum = 0;
    }
    void Clean()
    {
        connected = false;
        lastRecv = 0.0f;
        ping = 250;
        PLDeaths = 0;
        PLKills = 0;
        upBandwidth = 0x10000;
        downBandwidth = 0x10000;
        pipe = PIPE_INTERNET;
        nextGroupNum = 0;
    }
};

extern NetDest netDests[MAX_DESTS];
