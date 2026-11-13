#ifndef SWBF2_GRAPHICS_RENDERABLE_H
#define SWBF2_GRAPHICS_RENDERABLE_H


#include "Battlefront2/Source/Thread.h"

enum RENDERABLE_PHASE
{
    RENDERABLE_SKY,
    RENDERABLE_SKY_POSTWORLD,
    RENDERABLE_PREWORLD,
    RENDERABLE_POSTWORLD,
    RENDERABLE_POSTSHADOW,
    RENDERABLE_PRETRANSPARENT,
    RENDERABLE_POSTTRANSPARENT,
    RENDERABLE_PREFIRSTPERSON,
    RENDERABLE_FIRSTPERSON,
    RENDERABLE_SCREENEFFECT,
    NUM_RENDERABLE_PHASES
};

// VTABLE: SWBF2 0x00772acc
class Renderable
{
public:

    Renderable();

    Renderable(RENDERABLE_PHASE ePhase);

    virtual ~Renderable();

    // SYNTHETIC: SWBF2 0x006c3cf0
    // Renderable::`scalar deleting destructor'

    virtual void Activate(int value);

    virtual void Deactivate();

    virtual bool IsActive();

    virtual void Render(unsigned int value) const = 0;

    virtual bool IsReflected();

    virtual bool HasReflection();

    RENDERABLE_PHASE _ePhase;

    PblOrderedList<Renderable, int>::Node _RenderableNode;
};

 
 
 
 #endif
