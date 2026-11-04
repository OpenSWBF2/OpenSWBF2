#include "CommandBuildingArmed.h"
#include "GameModel.h"
#include "NetGame.h"

// SYNTHETIC: SWBF2 0x00402660
// `dynamic initializer for 'CommandBuildingArmed::rttiHashCommandBuildingArmed''
// CommandBuildingArmed::rttiHashCommandBuildingArmed

// GLOBAL: SWBF2 0x01eb928c
// CommandBuildingArmed::rttiHashCommandBuildingArmed
PblHash CommandBuildingArmed::rttiHashCommandBuildingArmed("CommandBuildingArmed");
// GLOBAL: SWBF2 0x00788c44
// CommandBuildingArmed::rttiNameCommandBuildingArmed
const char CommandBuildingArmed::rttiNameCommandBuildingArmed[] = "CommandBuildingArmed";







// FUNCTION: SWBF2 0x00476f50
EntityType CommandBuildingArmed::GetEntityType() const
{
    return ENT_POST;
}

// FUNCTION: SWBF2 0x00477000
CommandBuildingArmed::CommandBuildingArmed(CommandBuildingArmedClass* pClass, const EntityDesc& desc)
    : EntityBuildingArmed(pClass, desc)
{
    mClass = pClass;
    mPost = CommandPost::BuildPost(this, pClass);
}

// FUNCTION: SWBF2 0x004770a0
CommandBuildingArmed::~CommandBuildingArmed()
{
}

// SYNTHETIC: SWBF2 0x00476f60
// CommandBuildingArmed::`scalar deleting destructor'
// SYNTHETIC: SWBF2 0x00477496
// ??_ECommandBuildingArmed@@WM@AEPAXI@Z
// SYNTHETIC: SWBF2 0x0047746a
// ??_ECommandBuildingArmed@@WCAA@AEPAXI@Z
// SYNTHETIC: SWBF2 0x00477475
// ??_ECommandBuildingArmed@@WCGA@AEPAXI@Z
// SYNTHETIC: SWBF2 0x00477480
// ??_ECommandBuildingArmed@@WJE@AEPAXI@Z
// SYNTHETIC: SWBF2 0x0047748b
// ??_ECommandBuildingArmed@@WBEA@AEPAXI@Z

// FUNCTION: SWBF2 0x004770e0
void CommandBuildingArmed::Init()
{
    EntityBuildingArmed::Init();
    if (mState == STATE_ALIVE)
    {
        if (mPost)
        {
            mPost->Respawn();
        }
    }
}

// FUNCTION: SWBF2 0x00477100
void CommandBuildingArmed::Kill()
{
    EntityBuildingArmed::Kill();
    if (mPost)
    {
        mPost->Kill();
    }
}

// FUNCTION: SWBF2 0x00477120
void CommandBuildingArmed::Respawn()
{
    EntityBuildingArmed::Respawn();
    if (mPost)
    {
        mPost->Respawn();
    }
}

// FUNCTION: SWBF2 0x00477140
void CommandBuildingArmed::SetProperty(unsigned int uProperty, const char* pszValue)
{
    EntityBuildingArmed::SetProperty(uProperty, pszValue);
    if (mPost)
    {
        mPost->SetProperty(uProperty, pszValue);
    }
}

// FUNCTION: SWBF2 0x00477170
void CommandBuildingArmed::SetTeam(int iTeam)
{
    int iOldTeam = mTeam;
    GameObject::SetTeam(iTeam);
    if (mPost)
    {
        mPost->SetTeam(iOldTeam, iTeam);
    }
}

// FUNCTION: SWBF2 0x004771a0
CommandBuildingArmedClass::CommandBuildingArmedClass(unsigned int uHash)
    : EntityBuildingArmedClass(uHash), CommandPostClass(uHash)
{
}

// FUNCTION: SWBF2 0x00477230
CommandBuildingArmedClass::CommandBuildingArmedClass(CommandBuildingArmedClass* pBase, unsigned int uHash)
    : EntityBuildingArmedClass(pBase, uHash), CommandPostClass(pBase, uHash)
{
}

// FUNCTION: SWBF2 0x004772b0
CommandBuildingArmedClass::~CommandBuildingArmedClass()
{
}

// SYNTHETIC: SWBF2 0x00477200
// CommandBuildingArmedClass::`scalar deleting destructor'
// SYNTHETIC: SWBF2 0x0047749e
// ??_ECommandBuildingArmedClass@@WGHI@AEPAXI@Z

// SYNTHETIC: SWBF2 0x00402670
// `dynamic initializer for 'CommandBuildingArmedClass::rttiHashCommandBuildingArmedClass''
// CommandBuildingArmedClass::rttiHashCommandBuildingArmedClass

// GLOBAL: SWBF2 0x01eb9554
// CommandBuildingArmedClass::rttiHashCommandBuildingArmedClass
PblHash CommandBuildingArmedClass::rttiHashCommandBuildingArmedClass("CommandBuildingArmedClass");
// GLOBAL: SWBF2 0x00788c20
// CommandBuildingArmedClass::rttiNameCommandBuildingArmedClass
const char CommandBuildingArmedClass::rttiNameCommandBuildingArmedClass[] = "CommandBuildingArmedClass";







// FUNCTION: SWBF2 0x00477310
void CommandBuildingArmedClass::SetProperty(unsigned int uProperty, const char* pszValue)
{
    EntityBuildingArmedClass::SetProperty(uProperty, pszValue);
    CommandPostClass::SetProperty(uProperty, pszValue, mModel ? mModel->_mLod[0] : 0, 0);
}

// FUNCTION: SWBF2 0x00477350
EntityClass* CommandBuildingArmedClass::Derive(unsigned int uHash)
{
    return new CommandBuildingArmedClass(this, uHash);
}

// FUNCTION: SWBF2 0x004773c0
Entity* CommandBuildingArmedClass::Build(const EntityDesc& desc)
{
    CommandBuildingArmed* pBuilding = new CommandBuildingArmed(this, desc);
    if (pBuilding)
    {
        pBuilding->AddStaticToCollisionManager();
        pBuilding->ActivatePhysics();
        if (!NetGame::IsEnabled() || !NetGame::IsClient())
        {
            pBuilding->BuildEffects();
        }
    }
    return pBuilding;
}
