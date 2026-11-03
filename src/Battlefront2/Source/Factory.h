#pragma once

#include "PebbleFL/Common/PblList.h"

class Entity;
class EntityClass;
struct EntityDesc;
class Ordnance;
class OrdnanceClass;
class ExplosionClass;
struct OrdnanceDesc;
class Weapon;
class WeaponClass;
struct WeaponDesc;

template <class TYPE, class CLASS, class DESC>
class Factory
{
public:
    Factory(unsigned int uHash);
    Factory(CLASS* pParent, unsigned int uHash);

    virtual ~Factory()
    {
        if (mNode._pList)
        {
            sList.Remove(&mNode);
        }
    }

    virtual CLASS* Derive(unsigned int uHash) = 0;
    virtual TYPE* Build(const DESC& desc) = 0;

    static CLASS* Find(unsigned int uHash);
    static bool Contains(const CLASS* pClass);
    static void Reset();

    static CLASS* FindNetIndex(int iNetIndex);
    unsigned int GetId() const;
    unsigned int GetNetIndex() const { return mNetIndex; }

    typename PblList<Factory<TYPE, CLASS, DESC> >::Node mNode;
    CLASS* mParent;
    unsigned int mId;
protected:
    unsigned int mNetIndex;

public:

    static PblList<Factory<TYPE, CLASS, DESC> > sList;
    static unsigned int sCounter;
};

template <class TYPE, class CLASS, class DESC>
Factory<TYPE, CLASS, DESC>::Factory(unsigned int uHash)
{
    mParent = 0;
    mId = uHash;
    mNetIndex = sCounter++;
    sList.AddLast(&mNode, this);
}

template <class TYPE, class CLASS, class DESC>
// GLOBAL: SWBF2 0x007ec55c
// Factory<Entity,EntityClass,EntityDesc>::sList
// GLOBAL: SWBF2 0x007ec570
// Factory<Ordnance,ExplosionClass,OrdnanceDesc>::sList
// GLOBAL: SWBF2 0x007ec590
// Factory<Ordnance,OrdnanceClass,OrdnanceDesc>::sList
// GLOBAL: SWBF2 0x007ec5a4
// Factory<Weapon,WeaponClass,WeaponDesc>::sList
PblList<Factory<TYPE, CLASS, DESC> > Factory<TYPE, CLASS, DESC>::sList;

template <class TYPE, class CLASS, class DESC>
// GLOBAL: SWBF2 0x01faa458
// Factory<Entity,EntityClass,EntityDesc>::sCounter
// GLOBAL: SWBF2 0x01faa470
// Factory<Ordnance,ExplosionClass,OrdnanceDesc>::sCounter
// GLOBAL: SWBF2 0x01faa728
// Factory<Ordnance,OrdnanceClass,OrdnanceDesc>::sCounter
// GLOBAL: SWBF2 0x01faa758
// Factory<Weapon,WeaponClass,WeaponDesc>::sCounter
unsigned int Factory<TYPE, CLASS, DESC>::sCounter;

template <class TYPE, class CLASS, class DESC>
inline void Factory<TYPE, CLASS, DESC>::Reset()
{
    sCounter = 0;
    sList.RemoveAll();
}

template <class TYPE, class CLASS, class DESC>
Factory<TYPE, CLASS, DESC>::Factory(CLASS* pParent, unsigned int uHash)
{
    mParent = pParent;
    mId = uHash;
    mNetIndex = sCounter++;
    sList.AddLast(&mNode, this);
}

template <class TYPE, class CLASS, class DESC>
inline CLASS* Factory<TYPE, CLASS, DESC>::Find(unsigned int uHash)
{
    for (typename PblList<Factory<TYPE, CLASS, DESC> >::Itor it(sList.GetFirst()); it; ++it)
    {
        if (it->mId == uHash)
        {
            return static_cast<CLASS*>(&*it);
        }
    }
    return 0;
}

template <class TYPE, class CLASS, class DESC>
bool Factory<TYPE, CLASS, DESC>::Contains(const CLASS* pClass)
{
    if (!pClass)
    {
        return false;
    }
    for (typename PblList<Factory<TYPE, CLASS, DESC> >::Itor it(sList); it; ++it)
    {
        if (static_cast<CLASS*>((Factory<TYPE, CLASS, DESC>*)it) == pClass)
        {
            return true;
        }
    }
    return false;
}

// VTABLE: SWBF2 0x00799318
// class Factory<Entity, class EntityClass, struct EntityDesc>



// TEMPLATE: SWBF2 0x00491e70 SYMBOL
// ??0?$Factory@VEntity@@VEntityClass@@UEntityDesc@@@@QAE@I@Z
template Factory<Entity, EntityClass, EntityDesc>::Factory(unsigned int);

// TEMPLATE: SWBF2 0x00491ef0 SYMBOL
// ??0?$Factory@VEntity@@VEntityClass@@UEntityDesc@@@@QAE@PAVEntityClass@@I@Z
template Factory<Entity, EntityClass, EntityDesc>::Factory(EntityClass*, unsigned int);

// VTABLE: SWBF2 0x007ac47c
// class Factory<Ordnance, class OrdnanceClass, struct OrdnanceDesc>



// TEMPLATE: SWBF2 0x005f8940
// Factory<Ordnance,OrdnanceClass,OrdnanceDesc>::Factory<Ordnance,OrdnanceClass,OrdnanceDesc>
template Factory<Ordnance, OrdnanceClass, OrdnanceDesc>::Factory(unsigned int);

// VTABLE: SWBF2 0x007b0298
// class Factory<Weapon, class WeaponClass, struct WeaponDesc>



// TEMPLATE: SWBF2 0x0067bfe0 SYMBOL
// ??0?$Factory@VWeapon@@VWeaponClass@@UWeaponDesc@@@@QAE@I@Z
template Factory<Weapon, WeaponClass, WeaponDesc>::Factory(unsigned int);

// TEMPLATE: SWBF2 0x0067c060 SYMBOL
// ??0?$Factory@VWeapon@@VWeaponClass@@UWeaponDesc@@@@QAE@PAVWeaponClass@@I@Z
template Factory<Weapon, WeaponClass, WeaponDesc>::Factory(WeaponClass*, unsigned int);

// VTABLE: SWBF2 0x0079e8ac
// class Factory<Ordnance, class ExplosionClass, struct OrdnanceDesc>



// TEMPLATE: SWBF2 0x0051eb20
// Factory<Ordnance,ExplosionClass,OrdnanceDesc>::Factory<Ordnance,ExplosionClass,OrdnanceDesc>
template Factory<Ordnance, ExplosionClass, OrdnanceDesc>::Factory(unsigned int);

// TEMPLATE: SWBF2 0x00491f70
// Factory<Entity,EntityClass,EntityDesc>::~Factory<Entity,EntityClass,EntityDesc>

// TEMPLATE: SWBF2 0x0051ec20
// Factory<Ordnance,ExplosionClass,OrdnanceDesc>::~Factory<Ordnance,ExplosionClass,OrdnanceDesc>

// TEMPLATE: SWBF2 0x005f8a40
// Factory<Ordnance,OrdnanceClass,OrdnanceDesc>::~Factory<Ordnance,OrdnanceClass,OrdnanceDesc>

// TEMPLATE: SWBF2 0x0067c0e0
// Factory<Weapon,WeaponClass,WeaponDesc>::~Factory<Weapon,WeaponClass,WeaponDesc>
