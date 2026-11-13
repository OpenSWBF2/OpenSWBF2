#pragma once

#include "RedEngineFL/Util/ext/basevector.h"
#include "RedEngineFL/geometry/vector.h"
#include "RedEngineFL/geometry/polygons.h"
#include "RedEngineFL/geometry/Primitives.h"

namespace pvs
{
class ICamera;
class IDisplayable;
class IPortal;
class ISector;
class ITraverser;
class Sector;
class Manager;
}

namespace pvs
{

// VTABLE: SWBF2 0x00783534
class IManager
{
public:
    virtual void Update(const pvs::ICamera &, pvs::ITraverser *) = 0;
    virtual void RegisterDynamic(pvs::IDisplayable &) = 0;
    virtual void UnregisterDynamic(pvs::IDisplayable &) = 0;
    virtual void RegisterStatic(pvs::IDisplayable &) = 0;
    virtual void UnregisterStatic(pvs::IDisplayable &) = 0;
    virtual pvs::ISector & GetRootSector() = 0;
    virtual bool IsActive() = 0;
    virtual void SetActive(bool) = 0;
    virtual void Allocate(int, int) = 0;
    virtual pvs::IPortal * GetPortal(int) = 0;
    virtual pvs::ISector * GetSector(int) = 0;
    virtual pvs::IPortal * NewPortal() = 0;
    virtual pvs::ISector * NewSector() = 0;
    virtual pvs::IPortal * FindNearestPortal(const geo::Sphere &, float) = 0;
    // FUNCTION: SWBF2 0x006deec0
    virtual ~IManager() {}

    static IManager & GetInstance();
    static void SetInstance(IManager & rManager);
    static void ClearInstance();
    static void ReserveDisplayables();
};

class IPortal
{
public:
    virtual void SetActive(bool) = 0;
    virtual bool IsActive() = 0;
    virtual pvs::ISector * GetLeft() = 0;
    virtual pvs::ISector * GetRight() = 0;
    virtual void SetLeft(pvs::ISector *) = 0;
    virtual void SetRight(pvs::ISector *) = 0;
    virtual geo::Poly3D & GetPoly() = 0;
    virtual const geo::Poly3D & GetPoly() const = 0;
    virtual void Init(pvs::Sector *, pvs::Sector *) = 0;
    virtual void InitBound() = 0;
};

class ISector
{
public:
    virtual void RegisterDynamic(pvs::IDisplayable &) = 0;
    virtual void UnregisterDynamic(pvs::IDisplayable &) = 0;
    virtual void RegisterStatic(pvs::IDisplayable &) = 0;
    virtual void UnregisterStatic(pvs::IDisplayable &) = 0;
    virtual util::Vector<unsigned int> & GetStaticNames() = 0;
    virtual void AddPortal(pvs::IPortal *) = 0;
    virtual void SetBound(const geo::ExtrudedPoly3D &) = 0;
    virtual geo::ExtrudedPoly3D & GetBound() = 0;
};

class IDisplayable
{
public:
    virtual void OnTraverse(pvs::ITraverser &) = 0;
    virtual void SetActive(int) = 0;
    virtual bool IsActive(int) const = 0;
    virtual void SetProcessed(int) = 0;
    virtual bool IsProcessed(int) const = 0;
    virtual void GetAABound(geo::Box &) const = 0;
    virtual void GetSphereBound(geo::Sphere &) const = 0;
    virtual bool PortalClip() const = 0;
    virtual void SetPortalClip(bool) = 0;
    virtual bool PortalActive() const = 0;
    virtual void SetPortalActive(bool) = 0;
    virtual bool Dynamic() const = 0;
    virtual void SetDynamic(bool) = 0;
    virtual pvs::Sector * GetSector() = 0;
    virtual void SetSector(pvs::Sector *) = 0;
    virtual void Update(pvs::Manager &) = 0;
};

template <class T>
class BaseDisplayable : public T
{
public:
    BaseDisplayable() : sector_(0), flags_(0), aframe_(0), pframe_(0) {}

    virtual ~BaseDisplayable() {}

    // TEMPLATE: SWBF2 0x00446c30
    // pvs::BaseDisplayable<pvs::IDisplayable>::OnTraverse
    virtual void OnTraverse(pvs::ITraverser&)
    {
    }
    // TEMPLATE: SWBF2 0x00446c40
    // pvs::BaseDisplayable<pvs::IDisplayable>::SetActive
    virtual void SetActive(int iFrame)
    {
        aframe_ = iFrame;
    }
    // TEMPLATE: SWBF2 0x00446c50
    // pvs::BaseDisplayable<pvs::IDisplayable>::IsActive
    virtual bool IsActive(int iFrame) const
    {
        return aframe_ >= iFrame;
    }
    // TEMPLATE: SWBF2 0x00446c70
    // pvs::BaseDisplayable<pvs::IDisplayable>::SetProcessed
    virtual void SetProcessed(int iFrame)
    {
        pframe_ = iFrame;
    }
    // TEMPLATE: SWBF2 0x00446c80
    // pvs::BaseDisplayable<pvs::IDisplayable>::IsProcessed
    virtual bool IsProcessed(int iFrame) const
    {
        return pframe_ == iFrame;
    }
    // TEMPLATE: SWBF2 0x00446cd0
    // pvs::BaseDisplayable<pvs::IDisplayable>::PortalClip
    virtual bool PortalClip() const
    {
        return (flags_ >> 2) & 1;
    }
    // TEMPLATE: SWBF2 0x00446ce0
    // pvs::BaseDisplayable<pvs::IDisplayable>::SetPortalClip
    virtual void SetPortalClip(bool bClip)
    {
        if (bClip)
        {
            flags_ |= 4;
        }
        else
        {
            flags_ &= ~4u;
        }
    }
    // TEMPLATE: SWBF2 0x00446ca0
    // pvs::BaseDisplayable<pvs::IDisplayable>::PortalActive
    virtual bool PortalActive() const
    {
        return (flags_ >> 1) & 1;
    }
    // TEMPLATE: SWBF2 0x00446cb0
    // pvs::BaseDisplayable<pvs::IDisplayable>::SetPortalActive
    virtual void SetPortalActive(bool bActive)
    {
        if (bActive)
        {
            flags_ |= 2;
        }
        else
        {
            flags_ &= ~2u;
        }
    }
    // TEMPLATE: SWBF2 0x00446d00
    // pvs::BaseDisplayable<pvs::IDisplayable>::Dynamic
    virtual bool Dynamic() const
    {
        return (flags_ >> 3) & 1;
    }
    // TEMPLATE: SWBF2 0x00446d10
    // pvs::BaseDisplayable<pvs::IDisplayable>::SetDynamic
    virtual void SetDynamic(bool bDynamic)
    {
        if (bDynamic)
        {
            flags_ |= 8;
        }
        else
        {
            flags_ &= ~8u;
        }
    }
    // TEMPLATE: SWBF2 0x00446d30
    // pvs::BaseDisplayable<pvs::IDisplayable>::GetSector
    virtual pvs::Sector* GetSector()
    {
        return sector_;
    }
    virtual void SetSector(pvs::Sector *pSector)
    {
        if (!this->PortalActive())
        {
            sector_ = pSector;
            return;
        }
        if (pSector)
        {
            this->SetPortalActive(true);
        }
        sector_ = pSector;
    }
    virtual void Update(pvs::Manager &)
    {
    }

    pvs::Sector* sector_;
    unsigned int flags_;
    int aframe_;
    int pframe_;
};

// VTABLE: SWBF2 0x007835bc
class DynDisplayable : public pvs::BaseDisplayable<pvs::IDisplayable>
{
public:
    DynDisplayable();
    virtual ~DynDisplayable();

    virtual void SetSector(pvs::Sector *);
    virtual void Update(pvs::Manager &);

    float nextCheckDist_;
    geo::Vector3<float> lastCheckPos_;
};
}

// VTABLE: SWBF2 0x00783574
// class pvs::BaseDisplayable<pvs::IDisplayable>
