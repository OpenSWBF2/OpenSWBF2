#pragma once

#include "PebbleFL/Common/PblMatrix.h"
#include "vector.h"

namespace geo
{

struct BasePoly2D;
struct BasePoly3D;

class ProjClip
{
public:
    static void Op(const BasePoly3D& rIn, BasePoly3D& rOut);
};

class Transform
{
public:
    static void Op(const BasePoly3D& rIn, BasePoly3D& rOut, const PblMatrix& rTrans);

    static void Op(const BasePoly3D& rIn, BasePoly2D& rOut, const Vector3<float>& rX, const Vector3<float>& rY);
};

}
