#pragma once

#include "Allocator.h"
#include "hashtable.h"

namespace util
{

template <class ALLOC>
class VariableSet
{
public:

    int& GetInt(const char* name)
    {
        int*& val = int32Map_[name];
        if (!val)
        {
            Buffer buf(0, sizeof(int));
            ALLOC::Allocate(buf);
            val = (int*)buf.data_;
        }
        return *val;
    }

    static void* operator new(unsigned int uiSize, IAllocator& rAlloc)
    {
        Buffer buf(0, uiSize);
        rAlloc.Allocate(buf);
        if (!buf.data_)
        {
            *(volatile char*)buf.data_ = 4;
        }
        return buf.data_;
    }

    // TEMPLATE: SWBF2 0x00715250
    // util::VariableSet<util::SingletonAllocator<util::DefaultStaticAllocator> >::operator delete
    static void operator delete(void*, IAllocator& rAlloc)
    {
        Buffer buf(0, sizeof(VariableSet));
        rAlloc.Deallocate(buf);
    }

    void ResetAll()
    {
        for (typename HashTable<int*, const char*, ALLOC>::Itor i = int32Map_.Begin();
             i != int32Map_.End(); ++i)
        {
            **i = 0;
        }
        for (typename HashTable<float*, const char*, ALLOC>::Itor i = floatMap_.Begin();
             i != floatMap_.End(); ++i)
        {
            **i = 0.0f;
        }
        for (typename HashTable<bool*, const char*, ALLOC>::Itor i = boolMap_.Begin();
             i != boolMap_.End(); ++i)
        {
            **i = false;
        }
    }

    HashTable<int*, const char*, ALLOC>   int32Map_;
    HashTable<float*, const char*, ALLOC> floatMap_;
    HashTable<bool*, const char*, ALLOC>  boolMap_;
};

class Globals
{
public:
    Globals();
    ~Globals();
    static Globals& GetInstance();
    VariableSet<SingletonAllocator<DefaultStaticAllocator> >& GetSet(const char* pcName);

    HashTable<VariableSet<SingletonAllocator<DefaultStaticAllocator> >*, const char*,
              SingletonAllocator<DefaultStaticAllocator> > vsetMap_;
};

}

 
 

namespace util
{
extern util::Globals globals;
}
