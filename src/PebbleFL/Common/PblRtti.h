#pragma once

#include "PblHashTable.h"

#define PBL_DECLARE_DERIVED_RTTI(Class, Base) \
private: \
    static PblHash rttiHash##Class; \
    static const char rttiName##Class[]; \
 \
public: \
    virtual bool IsRtti(unsigned int uRtti) const \
    { \
        if (rttiHash##Class == uRtti) \
        { \
            return true; \
        } \
        return Base::IsRtti(uRtti); \
    } \
 \
    static unsigned int GetRtti() { return rttiHash##Class; } \
    static const char* GetRttiName() { return rttiName##Class; } \
    virtual unsigned int GetDerivedRtti() const { return rttiHash##Class; } \
    virtual const char* GetDerivedRttiName() const { return rttiName##Class; }

#define PBL_DECLARE_ROOT_RTTI(Class) \
private: \
    static PblHash rttiHash##Class; \
    static const char rttiName##Class[]; \
 \
public: \
    virtual bool IsRtti(unsigned int uRtti) const { return rttiHash##Class == uRtti; } \
    static unsigned int GetRtti() { return rttiHash##Class; } \
    static const char* GetRttiName() { return rttiName##Class; } \
    virtual unsigned int GetDerivedRtti() const { return rttiHash##Class; } \
    virtual const char* GetDerivedRttiName() const { return rttiName##Class; }
