#include "Battlefront2/Source/HUDItem.h"

void PblFile::WriteString(const char* pcText)
{
}

unsigned int PblFileChunk::PeekNextChildId() const
{
    return 0;
}

PblConfig::PblConfig(const PblConfig& config, bool bShareChunk)
    : m_Chunk(config.m_Chunk), m_pChunk(config.m_pChunk), m_CurrentChild(config.m_CurrentChild),
      m_uiNameHash(config.m_uiNameHash)
{
}

PblConfig::Data PblConfig::ReadNextData()
{
    return Data();
}

PblConfig PblConfig::ReadNextScope()
{
    return PblConfig(*this, true);
}

namespace HUD
{

EventClass* EventClass::Create(Type eType, const char* pFormat, ...)
{
    return 0;
}

void EventClass::RegisterEventHandler(EventHandler& handler)
{
}

unsigned int EventClass::AddRef()
{
    return 0;
}

EventClass* EventClass::FindByHashID(unsigned int uHashID)
{
    return 0;
}

}
