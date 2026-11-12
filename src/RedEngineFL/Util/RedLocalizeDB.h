#pragma once

#include "PebbleFL/Common/PblBuildType.h"

class PblFileChunk;

namespace RedLocalizeDB
{

eGAMELANGUAGE GetLanguageID();

void ForceLanguageID(eGAMELANGUAGE eLanguage);

void PickLanguage();

char const* GetLanguageStr();

bool Read(PblFileChunk& chunk);

void ReadMasterLocalizeFile(PblFileChunk& chunk);

const unsigned short* Find(unsigned int uiHash, bool bNullIfMissing);

void GetLocalizedString(unsigned short* pusBuffer, unsigned int uiSize, unsigned int uiHash);

void Reset();

void SetSkipCount(int count);

}

struct FLLocalizeEntry
{
    int mEntryHash;
    unsigned short mEntryLen;
    unsigned short mData[1];
};
