#include "Primitives.h"
#include "basics.h"
#include "polygons.h"

#include <math.h>

namespace geo
{
namespace
{

// FUNCTION: SWBF2 0x00722d70
Vector3<float> Convert_(const Vector3<float>& rLocal, const Vector3<float>& rX,
                        const Vector3<float>& rY, const Vector3<float>& rZ)
{
    return rX * rLocal.x + rY * rLocal.y + rZ * rLocal.z;
}

}

// FUNCTION: SWBF2 0x00722880
Sphere::Sphere(const BasePoly3D& rPoly)
{
    center_ = Vector3<float>(0, 0, 0);
    for (int i = 0; i < rPoly.numPoints_; ++i)
    {
        center_ += rPoly.points_[i];
    }
    float fInv = 1.0f / (float)rPoly.numPoints_;
    center_ *= fInv;

    for (int i = 0; i < rPoly.numPoints_; ++i)
    {
        Enclose(rPoly.points_[i]);
    }
}

// FUNCTION: SWBF2 0x007229d0
Sphere::Sphere(const ExtrudedPoly3D& rPoly)
{
    center_ = Vector3<float>(0, 0, 0);
    for (int i = 0; i < rPoly.points_.GetSize(); ++i)
    {
        const Vector2<float>& rPt = rPoly.points_[i];
        Vector3<float> top = Convert_(Vector3<float>(rPt.x, rPt.y, rPoly.max_),
                                      rPoly.xaxis_, rPoly.yaxis_, rPoly.zaxis_);
        Vector3<float> bot = Convert_(Vector3<float>(rPt.x, rPt.y, rPoly.min_),
                                      rPoly.xaxis_, rPoly.yaxis_, rPoly.zaxis_);
        center_ += top + bot;
    }
    center_ *= 0.5f / (float)rPoly.points_.GetSize();

    for (int i = 0; i < rPoly.points_.GetSize(); ++i)
    {
        const Vector2<float>& rPt = rPoly.points_[i];
        Vector3<float> top = Convert_(Vector3<float>(rPt.x, rPt.y, rPoly.max_),
                                      rPoly.xaxis_, rPoly.yaxis_, rPoly.zaxis_);
        Enclose(top);

        Vector3<float> bot = Convert_(Vector3<float>(rPt.x, rPt.y, rPoly.min_),
                                      rPoly.xaxis_, rPoly.yaxis_, rPoly.zaxis_);
        Enclose(bot);
    }
}

// FUNCTION: SWBF2 0x007226c0
Box::Box()
    : min_(), max_()
{
    min_ = Vector3<float>(1e21f, 1e21f, 1e21f);
    max_ = Vector3<float>(-1e21f, -1e21f, -1e21f);
}

// FUNCTION: SWBF2 0x00722730
Plane::Plane(const BasePoly3D& rPoly)
{
    if (!IsDegenerate(rPoly))
    {
        dir_ = normalize(cross(rPoly.points_[0] - rPoly.points_[1], rPoly.points_[2] - rPoly.points_[1]));
        dist_ = -dot(rPoly.points_[0], dir_);
    }
}

// FUNCTION: SWBF2 0x00722e20
float Box::Volume()
{
    Vector3<float> size = max_ - min_;
    float fArea = Max_(size.x, 0.0f) * Max_(size.y, 0.0f);
    return fArea * Max_(size.z, 0.0f);
}

// FUNCTION: SWBF2 0x00722cf0
void Box::Bound(const Vector3<float>& rPoint)
{
    min_.x = Min_(min_.x, rPoint.x);
    max_.x = Max_(max_.x, rPoint.x);
    min_.y = Min_(min_.y, rPoint.y);
    max_.y = Max_(max_.y, rPoint.y);
    min_.z = Min_(min_.z, rPoint.z);
    max_.z = Max_(max_.z, rPoint.z);
}

// FUNCTION: SWBF2 0x00722ca0
void BoundedPoly3D::Bound(const BasePoly3D& rPoly, Box& rBox)
{
    for (int i = 0; i < rPoly.numPoints_; ++i)
    {
        const Vector3<float>& rPoint = rPoly.points_[i];
        if (i == 0)
        {
            rBox.min_ = rBox.max_ = rPoint;
        }
        else
        {
            rBox.Bound(rPoint);
        }
    }
}

}
