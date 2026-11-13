#pragma once

enum CollisionObjectType
{
    COLL_UNKNOWN = 0,
    COLL_ITEM = 1,
    COLL_SOFT = 2,
    COLL_ASTEROID = 4,
    COLL_RIGID = 8,
    COLL_STATIC = 16,
    COLL_ATTACHED = 32,
    COLL_SPECIAL = 64,
    COLL_TERRAIN = 128,
    COLL_WATER = 256,
    COLL_LEAFPATCH = 512,
    COLL_ORDNANCE = 1024,
    COLL_TOWCABLE = 2048,
    COLL_SHIELD = 4096,
    COLL_TARGETABLE = 8192,
    COLL_HOLE = 16384
};
