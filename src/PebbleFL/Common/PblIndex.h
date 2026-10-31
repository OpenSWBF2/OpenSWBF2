#pragma once

template <typename T, unsigned int N>
inline bool PblInBounds(const T (&)[N], int iIndex)
{
    return (unsigned int)iIndex < N;
}
