#pragma once

#include <stddef.h>


 

void ParseCommandLine(char const* pcAppName, char const* pcCmdLine);

void ParseCommandLine(int iArgc, char** const apcArgv);

bool FindCommandLineArg(char const* pcLine, char const* pcName, char const** ppcVal, int* piLen);

bool FindNextCommandLineArg(char const* pcLine, char const* pcName, int* piPos, char const** ppcVal, int* piLen,
                            bool bKeepSwitch = false);

int CommandLineRepeatCount(char const* pcVal, int iLen);


bool NoVictoryActive();

bool TestOnlyArgIgnored(int i, char const** ppcName);

extern char gGamespyAutoName[32];
extern char gGamespyAutoConnectAddr[64];
extern char gGamespyAutoPassStr[128];
extern char gGamespyAutoEmail[64];
extern char gGamespyAutoPPass[32];
extern unsigned int gGamespyAutoPass;
extern bool gBootLagger;
extern bool gShowLoadSpam;
extern bool gLogFinalScore;
extern bool gExitOnGameOver;
extern bool gNoVictory;
extern int gDigestTrace;
extern bool gFactoryLog;
extern bool gNoFocusPause;
extern bool gAllowUploads;
extern bool gSeedSet;
extern unsigned int gSeed;
extern int gBootLaggerPing;

namespace PlayerPrefs
{
extern int gDefaultDifficulty;
}
namespace PlayerPrefs
{
extern int gDefaultTeamNum;
}
