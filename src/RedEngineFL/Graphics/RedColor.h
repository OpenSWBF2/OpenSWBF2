#pragma once

#include <d3d9.h>

class RedColor
{
public:

    // FUNCTION: SWBF2 0x004108a0
    RedColor() {}

    // FUNCTION: SWBF2 0x0040f750
    RedColor(const RedColor& c) : b(c.b), g(c.g), r(c.r), a(c.a) {}
    RedColor(const RedColor& color, unsigned char aA)
        : b(color.b), g(color.g), r(color.r), a(aA) {}
    RedColor(unsigned char aR, unsigned char aG, unsigned char aB, unsigned char aA)
        : b(aB), g(aG), r(aR), a(aA) {}

    RedColor& Set(unsigned char aR, unsigned char aG, unsigned char aB, unsigned char aA)
    {
        r = aR;
        g = aG;
        b = aB;
        a = aA;
        return *this;
    }

    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;

    // FUNCTION: SWBF2 0x0041bbe0
    unsigned int GetARGB() const
    {
        unsigned int argb = (static_cast<unsigned int>(a) << 8) | r;
        return (((argb << 8) | g) << 8) | b;
    }

    operator unsigned int() const { return GetARGB(); }

    static RedColor WHITE;
    static RedColor LTGREY;
    static RedColor MDGREY;
    static RedColor DKGREY;
    static RedColor BLACK;
    static RedColor RED;
    static RedColor GREEN;
    static RedColor BLUE;
    static RedColor CYAN;
    static RedColor MAGENTA;
    static RedColor YELLOW;

    // FUNCTION: SWBF2 0x00525ee0
    const RedColor operator*(const RedColor& c) const
    {
        RedColor result = *this;
        result *= c;
        return result;
    }

    // FUNCTION: SWBF2 0x00492000
    const RedColor& operator*=(const RedColor& c)
    {
        r = (unsigned char)(int)(r * (1.0f / 255.0f) * c.r + 0.5f);
        g = (unsigned char)(int)(g * (1.0f / 255.0f) * c.g + 0.5f);
        b = (unsigned char)(int)(b * (1.0f / 255.0f) * c.b + 0.5f);
        a = (unsigned char)(int)(a * (1.0f / 255.0f) * c.a + 0.5f);
        return *this;
    }
};

// FUNCTION: SWBF2 0x004bd820
inline const RedColor Interpolate(const RedColor& a, const RedColor& b, float t)
{
    RedColor result;
    result.b = (unsigned char)(int)(a.b * (1.0f - t) + b.b * t + 0.5f);
    result.g = (unsigned char)(int)(a.g * (1.0f - t) + b.g * t + 0.5f);
    result.r = (unsigned char)(int)(a.r * (1.0f - t) + b.r * t + 0.5f);
    result.a = (unsigned char)(int)(a.a * (1.0f - t) + b.a * t + 0.5f);
    return result;
}

class RedColorValue
{
public:

    // FUNCTION: SWBF2 0x0048ebf0
    RedColorValue() {}
    explicit RedColorValue(float fValue) : r(fValue), g(fValue), b(fValue), a(fValue) {}
    RedColorValue(float fR, float fG, float fB, float fA) : r(fR), g(fG), b(fB), a(fA) {}
    RedColorValue(const RedColorValue& c) : r(c.r), g(c.g), b(c.b), a(c.a) {}
    RedColorValue(const RedColorValue& color, float aA)
        : r(color.r), g(color.g), b(color.b), a(color.a * aA) {}

    RedColorValue(const RedColor& color);

    RedColorValue operator+(const RedColorValue& color) const
    {
        return RedColorValue(r + color.r, g + color.g, b + color.b, a + color.a);
    }

    RedColorValue operator*(float scale) const
    {
        return RedColorValue(r * scale, g * scale, b * scale, a * scale);
    }

    const RedColorValue& operator*=(const RedColorValue& color)
    {
        r *= color.r;
        g *= color.g;
        b *= color.b;
        a *= color.a;
        return *this;
    }

    operator RedColor() const
    {
        return RedColor((unsigned char)(int)(r * 255.0f),
                        (unsigned char)(int)(g * 255.0f),
                        (unsigned char)(int)(b * 255.0f),
                        (unsigned char)(int)(a * 255.0f));
    }

    operator D3DCOLORVALUE() const
    {
        return *reinterpret_cast<const D3DCOLORVALUE*>(this);
    }

    float r;
    float g;
    float b;
    float a;

    static RedColorValue WHITE;

    static RedColorValue BLACK;
};

// FUNCTION: SWBF2 0x004bd900
inline RedColorValue::RedColorValue(const RedColor& color)
{
    const float kInv255 = 1.0f / 255.0f;
    r = color.r * kInv255;
    g = color.g * kInv255;
    b = color.b * kInv255;
    a = color.a * kInv255;
}

 
