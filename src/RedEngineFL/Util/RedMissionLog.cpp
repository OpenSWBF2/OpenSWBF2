#include "RedMissionLog.h"

namespace RedMissionLog
{

// GLOBAL: SWBF2 0x00964f78
bool mEnabled;

// GLOBAL: SWBF2 0x007df210
bool mWriteWarnings = true;

// FUNCTION: SWBF2 0x006ccca0
bool Enabled()
{
    return RedMissionLog::mEnabled;
}

// FUNCTION: SWBF2 0x006cccc0
void Init()
{
}

// FUNCTION: SWBF2 0x006cccd0
void Reset()
{
}

// FUNCTION: SWBF2 0x006cccb0
bool GetWriteWarnings()
{
    return mWriteWarnings;
}

}
