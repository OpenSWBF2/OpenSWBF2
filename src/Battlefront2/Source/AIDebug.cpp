#include "RedEngineFL/Graphics/RedCamera.h"

#include "Character.h"
#include "CollisionManager.h"
#include "PebbleFL/Common/PblMath.h"
#include "PebbleFL/Common/PblMatrix.h"
#include "PebbleFL/Common/PblRandom.h"
#include "GameLoop.h"
#include "NetGame.h"
#include "PlayerProfile.h"
#include "CameraManager.h"

unsigned int Hash16(unsigned short const* pusString);

namespace AIDebugR
{
    // GLOBAL: SWBF2 0x01e2fd5c
    bool mBulletTimeEnabled;
}
// GLOBAL: SWBF2 0x01e2fd5d

bool gTestSpawnTakeoverPending;

// GLOBAL: SWBF2 0x01e2fd5e

bool gTestSpawnForced;
// GLOBAL: SWBF2 0x01e2fd5f

bool gTestSpawnEnabled;

// GLOBAL: SWBF2 0x007e5885

bool gTestSpawnOptionEnabled = true;

extern bool gNoAIRespawn;

namespace AIDebugR
{
    // GLOBAL: SWBF2 0x01e2fd60
    float mBulletTimeScale;
}
namespace AIDebugR
{
    // GLOBAL: SWBF2 0x01e2fd64
    float mBulletTimeStart;
}
namespace AIDebugR
{
    // GLOBAL: SWBF2 0x01e2fd68
    float mBulletTimeUntil;
}

namespace AIDebug
{
    namespace TestSpawn
    {
        void Create();
    }
}

namespace AIDebug
{

// FUNCTION: SWBF2 0x00438a00
bool GetTestSpawnMode()
{
    return false;
}

}

namespace AIDebugR
{

// FUNCTION: SWBF2 0x004385d0
void Create()
{
    PlayerProfile::tProfile* pProfile = PlayerProfile::GetProfile(0);
    const unsigned int uTail = Hash16(pProfile->GetName() + 1);
    AIDebugR::mBulletTimeEnabled = Hash16(pProfile->GetName()) - uTail == 0x60081D20;
}

// FUNCTION: SWBF2 0x00438610
void StartBulletTime(float fFirst, float fSecond, float fThird)
{
    if (!AIDebugR::mBulletTimeEnabled)
    {
        return;
    }
    float fTime = GameLoop::GetMissionTime() + fFirst;
    AIDebugR::mBulletTimeStart = fTime;
    AIDebugR::mBulletTimeUntil = fTime + fThird;
    AIDebugR::mBulletTimeScale = fSecond;
}

}

namespace AIDebug
{

// FUNCTION: SWBF2 0x00438660
void Create()
{
    AIDebugR::Create();
    TestSpawn::Create();
}

// FUNCTION: SWBF2 0x004386b0
void StartBulletTime(float fFirst, float fSecond, float fThird)
{
    if (!NetGame::IsEnabled() && CameraManager::Instance()->GetNumCameras() == 1)
    {
        AIDebugR::StartBulletTime(fFirst, fSecond, fThird);
    }
}

// FUNCTION: SWBF2 0x004386e0
void TestSpawn::Create()
{
    if (!gTestSpawnForced)
    {
        PlayerProfile::tProfile* pProfile = PlayerProfile::GetProfile(0);
        const unsigned int uDiff = Hash16(pProfile->GetName()) - Hash16(pProfile->GetName() + 1);
        gTestSpawnEnabled = false;
        if (uDiff == 0xC2ADE824)
        {
            gTestSpawnEnabled = true;
        }
    }
    else
    {
        gTestSpawnEnabled = true;
    }
    if (GetTestSpawnMode())
    {
        gTestSpawnTakeoverPending = false;
        gTestSpawnOptionEnabled = true;
        gNoAIRespawn = true;
    }
}

}

// FUNCTION: SWBF2 0x00438a20
bool __cdecl IsCoverHintFilteringEnabled()
{
    return false;
}

namespace AIDebug
{

// FUNCTION: SWBF2 0x00438670
bool AllowUpdateFreeCamSpeeds()
{
    return true;
}

// FUNCTION: SWBF2 0x00438680
bool OnlyOneAI()
{
    return false;
}

// FUNCTION: SWBF2 0x00438690
bool PauseAfterPathFind()
{
    return false;
}

// FUNCTION: SWBF2 0x004386a0
bool SkipCameraForLODs()
{
    return false;
}

}

// GLOBAL: SWBF2 0x007eb760
// CYL_CENTER
static PblVector3 CYL_CENTER(-74.0f, 12.0f, 20.0f);

namespace AIDebug
{
namespace TestSpawn
{

// GLOBAL: SWBF2 0x01e2fd6c
Character* aic[4];

// SYNTHETIC: SWBF2 0x004021a0
// AIDebug::TestSpawn::`dynamic initializer for 'mCamera''
// SYNTHETIC: SWBF2 0x00766d30
// AIDebug::TestSpawn::`dynamic atexit destructor for 'mCamera''

// GLOBAL: SWBF2 0x01eb0250
// mCamera
RedCamera mCamera;

// SYNTHETIC: SWBF2 0x004021c0
// AIDebug::TestSpawn::`dynamic initializer for 'camDir''

// GLOBAL: SWBF2 0x01eaffe4
// camDir
PblVector3 camDir = CYL_CENTER;

// FUNCTION: SWBF2 0x00438750
PblMatrix GetTestSpawnMatrix(Character* pCharacter)
{
    if (!pCharacter->IsPlayer() &&
        AIDebug::TestSpawn::aic[0] != pCharacter &&
        AIDebug::TestSpawn::aic[1] != pCharacter &&
        AIDebug::TestSpawn::aic[2] != pCharacter &&
        AIDebug::TestSpawn::aic[3] != pCharacter)
    {
        PblMatrix mSpawn(gMatrixIdentity);
        mSpawn.SetTranslation(PblVector3((float)Character::GetIndex(pCharacter) * 3.0f, 100.0f, 0.0f));
        return mSpawn;
    }

    const float kHalfHeight = 25.0f;

    const float fAngle = PblRandom::_Global.GetFloat() * 6.2831855f;
    float fCos, fSin;
    PblMath::SinCos(&fSin, &fCos, fAngle);
    const float fRadius = PblRandom::_Global.GetFloat() * 32.0f;
    const PblVector3 vStart = fRadius * PblVector3(fSin, 0.0f, fCos) + CYL_CENTER + PblVector3(0.0f, kHalfHeight, 0.0f);
    const PblVector3 vDown = -PblVector3(0.0f, 1.0f, 0.0f);
    CollisionObject* pHitObject = 0;
    const float fHit = CollisionManager::RayHit(vStart, vDown, kHalfHeight * 2.0f,
                                                 pHitObject, 0, 0, 0, 0, false);
    const PblVector3 vPosition = vStart - PblVector3(0.0f, fHit * kHalfHeight * 2.0f, 0.0f);

    return PblMatrix(PblVector3(-fCos, 0.0f, fSin),
                     PblVector3(0.0f, 1.0f, 0.0f),
                     PblVector3(-fSin, 0.0f, -fCos),
                     vPosition);
}

}

// FUNCTION: SWBF2 0x00438a10
PblMatrix GetTestSpawnMatrix(Character* pCharacter)
{
    return AIDebug::TestSpawn::GetTestSpawnMatrix(pCharacter);
}
}
