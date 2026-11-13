#ifndef SWBF2_GRAPHICS_PC_PCDYNAMICSPRITE_H
#define SWBF2_GRAPHICS_PC_PCDYNAMICSPRITE_H

#include "RedEngineFL/Application/Renderable.h"

// VTABLE: SWBF2 0x007964d0
class pcDynamicSprite : public Renderable
{
public:

    // VTABLE: SWBF2 0x007964b0
    class ClearViewportsCallback : public Renderable
    {
    public:

        ClearViewportsCallback(pcDynamicSprite* pOwner)
            : Renderable(RENDERABLE_PREWORLD), m_pOwner(pOwner)
        {
        }

        // FUNCTION: SWBF2 0x004118f0
        virtual ~ClearViewportsCallback()
        {
            Deactivate();
        }

        // SYNTHETIC: SWBF2 0x00411950
        // pcDynamicSprite::ClearViewportsCallback::`scalar deleting destructor'

        virtual void Render(unsigned int value) const;

        pcDynamicSprite* m_pOwner;
    };

    pcDynamicSprite();
    virtual ~pcDynamicSprite();

    // SYNTHETIC: SWBF2 0x00411a80
    // pcDynamicSprite::`scalar deleting destructor'

    virtual void Activate(int value);

    virtual void Deactivate();

    virtual void Render(unsigned int value) const;

    void ClearViewports();

    static void Init();

    static pcDynamicSprite s_Instance;

    int m_iNumViewports;
    int m_iNumViewportsX;

    float m_fViewportWidth;
    float m_fViewportHeight;

    mutable int m_iNumTempBufferSprites;
    mutable bool m_bViewportsLocked;

    ClearViewportsCallback m_ClearViewportsCallback;
};
 
 
 
 
 
 
 
 
 
 

  #endif
