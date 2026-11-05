#pragma once

#include <math.h>
#include <cmath>

static_assert(sizeof(sin(1.0f)) == 4, "float math overloads");

namespace geo
{

template <class T>
class Vector2
{
public:
    Vector2() : x(0), y(0) {}

    Vector2(T ux, T uy) : x(ux), y(uy) {}

    T x;
    T y;
};

template <class T>
class Vector3
{
public:
    Vector3() : x(0), y(0), z(0) {}

    template <class U>
    Vector3(U ux, U uy, U uz) : x((T)ux), y((T)uy), z((T)uz) {}

    const Vector3& operator+=(const Vector3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    const Vector3& operator*=(T value)
    {
        x *= value;
        y *= value;
        z *= value;
        return *this;
    }

    T x;
    T y;
    T z;
};

template <class T>
inline const Vector2<T> operator-(const Vector2<T>& a, const Vector2<T>& b)
{
    return Vector2<T>(a.x - b.x, a.y - b.y);
}

template <class T>
inline T cross(const Vector2<T>& a, const Vector2<T>& b)
{
    return a.x * b.y - a.y * b.x;
}

template <class T>
inline const Vector3<T> operator+(const Vector3<T>& a, const Vector3<T>& b)
{
    return Vector3<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}

template <class T>
inline const Vector3<T> operator-(const Vector3<T>& a, const Vector3<T>& b)
{
    return Vector3<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template <class T>
inline const Vector3<T> operator*(const Vector3<T>& v, T f)
{
    return Vector3<T>(v.x * f, v.y * f, v.z * f);
}

template <class T>
inline T dot(const Vector3<T>& a, const Vector3<T>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <class T>
inline Vector3<T> cross(const Vector3<T>& a, const Vector3<T>& b)
{
    return Vector3<T>(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

template <class T>
inline T length(const Vector3<T>& v)
{
    return (T)sqrt((double)dot(v, v));
}

template <class T>
inline Vector3<T> normalize(const Vector3<T>& v)
{
    return v * ((T)1 / length(v));
}

}
