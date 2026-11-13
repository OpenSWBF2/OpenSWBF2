#include "Battlefront2/Source/LetterboxEffect.h"

#include "RedEngineFL/Application/Renderable.h"
#include "PebbleFL/Common/PblMath.h"

// VTABLE: SWBF2 0x0079657c
class LetterboxEffectRenderable : public Renderable
{
public:

    LetterboxEffectRenderable()
        : Renderable(RENDERABLE_SCREENEFFECT)
    {
    }

    // SYNTHETIC: SWBF2 0x0041f950
    // LetterboxEffectRenderable::`scalar deleting destructor'

    virtual void Render(unsigned int value) const;
};

// FUNCTION: SWBF2 0x0041f980
void LetterboxEffectRenderable::Render(unsigned int) const
{
}

// GLOBAL: SWBF2 0x01e2fd2c
static LetterboxEffectRenderable* s_pRenderable;

// FUNCTION: SWBF2 0x0041f990
void LetterboxEffect::Init()
{
    s_pRenderable = new LetterboxEffectRenderable;
}


