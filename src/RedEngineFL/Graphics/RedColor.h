#pragma once

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
        return ((unsigned int)a << 24) | ((unsigned int)r << 16) |
               ((unsigned int)g << 8) | (unsigned int)b;
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

 
