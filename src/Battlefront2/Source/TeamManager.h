#pragma once
#include "PebbleFL/Common/PblHandle.h"
#include "PebbleFL/Common/PblVector.h"

class GameObject;
class PblVector3;
class Team;

namespace AI
{
enum AIAffil
{
    AFFIL_ENEMY = 1,
    AFFIL_NEUTRAL = 2,
    AFFIL_ALLY = 4,
    AFFIL_ALLYNEUTRAL = 6,
    AFFIL_ALL = 7
};
}

namespace TeamManager
{
void Init();
void UpdatePositions();
void EnterClient();
void LeaveClient();

void AddObject(GameObject* pObject);
void RemoveObject(GameObject* pObject);
int sGetObjectsInRange(const PblVector3& vPos, float fRange, GameObject** const ppObjects, int iMax,
                       const Team* pTeam, int iAffilMask, const GameObject* pIgnore);
int GetObjectsInRange(const PblVector3& vPos, float fRange, GameObject** const ppObjects, int iMax,
                      const GameObject* pIgnore, AI::AIAffil eAffil);
int GetObjectsInRange(const PblVector3& vPos, float fRange, GameObject** const ppObjects, int iMax,
                      const Team* pTeam, int iAffilMask);
int GetApproxObjectsAlongLine(const PblVector3& vStart, const PblVector3& vEnd, float fRadius,
                              GameObject** const ppObjects, int iMax, const GameObject* pIgnore,
                              AI::AIAffil eAffil);
}
namespace TeamManager
{
void UpdatePositions();
}

class GameObject;

namespace TeamManager
{

struct ObjCacheData
{

    // SYNTHETIC: SWBF2 0x00654fb0
    // TeamManager::ObjCacheData::ObjCacheData

    // FUNCTION: SWBF2 0x00654fc0
    ~ObjCacheData() {}

    PblHandle<GameObject> mObj;
    PblSphere mSphere;
};

}
