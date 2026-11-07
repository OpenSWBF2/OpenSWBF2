#include "Battlefront2/Source/Tracker.h"
#include "Battlefront2/Source/Trackable.h"
#include "Battlefront2/Source/CameraManager.h"
#include "Battlefront2/Source/NetGame.h"

int netHostIndex = -1;
bool netClientActive;

CameraManager* CameraManager::sInstance;
void CameraManager::Chase(Trackable*, int) {}
void CameraManager::TrackDeath(Trackable*, int, Character*) {}

int NetGame::GetJoystickIndex(int) { return -1; }

AloTrackable::AloTrackable()
{
    sig = ALO_TRACKABLE;
    deltaSends = false;
    Reset();
}

void AloTrackable::Reset()
{
    for (int i = 0; i < 64; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            mTrackable[i][j] = (Trackable*)-1;
        }
    }
    for (int i = 0; i < 64; i++)
    {
        changeTurn[i] = (unsigned int)-1;
    }
}

AloTrackable aloTrackable;

Trackable::~Trackable()
{
    Tracker::RemoveTrackable(this);
}

void Trackable::SetTracker(Tracker* pTracker) { mTracker = pTracker; }
void Trackable::SetupCameraTrackMatrixHUDTargeting(PblMatrix&) {}
void Trackable::SetupCameraTrackMatrix(PblMatrix&, float) {}
void Trackable::SetupCameraZoomFactor(float&) {}
void Trackable::SetupCameraFOV(float&) {}
void Trackable::SetupCameraIconVisibility(unsigned int&) {}
void Trackable::SetupInterpolatedCameraTrackMatrix(PblMatrix&) {}
GameObject* Trackable::GetGameObject() { return 0; }
const GameObject* Trackable::GetGameObject() const { return 0; }
Controllable* Trackable::GetControllable() { return 0; }
const Controllable* Trackable::GetControllable() const { return 0; }
CameraTrackSetting* Trackable::GetCameraTrackSettings() { return 0; }
bool Trackable::RenderTrackable(unsigned int) const { return true; }
bool Trackable::IsFirstPersonAvailablePS2() const { return true; }
void Trackable::SetFirstPersonView(int, bool) {}
bool Trackable::IsForcedThirdPerson() const { return false; }
bool Trackable::IsForcedFirstPerson() const { return false; }
