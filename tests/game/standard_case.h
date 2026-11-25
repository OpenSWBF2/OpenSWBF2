#pragma once
#include <string>

class GameRun;
struct GameRunResult;

const unsigned int kStandardTurnCap = 617300;

std::string StandardTail(bool bSound);
std::string RealTimeTail();
std::string RenderTail();
std::string SmokeFile(const char* pcName);
bool DigestRow(const char* pcName, std::string& sRow);
std::string StandardArgs(const char* pcName, const std::string& sArgs);
void AddStandardTests(GameRun& kRun);
std::string ReadRunLog(const GameRun& kRun);
void CheckStandardRun(const GameRun& kRun, const char* pcName, unsigned int uSoak);
void CheckDigestRow(const std::string& sLog, const std::string& sRow, const std::string& sName);
bool HasLine(const std::string& sLog, const char* pcText);
std::string ReadRunFile(const GameRun& kRun, const char* pcLeaf);
void CheckGroupRows(const GameRun& kRun, const char* pcRow);
int LineCount(const GameRunResult& kResult, const char* pcCategory, int iSeverity, const char* pcRegex);
int LineIndex(const GameRunResult& kResult, const char* pcText, int iFrom = 0);
