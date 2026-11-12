namespace PblProfile
{

// FUNCTION: SWBF2 0x0072b330
void StartFrame()
{
}

}

namespace PblProfile
{

// FUNCTION: SWBF2 0x0072b300
void EndFrame()
{
}

}

// GLOBAL: SWBF2 0x009cefc0
// mPausedFlag
namespace PblProfile
{
static bool mPausedFlag;
}

namespace PblProfile
{

// FUNCTION: SWBF2 0x0072b310
bool IsPaused()
{
    return PblProfile::mPausedFlag;
}
}

namespace PblProfile
{

// FUNCTION: SWBF2 0x0072b320
void SetPaused(bool bValue)
{
    PblProfile::mPausedFlag = bValue;
}
}
