#pragma once

class PblHash
{
public:
    PblHash(const char* pcText);
    PblHash(unsigned int uiSeed, const char* pcText);

    // FUNCTION: SWBF2 0x00411c40
    operator unsigned int() const { return _uiValue; }

    unsigned int _uiValue;

private:
    static unsigned int __cdecl _MakeHash(const char* pcText);
    static unsigned int __cdecl _MakeHash(const char* pcText, unsigned int uiSeed);
};
 

class PblHashTableCode
{
protected:
    static void __cdecl _Clear(unsigned int* pTable, int iCount);
    static void* __cdecl _Find(const unsigned int* pTable, int iCount, unsigned int uiHash);
    static bool __cdecl _Store(unsigned int* pTable, int iCount, unsigned int uiHash, void* pValue);
    static bool __cdecl _Remove(unsigned int* pTable, int iCount, unsigned int uiHash);
};
 

template <class TYPE, int SLOTS>
class PblHashTable : public PblHashTableCode
{
public:

    PblHashTable() { Clear(); }

    void Clear() { _Clear(&_uiTable[0], SLOTS); _iNumEntries = 0; }
    TYPE* Find(unsigned int uiHash) const { return (TYPE*)_Find(&_uiTable[0], 2 * SLOTS, uiHash); }

    void Store(unsigned int uiHash, TYPE* pValue)
    {
        if (_Store(&_uiTable[0], 2 * SLOTS, uiHash, (void*)pValue))
        {
            ++_iNumEntries;
        }
    }
    void Remove(unsigned int uiHash)
    {
        if (_Remove(&_uiTable[0], 2 * SLOTS, uiHash))
        {
            --_iNumEntries;
        }
    }

    int GetNumEntries() const { return _iNumEntries; }
    int GetMaxEntries() const { return SLOTS; }

    static int GetSlotCount() { return SLOTS; }

    bool HasValue(const TYPE* pValue) const
    {
        if (!pValue)
        {
            return false;
        }
        for (int i = 0; i < SLOTS; ++i)
        {
            if (_table[i] && (const TYPE*)_table[SLOTS + i] == pValue)
            {
                return true;
            }
        }
        return false;
    }

    class Itor
    {
    public:
        Itor() : _table(0), _pos(0) {}
        Itor(const PblHashTable* pTable) : _table(pTable), _pos(0)
        {
            while (_pos < SLOTS && !_table->_uiTable[_pos])
            {
                ++_pos;
            }
        }

        operator TYPE*() const { return (TYPE*)_table->_uiTable[SLOTS + _pos]; }
        TYPE& operator*() const { return *(TYPE*)_table->_uiTable[SLOTS + _pos]; }

        unsigned int GetKey() const { return _table->_uiTable[_pos]; }

        const Itor& operator++()
        {
            ++_pos;
            while (_pos < SLOTS && !_table->_uiTable[_pos])
            {
                ++_pos;
            }
            return *this;
        }

        Itor(const PblHashTable* pTable, unsigned int uiPos)
            : _table(pTable), _pos(uiPos) {}

    private:
        const PblHashTable* _table;
        unsigned int _pos;
    };

    Itor Begin() const { return Itor(this); }
    Itor End() const { return Itor(this, SLOTS); }

private:
    int _iNumEntries;
    unsigned int _uiTable[2 * SLOTS];
};

unsigned int __cdecl PblTEMPHash(const char* pcText);
unsigned int __cdecl CalcCRC(const void* pData, unsigned int uiSize, unsigned int uiCRC);
