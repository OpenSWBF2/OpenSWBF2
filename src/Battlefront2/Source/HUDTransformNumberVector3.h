#pragma once

#include <stddef.h>
#include "HUDTransformNumberColor.h"
#include "PebbleFL/Common/PblVector.h"

namespace HUD
{
// VTABLE: SWBF2 0x007a371c
class TransformNumberVector3 : public HUD::TransformNumberType
{
public:

    TransformNumberVector3(const char* const pName, Factory::ItemCallback pCallback, void* const pUserData);

    static void EventInput(const HUD::Event& event, void* const pData);
    virtual ~TransformNumberVector3();

    // VTABLE: SWBF2 0x007a32e4

    class Factory : public HUD::Item::Factory
    {
    public:
        Factory();

        // SYNTHETIC: SWBF2 0x00564c20
        // HUD::TransformNumberVector3::Factory::`scalar deleting destructor'

        virtual HUD::Item* CreateItem(const char* const pName, ItemCallback pCallback,
                                      void* const pUserData);
    };

    struct NumberVector3
    {
        float number;
        PblVector3 vector3;
    };

    void Read(PblConfig& config, Factory::ItemCallback pCallback, void* const pUserData);

protected:

    PblVector3 CalculateOutputVector3(float fInput) const;

    bool ReadData(PblConfig& config, PblConfig::Data& data);
    void WriteData(HUD::File& file, unsigned int uTabs);

public:
    NumberVector3* mMapping;
    unsigned int mNumMappings;
};
}
 
 
 

inline HUD::TransformNumberVector3::Factory::Factory()
    : HUD::Item::Factory(0x2ef8f006u /* "TransformNumberVector3" */)
{
    mName = "TransformNumberVector3";
}

// FUNCTION: SWBF2 0x00564ba0
inline HUD::Item* HUD::TransformNumberVector3::Factory::CreateItem(const char* const pName, ItemCallback pCallback,
                                                                   void* const pUserData)
{
    HUD::Item* pItem = new TransformNumberVector3(pName, pCallback, pUserData);
    if (pItem)
    {
        return pItem->SetFactory(this);
    }
    return 0;
}
