#include "CmdInfo.h"
#include "CommandPost.h"
#include "GameObject.h"





// FUNCTION: SWBF2 0x0045cb40
CmdInfo::CmdInfo()
{
    mWhat = 0;
    mWho = 0;
    mOwner = 0;
    mPos = PblVector3(0, 0, 0);
    mWhoCommandPost = 0;
    mParam = 0;
    mFlag = 0;
}

// FUNCTION: SWBF2 0x0045cbe0
CmdInfo::CmdInfo(int iWhat, PblHandle<GameObject> who, const PblVector3& pos, PblHandle<GameObject> owner)
{
    mWhat = iWhat;
    mWho = who;
    mOwner = owner;
    mWhoCommandPost = 0;
    mPos = pos;
    mParam = 0;
    mChr = 0;
    mFlag = 0;
}

// FUNCTION: SWBF2 0x0045cc70
CmdInfo::CmdInfo(int iWhat, ConstCommandPostHandle post)
{
    mWho = 0;
    mOwner = 0;
    mWhat = iWhat;
    mPos = PblVector3(0, 0, 0);
    mParam = 0;
    mChr = 0;
    mFlag = 0;
    mWhoCommandPost = const_cast<CommandPost*>((const CommandPost*)post);
}

// FUNCTION: SWBF2 0x0045cd40
CmdInfo::CmdInfo(const CmdInfo& other)
{
    mWhat = other.mWhat;
    mWho = other.mWho;
    mOwner = other.mOwner;
    mWhoCommandPost = other.mWhoCommandPost;
    mPos = other.mPos;
    mParam = other.mParam;
    mChr = other.mChr;
    mFlag = other.mFlag;
}

// FUNCTION: SWBF2 0x0045cdd0
CmdInfo::~CmdInfo()
{
}

// FUNCTION: SWBF2 0x0045cde0
CmdInfo& CmdInfo::operator=(const CmdInfo& other)
{
    mWhat = other.mWhat;
    mWho = other.mWho;
    mOwner = other.mOwner;
    mWhoCommandPost = other.mWhoCommandPost;
    mPos = other.mPos;
    mParam = other.mParam;
    mChr = other.mChr;
    mFlag = other.mFlag;
    return *this;
}
