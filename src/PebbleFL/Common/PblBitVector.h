#pragma once

#include <string.h>

template <int N>
class PblBitVector
{
public:
    enum { NUM_UINTS = (N + 31) / 32 };

    PblBitVector() {}

    PblBitVector(const PblBitVector& v)
    {
        *this = v;
    }

    void ClearAll()
    {
        memset(m_auiData, 0, sizeof(m_auiData));
    }

    void Or(const PblBitVector& rOther)
    {
        for (unsigned int i = 0; i < NUM_UINTS; ++i)
        {
            m_auiData[i] |= rOther.m_auiData[i];
        }
    }

    void SetAll()
    {
        for (unsigned int i = 0; i < NUM_UINTS; ++i)
        {
            m_auiData[i] = 0xffffffff;
        }
    }
    void ClearBit(unsigned int uBit)
    {
        m_auiData[NUM_UINTS == 1 ? 0 : (uBit >> 5)] &= ~(1 << (NUM_UINTS == 1 ? uBit : (uBit & 31)));
    }

    void SetBit(unsigned int uBit)
    {
        m_auiData[NUM_UINTS == 1 ? 0 : (uBit >> 5)] |= 1 << (NUM_UINTS == 1 ? uBit : (uBit & 31));
    }
    void InvertAll()
    {
        for (unsigned int i = 0; i < N / 32; ++i)
        {
            m_auiData[i] = ~m_auiData[i];
        }
        if (N % 32)
        {
            m_auiData[N / 32] ^= (1u << (N % 32)) - 1;
        }
    }
    void And(const PblBitVector& rOther)
    {
        for (unsigned int i = 0; i < NUM_UINTS; ++i)
        {
            m_auiData[i] &= rOther.m_auiData[i];
        }
    }
    const PblBitVector operator~() const
    {
        PblBitVector result(*this);
        result.InvertAll();
        return result;
    }
    bool IsBitSet(unsigned int uBit) const
    {
        return (m_auiData[NUM_UINTS == 1 ? 0 : (uBit >> 5)] & (1 << (NUM_UINTS == 1 ? uBit : (uBit & 31)))) != 0;
    }

    unsigned int m_auiData[NUM_UINTS];
};
