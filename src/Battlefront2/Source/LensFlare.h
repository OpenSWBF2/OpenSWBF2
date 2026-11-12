#pragma once

#include <stddef.h>
#include "FLEffect.h"
#include "PebbleFL/Common/PblList.h"
#include "PebbleFL/Common/PblMath.h"
#include "PebbleFL/Common/PblRandom.h"
#include "RedEngineFL/Graphics/RedColor.h"

class LensFlare;
struct FlareVertex;
struct FlareCompressedVertex;

// VTABLE: SWBF2 0x007a3b18
class LensFlareEffect : public FLEffect
{
public:

    class LensFlareList : public PblList<LensFlare>
    {
    public:

        void Cleanup();

        void Add(LensFlare* pFlare);
        void Remove(LensFlare* pFlare);

        template <class VERTEX, int RVF>
        void Render() const;
    };

    LensFlareEffect()
        : FLEffect(0xAEC1B09Fu /* "LensFlare" */, RENDERABLE_POSTTRANSPARENT)
    {
    }

    // SYNTHETIC: SWBF2 0x0056f430
    // LensFlareEffect::`scalar deleting destructor'
    virtual void Render(unsigned int value) const;
    virtual void Init();
    virtual void Cleanup();
    virtual FLEffectClass* BuildClass(PblConfig& config) const;
    virtual void PlatformInit();
    virtual void PlatformCleanup();

    static void Create();

    static void Destroy();

    static LensFlareEffect* s_pInstance;

    LensFlareList _List;
};
 
 
 // VTABLE: SWBF2 0x0079b860 PblHandled
 // VTABLE: SWBF2 0x0079b868
 // VTABLE: SWBF2 0x0079b8e0 Thread
 class LensFlare : public FLEffectClass, public FLEffectObject
{
public:
    // FUNCTION: SWBF2 0x004bd990
    LensFlare()
        : _fSize(1.0f)
        , _color(RedColor::WHITE)
        , _fSquaredDrawDistance(0.0f)
        , _uiHideInView(0)
    {
        _fRandomRotOffset = PblRandom::_Global.GetFloat() * PblMath::fTwoPi;
    }
    // FUNCTION: SWBF2 0x0056f4a0
    LensFlare(const LensFlare& that)
        : _fSize(that._fSize)
        , _color(that._color)
        , _fSquaredDrawDistance(that._fSquaredDrawDistance)
        , _uiHideInView(0)
    {
    }
    // [thunk]:LensFlare::`scalar deleting destructor' adjustor{40}
    // SYNTHETIC: SWBF2 0x004c0902
    // ??_ELensFlare@@WCI@AEPAXI@Z
    // [thunk]:LensFlare::`scalar deleting destructor' adjustor{16}
    // SYNTHETIC: SWBF2 0x004c090a
    // ??_ELensFlare@@WBA@AEPAXI@Z
    // SYNTHETIC: SWBF2 0x004bdb80
    // LensFlare::`scalar deleting destructor'
    // FUNCTION: SWBF2 0x004bdb00
    virtual ~LensFlare() { DeactivateEffect(); }
    virtual FLEffectObject* BuildEffect();
    // FUNCTION: SWBF2 0x004bdb70
    virtual float GetSize() const { return _fSize; }

    void SetColor(const RedColor& color) { _color = color; }

    void SetSize(float fSize) { _fSize = fSize; }
    void SetDrawDistance(float fDistance) { _fSquaredDrawDistance = fDistance * fDistance; }

    const RedColor GetColor() const { return _color; }
    float GetSquaredDrawDistance() const { return _fSquaredDrawDistance; }
    bool IsHiddenInView(unsigned int uiView) const { return (_uiHideInView & (1 << uiView)) != 0; }
    virtual bool IsEffectActive() const;
    virtual void ActivateEffect();
    virtual void DeactivateEffect();
protected:

    template <class VERTEX>
    void Emit(VERTEX*& pVertices, const PblVector3& vPosition, float fU, float fV, float fFade);
    template <class VERTEX>
    static void EmitRing(VERTEX*& pVertices, const PblVector3& vPosition, float, float, float,
                         float, const RedColor&, const RedColor&, const RedColor&, int, float,
                         float);
    friend class LensFlareEffect::LensFlareList;

    virtual bool Update(float fDeltaTime);
    virtual void UpdatedWorldMatrix();
public:
    float _fSize;
    RedColor _color;
    float _fSquaredDrawDistance;
    unsigned int _uiHideInView;
    float _fRandomRotOffset;
    PblList<LensFlare>::Node _Node;
};
 
 
 
