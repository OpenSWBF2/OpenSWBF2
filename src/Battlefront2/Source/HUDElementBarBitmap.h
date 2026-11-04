#pragma once

#include <stddef.h>

#include "HUDElementBar.h"
#include "HUDElementBitmap.h"

namespace HUD
{

// VTABLE: SWBF2 0x007a08ac RedBitmapElement
// VTABLE: SWBF2 0x007a0908 HUD::ElementBitmapBase
// VTABLE: SWBF2 0x007a0944
class ElementBarBitmap : public HUD::ElementBitmap, protected HUD::ElementBar
{
public:

    ElementBarBitmap(const char* const pName, HUD::Item::Factory::ItemCallback pCallback,
                     void* const pUserData);

    // SYNTHETIC: SWBF2 0x0054abe0
    // HUD::ElementBarBitmap::`scalar deleting destructor'
    // SYNTHETIC: SWBF2 0x0054b761
    // ??_EElementBarBitmap@HUD@@WBCA@AEPAXI@Z
    // SYNTHETIC: SWBF2 0x0054b777
    // ??_EElementBarBitmap@HUD@@WCCA@AEPAXI@Z
    // SYNTHETIC: SWBF2 0x0054ac10
    // HUD::ElementBarBitmap::~ElementBarBitmap

    virtual float SetValue(const float fValue);

    virtual bool SetProperty(const unsigned int uHashID, const void* const pValue);
    virtual bool GetProperty(const unsigned int uHashID, void* const pValue);
    virtual void Update(const float fElapsed);
    virtual void GetSize(float& rfWidth, float& rfHeight);

    void RefreshFlashyBit();

protected:

    virtual bool ReadData(PblConfig& config, PblConfig::Data& data);
    virtual void PostReadSetup();
    virtual void WriteData(HUD::File& file, unsigned int uTabs);

    static void EventDisable(const HUD::Event& event, void* const pData);

public:

    // VTABLE: SWBF2 0x007a0864

    class Factory : public HUD::ElementBitmap::Factory, protected HUD::ElementBar::Factory
    {
    public:
        // FUNCTION: SWBF2 0x0054a800
        Factory();

        // SYNTHETIC: SWBF2 0x0054a960
        // HUD::ElementBarBitmap::Factory::`scalar deleting destructor'

        virtual HUD::Item* CreateItem(const char* const pName, ItemCallback pCallback,
                                      void* const pUserData);

        Property mScaleSize;
        Property mScaleTexture;
        Property mFlashyScale;
        Property mFlashyIncFadeOutTime;
        Property mFlashyDecFadeOutTime;
    };

    HUD::ElementBitmap mFlashyBit;
    float mFlashyScale;
    float mFlashyIncFadeOutTime;
    float mFlashyDecFadeOutTime;
    float mBarWidth;
    float mBarU1;
    bool mScaleTex : 1;
    bool mScaleSize : 1;
    bool mIncreasedValue : 1;
    bool mInitialValueSet : 1;
};
}

 
 
 
 
 
 
 
 
