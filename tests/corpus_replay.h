#pragma once
#include <string>
#include <vector>

typedef std::vector<unsigned char> ByteVec;

std::vector<std::string> ListCorpus(const char* pcReader, bool bKnownBug);

bool LoadCorpusFile(const std::string& sPath, ByteVec& bytes);

bool ReplayRecvGroupBytes(const ByteVec& bytes, bool* pbConsumed = 0);
bool ReplayRecvPacketBytes(const unsigned char* pData, unsigned int uiSize);
