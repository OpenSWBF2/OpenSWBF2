#pragma once

#include <stddef.h>
#include "RedEngineFL/Util/RedSceneObject.h"

class OrdnanceLodData : public RedLodData
{
public:
    OrdnanceLodData();
};
 

class NoLodData : public RedLodData
{
public:
    NoLodData();
};
 

class SmallEffectLodData : public RedLodData
{
public:
    SmallEffectLodData();
};
class MediumEffectLodData : public RedLodData
{
public:
    MediumEffectLodData();
};
class LargeEffectLodData : public RedLodData
{
public:
    LargeEffectLodData();
};
extern SmallEffectLodData smallEffectLodData;
extern MediumEffectLodData mediumEffectLodData;
extern LargeEffectLodData largeEffectLodData;
 

class RedModel;

class ModelLodData : public RedLodData
{
public:
    ModelLodData(int iLodType, const RedModel* pLod0, const RedModel* pLod1,
                 const RedModel* pLod2, const RedModel* pLod3);
};
 

class SoldierLodData : public RedLodData
{
public:
    SoldierLodData(const RedModel* pModel, const RedModel* pLowResModel);
};
