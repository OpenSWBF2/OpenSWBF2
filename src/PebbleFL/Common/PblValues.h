#ifndef PBLVALUES_H
#define PBLVALUES_H

enum PblValuesType
{
    PVT_FULL = 0,
    PVT_MOVES_LOW = 1,
    PVT_MOVES_HIGH = 2
};

class PblValues
{
public:

    template <class T>
    static void Record(const T& value, PblValuesType type);
};

// TEMPLATE: SWBF2 0x004fab50
// PblValues::Record<bool>
// TEMPLATE: SWBF2 0x00453680
// PblValues::Record<int>
// TEMPLATE: SWBF2 0x005d3bf0
// PblValues::Record<unsigned int>
// TEMPLATE: SWBF2 0x00458950
// PblValues::Record<char>
// TEMPLATE: SWBF2 0x0043cd30
// PblValues::Record<float>
template <class T>
inline void PblValues::Record(const T& value, PblValuesType type)
{
    (void)value;
    (void)type;
}

#endif
