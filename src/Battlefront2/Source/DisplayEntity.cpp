#include "DisplayEntity.h"

// FUNCTION: SWBF2 0x0048d620
void EntityElement::SetScale(float fScale)
{
    mScale = fScale;
}

// FUNCTION: SWBF2 0x0048d640
void EntityElement::SetSelected(bool bSelected)
{
    (void)bSelected;
}

// FUNCTION: SWBF2 0x0048d650
EntityElement::EntityElement()
{
    mScale = 1.0f;
    mRotation = PblQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
    mTranslation = PblVector3(0.0f, 0.0f, 0.0f);
    mModel = 0;
}

// SYNTHETIC: SWBF2 0x0048d6f0
// EntityElement::`scalar deleting destructor'
// FUNCTION: SWBF2 0x0048d720
EntityElement::~EntityElement()
{
}

// FUNCTION: SWBF2 0x0048d730
void EntityElement::RenderUsingContext(const PblMatrix&, const RedColor&)
{
}
