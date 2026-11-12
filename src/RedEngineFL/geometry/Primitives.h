#pragma once

#include <stddef.h>

#include "vector.h"
#include "basics.h"

namespace geo
{

template <class T> class Ray;

struct BasePoly3D;
struct ExtrudedPoly3D;

struct Sphere
{
    Sphere() : radius_(0) {}

    Sphere(const Vector3<float>& rCenter, float fRadius)
        : center_(rCenter), radius_(fRadius) {}

    Sphere(const BasePoly3D& rPoly);
    Sphere(const ExtrudedPoly3D& rPoly);

    void Enclose(const Vector3<float>& point)
    {
        radius_ = Max_(radius_, length(center_ - point));
    }

    Vector3<float> center_;
    float radius_;
};

struct BasePoly2D;

class PolyPoly2D
{
public:
    static bool PointInPolyCN(const Vector2<float>& rPoint, const BasePoly2D& rPoly);

    static int LineSegPolyC(const Vector2<float>& rA0, const Vector2<float>& rA1, const BasePoly2D& rPoly);

    static void UnitTest();
};

void UnitTest();

class Overlap3D
{
public:
    static bool Op(const ExtrudedPoly3D& rPoly, const Vector3<float>& rPoint);
    static bool Op(const ExtrudedPoly3D& rPoly, const Sphere& rSphere);

    static bool Op(const BasePoly3D& rPoly, const Ray<Vector3<float> >& rRay);

    static bool Op(const class Frustum& rFrustum, const Sphere& rSphere);

    static bool Op(const Sphere& rSphere, const Ray<Vector3<float> >& rRay);
    static bool Op(const class Frustum& rFrustum, const BasePoly3D& rPoly);
    static bool Op(const class Frustum& rFrustum, const struct SphereBoundedPoly3D& rPoly);
};

struct Box;

class Intersect3D
{
public:
    static bool Op(const Box& rA, const Box& rB, Box& rOut);

    static bool Op(const struct Plane& rPlane, const Ray<Vector3<float> >& rRay, Vector3<float>& rHit);
};

struct Circle;
class Overlap2D
{
public:
    static bool LineSegLineSeg(const Vector2<float>& rA0, const Vector2<float>& rA1,
                               const Vector2<float>& rB0, const Vector2<float>& rB1);
    static bool Op(const Circle& rCircle, const Ray<Vector2<float> >& rRay);
    static bool Op(const Ray<Vector2<float> >& rA, const Ray<Vector2<float> >& rB);
};

class Classify3D
{
public:
    static unsigned int Op(const class Frustum& rFrustum, const Sphere& rSphere);
};

class SepDistance
{
public:
    static float Op(const Sphere& rA, const Sphere& rB);
};

struct Circle
{
    Circle(const Vector2<float>& rCenter, float fRadius) : center_(rCenter), radius_(fRadius) {}

    Vector2<float> center_;
    float radius_;
};

struct Box
{

    Box();

    Box(const Vector3<float>& rMin, const Vector3<float>& rMax) : min_(rMin), max_(rMax) {}

    void Bound(const Vector3<float>& rPoint);

    float Volume();

    Vector3<float> min_;
    Vector3<float> max_;
};

struct Plane
{

    Plane() : dist_(0.0f) {}

    Plane(const BasePoly3D& rPoly);

    Vector3<float> dir_;
    float dist_;
};
 
 
 
 

class Frustum
{
public:
    Frustum();
    Frustum(const Vector3<float>& rCenter, const Vector3<float>& rUp, const Vector3<float>& rFwd,
            float fWidth, float fHeight, float fNear, float fFar);

    const Plane* GetPlanes() const
    {
        if (!initdone_)
        {
            GetPlanes_(planes_);
            initdone_ = true;
        }
        return planes_;
    }

    void GetPlanes_(Plane* pPlanes) const;
    void GetLeftPlane(Plane& rPlane) const;
    void GetRightPlane(Plane& rPlane) const;
    void GetBottomPlane(Plane& rPlane) const;
    void GetTopPlane(Plane& rPlane) const;

    void GetBaseEdges(Ray<Vector3<float> >* pEdges) const;
    void GetNearPlane(Plane& rPlane) const;
    void GetFarPlane(Plane& rPlane) const;

    Vector3<float> center_;
    Vector3<float> up_;
    Vector3<float> fwd_;
    float width_;
    float height_;
    float near_;
    float far_;
    mutable Plane planes_[6];
    mutable bool initdone_;
};

template <class T>
class Line
{
public:

    Line() : orig_(), dir_() {}
    Line(const T& rOrig, const T& rDir) : orig_(rOrig), dir_(rDir) {}

    T orig_;
    T dir_;
};

template <class T>
class Ray : public Line<T>
{
public:
    Ray() {}
    Ray(const T& rOrig, const T& rDir) : Line<T>(rOrig, rDir) {}
};

// FUNCTION: SWBF2 0x00723810
inline void Frustum::GetPlanes_(Plane* pPlanes) const
{
    GetNearPlane(pPlanes[0]);
    GetFarPlane(pPlanes[1]);
    GetLeftPlane(pPlanes[2]);
    GetRightPlane(pPlanes[3]);
    GetBottomPlane(pPlanes[4]);
    GetTopPlane(pPlanes[5]);
}

// FUNCTION: SWBF2 0x00722eb0
inline void Frustum::GetBaseEdges(Ray<Vector3<float> >* pEdges) const
{
    Vector3<float> right = normalize(cross(up_, fwd_));
    pEdges[0].dir_ = fwd_ * far_ + up_ * height_ + right * width_;
    pEdges[1].dir_ = fwd_ * far_ - up_ * height_ + right * width_;
    pEdges[2].dir_ = fwd_ * far_ - up_ * height_ - right * width_;
    pEdges[3].dir_ = fwd_ * far_ + up_ * height_ - right * width_;
    pEdges[0].orig_ = center_;
    pEdges[1].orig_ = center_;
    pEdges[2].orig_ = center_;
    pEdges[3].orig_ = center_;
}

inline void Frustum::GetNearPlane(Plane& rPlane) const
{
    rPlane.dir_ = fwd_;
    rPlane.dist_ = (dot(rPlane.dir_, center_) + near_) * -1.0f;
}

inline void Frustum::GetFarPlane(Plane& rPlane) const
{
    rPlane.dir_ = fwd_ * -1.0f;
    rPlane.dist_ = (dot(rPlane.dir_, center_) - far_) * -1.0f;
}

// FUNCTION: SWBF2 0x007235c0
inline void Frustum::GetLeftPlane(Plane& rPlane) const
{
    Vector3<float> side = normalize(cross(up_, fwd_)) * width_;
    Vector3<float> edge = normalize(fwd_ * far_ - side);
    rPlane.dir_ = normalize(cross(up_, edge));
    const float fDot = dot(rPlane.dir_, center_);
    rPlane.dist_ = fDot * -1.0f;
}

// FUNCTION: SWBF2 0x00723900
inline void Frustum::GetRightPlane(Plane& rPlane) const
{
    Vector3<float> side = normalize(cross(up_, fwd_));
    Vector3<float> edge = normalize(fwd_ * far_ + side * width_);
    rPlane.dir_ = normalize(cross(edge, up_));
    const float fDot = dot(rPlane.dir_, center_);
    rPlane.dist_ = fDot * -1.0f;
}

// FUNCTION: SWBF2 0x00723240
inline void Frustum::GetBottomPlane(Plane& rPlane) const
{
    Vector3<float> side = normalize(cross(up_, fwd_));
    Vector3<float> edge = normalize(fwd_ * far_ - up_ * height_);
    rPlane.dir_ = normalize(cross(edge, side));
    const float fDot = dot(rPlane.dir_, center_);
    rPlane.dist_ = fDot * -1.0f;
}

// FUNCTION: SWBF2 0x00723b50
inline void Frustum::GetTopPlane(Plane& rPlane) const
{
    Vector3<float> side = normalize(cross(up_, fwd_));
    Vector3<float> edge = normalize(fwd_ * far_ + up_ * height_);
    rPlane.dir_ = normalize(cross(side, edge));
    const float fDot = dot(rPlane.dir_, center_);
    rPlane.dist_ = fDot * -1.0f;
}

}
