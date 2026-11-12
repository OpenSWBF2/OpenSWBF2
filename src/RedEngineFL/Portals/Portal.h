#pragma once

#include <stddef.h>

#include "PebbleFL/Common/PblMatrix.h"
#include "RedEngineFL/Util/ext/basevector.h"
#include "RedEngineFL/Util/ext/baselist.h"
#include "RedEngineFL/Util/ext/IAllocator.h"
#include "RedEngineFL/geometry/vector.h"
#include "RedEngineFL/geometry/polygons.h"
#include "RedEngineFL/geometry/Primitives.h"
#include "BaseDisplayable.h"

namespace pvs
{
class ICamera;
}
namespace pvs
{
class IDisplayable;
}
namespace pvs
{
class IPortal;
}
namespace pvs
{
class ISector;
}
namespace pvs
{
class ITraverser;
}
namespace pvs
{
class Sector;
}
namespace pvs
{
class Portal;
}
namespace pvs
{
class BaseCamera;
}
namespace geo
{
struct Box;
}

namespace pvs
{

int GetCurrentFrame_();

}

namespace pvs
{

// VTABLE: SWBF2 0x00783604
class Sector : public pvs::ISector
{
public:
    Sector();
    virtual ~Sector();

    virtual void RegisterDynamic(pvs::IDisplayable &);
    virtual void UnregisterDynamic(pvs::IDisplayable &);
    virtual void RegisterStatic(pvs::IDisplayable &);
    virtual void UnregisterStatic(pvs::IDisplayable &);
    // FUNCTION: SWBF2 0x006e0a80
    virtual util::Vector<unsigned int>& GetStaticNames()
    {
        return staticNames_;
    }
    virtual void AddPortal(pvs::IPortal *);
    // FUNCTION: SWBF2 0x006e14e0
    virtual void SetBound(const geo::ExtrudedPoly3D& rBound)
    {
        bound_ = rBound;
    }
    // FUNCTION: SWBF2 0x006e0660
    virtual geo::ExtrudedPoly3D& GetBound()
    {
        return bound_;
    }

    bool Contains(const geo::Vector3<float> &);
    bool Overlap(const geo::Sphere &);
    void Create();
    geo::Sphere & GetSphereBound();
    bool GetTravMark();
    void SetTravMark(bool);
    void SetActiveTravMark(bool);
    bool TestWindingOrder();
    void Process(pvs::IDisplayable &, const pvs::ICamera &, const geo::Box &, pvs::ITraverser *);

    pvs::Sector * PortalCrossingCheck(const geo::Sphere &, const geo::Vector3<float> &, float &);

    void Traverse(const pvs::ICamera &, const geo::Box &, const geo::Frustum &, pvs::ITraverser *, pvs::Sector *);

    bool GetActiveTravMark();

    float windingMult_;
    bool tmark_;
    bool atmark_;
    bool initdone_;
    util::Vector<pvs::Portal *> portals_;
    util::List<pvs::IDisplayable *, util::SLNode<pvs::IDisplayable *> > dynamics_;
    util::Vector<pvs::IDisplayable *> statics_;
    util::Vector<unsigned int> staticNames_;
    geo::ExtrudedPoly3D bound_;
    geo::Sphere sbound_;
};
}
 
 
 
 
 
 
 
 
 
 
 

namespace pvs
{

class SectorCache
{
public:
    struct Entry
    {
        Entry() : sect_(0), nextCheckDist_(0) {}

        pvs::Sector* sect_;
        float nextCheckDist_;
        geo::Vector3<float> lastCheckPos_;
    };

    // SYNTHETIC: SWBF2 0x006df140
    // pvs::SectorCache::~SectorCache

    util::Vector<Entry> entries_;
};
}
 

namespace pvs
{

// VTABLE: SWBF2 0x0078365c
class Manager : public pvs::IManager
{
public:
    Manager();
    virtual ~Manager();

    virtual void Update(const pvs::ICamera &, pvs::ITraverser *);
    virtual void RegisterDynamic(pvs::IDisplayable &);
    virtual void UnregisterDynamic(pvs::IDisplayable &);
    virtual void RegisterStatic(pvs::IDisplayable &);
    virtual void UnregisterStatic(pvs::IDisplayable &);
    virtual pvs::ISector & GetRootSector();
    virtual bool IsActive();
    virtual void SetActive(bool);
    virtual void Allocate(int, int);
    virtual pvs::IPortal * GetPortal(int);
    virtual pvs::ISector * GetSector(int);
    virtual pvs::IPortal * NewPortal();
    virtual pvs::ISector * NewSector();
    virtual pvs::IPortal * FindNearestPortal(const geo::Sphere &, float);

    pvs::Sector * FindSector(const pvs::IDisplayable &);

    pvs::Sector * FindSector(const geo::Sphere &);

    void FindSectors(const pvs::BaseCamera &, util::Vector<pvs::Sector *> &, pvs::SectorCache &);

    void GetCamFrustum(const pvs::ICamera &, geo::Frustum &);

protected:
    virtual void UpdateDynamics();

    void UpdateDynamics(pvs::Sector &);
    virtual void TraversePortals(const pvs::ICamera &, pvs::ITraverser *);

public:
    pvs::SectorCache scache_;
    PblMatrix saveCamTrans_;
    geo::Vector3<float> saveCamPos_;
    pvs::Sector* saveCamSect_;
    bool lastSaveCam_;
    util::Vector<pvs::Sector *> camsectors_;
    util::Vector<pvs::Sector *> sectors_;
    util::Vector<pvs::Portal *> portals_;
    pvs::Sector rootSect_;
    pvs::Sector* curCamSector_;
    util::Vector<pvs::IDisplayable *> dynamics_;
};
}
 
 
 
 
 
 
 
 
 
 
 
 

namespace pvs
{

// VTABLE: SWBF2 0x0078362c
class Portal : public pvs::IPortal
{
public:
    Portal() : active_(true), clipMark_(false) {}
    // SYNTHETIC: SWBF2 0x006df3a0
    // pvs::Portal::`scalar deleting destructor'
    virtual ~Portal() {}

    // FUNCTION: SWBF2 0x006e14c0
    virtual void SetActive(bool bActive)
    {
        active_ = bActive;
    }
    // FUNCTION: SWBF2 0x006e0be0
    virtual bool IsActive()
    {
        return active_;
    }
    // FUNCTION: SWBF2 0x006e0990
    virtual pvs::ISector* GetLeft()
    {
        return left_;
    }
    // FUNCTION: SWBF2 0x006e09e0
    virtual pvs::ISector* GetRight()
    {
        return right_;
    }
    // FUNCTION: SWBF2 0x006e15b0
    virtual void SetLeft(pvs::ISector* pLeft)
    {
        left_ = (pvs::Sector*)pLeft;
    }
    // FUNCTION: SWBF2 0x006e15d0
    virtual void SetRight(pvs::ISector* pRight)
    {
        right_ = (pvs::Sector*)pRight;
    }
    // FUNCTION: SWBF2 0x006e09b0
    virtual geo::Poly3D& GetPoly()
    {
        return poly_;
    }
    // FUNCTION: SWBF2 0x006e09c0
    virtual const geo::Poly3D& GetPoly() const
    {
        return poly_;
    }
    // FUNCTION: SWBF2 0x006e0aa0
    virtual void Init(pvs::Sector* pLeft, pvs::Sector* pRight)
    {
        left_ = pLeft;
        active_ = true;
        right_ = pRight;
    }
    virtual void InitBound();

    void FindNormal();

    bool Clip(const pvs::ICamera &, geo::Box &, const geo::Frustum &, const pvs::Sector &);

    pvs::Sector* GetOther(pvs::Sector* pSect) { return pSect == left_ ? right_ : left_; }

    pvs::Sector* left_;
    pvs::Sector* right_;
    bool active_;
    bool clipMark_;
    unsigned int travmark_;
    geo::Vector3<float> norm_;
    geo::Poly3D poly_;
    geo::Sphere sbound_;
};
}
 
 
 
 
 
 

namespace pvs
{
class Manager;
}
namespace geo
{
struct Box;
}

namespace pvs
{

class ICamera
{
public:
    virtual void GetPos(geo::Vector3<float> &) const = 0;
    virtual void GetViewTrans(PblMatrix &) const = 0;
    virtual void GetInvViewTrans(PblMatrix &) const = 0;
};

// VTABLE: SWBF2 0x00772db4 pvs::DynDisplayable
// VTABLE: SWBF2 0x00772dfc pvs::ICamera
class BaseCamera : public pvs::DynDisplayable, public pvs::ICamera
{
public:
    BaseCamera() {}
    virtual ~BaseCamera();

    virtual void GetAABound(geo::Box &) const;
    virtual void GetSphereBound(geo::Sphere &) const;
};
}

