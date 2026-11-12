#ifndef SWBF2_REDENGINEFL_GRAPHICS_REDMATERIAL_H
#define SWBF2_REDENGINEFL_GRAPHICS_REDMATERIAL_H

#include <stddef.h>

#include "RedColor.h"
#include "RedTextureTransform.h"

struct RedMaterialAnimationData;

 
 
 
 
 

class RedLight;

struct RedMaterialAnimationData
{
    RedColorValue diffuseColor;
    unsigned int attachedLightName;
    RedLight* attachedLight;
    unsigned char scrollRateU;
    unsigned char scrollRateV;
    unsigned char minIntensity;
    unsigned char blinkRate;
    unsigned char numRows;
    unsigned char numCols;
    unsigned char frameRate;
};
 

struct RedMaterial
{
    unsigned int diffuseMapHash;
    unsigned int detailMapHash;
    unsigned int bumpMapHash;
    unsigned int envMapHash;
    unsigned int materialFlags;
    RedColorValue diffuseColor;
    RedColorValue specularColor;
    float specularPower;
    RedTextureTransform diffuseTexTransform;
    RedTextureTransform detailTexTransform;
    RedMaterialAnimationData* animationData;
};
 
 
 
 
 
 
 
 

#endif
