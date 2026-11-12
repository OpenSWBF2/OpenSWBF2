#include "Battlefront2/Source/VideoEffect.h"

#include "RedEngineFL/Application/Renderable.h"

// VTABLE: SWBF2 0x00796ac0
class VideoEffectRenderable : public Renderable
{
public:

    VideoEffectRenderable()
        : Renderable(RENDERABLE_SCREENEFFECT)
    {
    }

    // SYNTHETIC: SWBF2 0x00433df0
    // VideoEffectRenderable::`scalar deleting destructor'

    virtual void Render(unsigned int value) const;
};

// FUNCTION: SWBF2 0x00433e20
void VideoEffectRenderable::Render(unsigned int) const
{
}

// GLOBAL: SWBF2 0x01e2fd58
static VideoEffectRenderable* s_pRenderable;

// FUNCTION: SWBF2 0x00433e30
void VideoEffect::Init()
{
    s_pRenderable = new VideoEffectRenderable;
}
