#include <string.h>

#include "LuaHelper.h"
#include "LuaFL/include/lualib.h"
#include "LuaFL/include/lauxlib.h"
#include "PebbleFL/Common/PblHashTable.h"
#include "PebbleFL/Common/PblSimOrder.h"
#include "PebbleFL/Common/PblUnicode.h"
#include "LuaScript.h"
#include "LuaEvent.h"
#include "LuaMemory.h"
#include "pcCrash.h"

namespace LuaHelper
{

// GLOBAL: SWBF2 0x01e579a0
// mState
lua_State* mState;

// GLOBAL: SWBF2 0x01e5794b
bool mbInitialized;

// FUNCTION: SWBF2 0x005a0ce0
void InitState()
{
    if (mbInitialized)
    {
        return;
    }

    lua_setinsertionorder(PblGetSimOrder() == PBL_SIMORDER_SERIAL);
    mState = lua_open();
    if (!mState)
    {
        pcCrash::Exit(1);
    }
    lua_atpanic(mState, Lua_Red_Panic);

    luaopen_base(mState);
    luaopen_table(mState);
    luaopen_io(mState);
    luaopen_string(mState);
    luaopen_math(mState);
    luaopen_debug(mState);

    mbInitialized = true;
}

// FUNCTION: SWBF2 0x005a0e70
void Register(char const* pcName, lua_CFunction pfnCallback, int iNumUpvalues)
{
    lua_pushcclosure(mState, pfnCallback, iNumUpvalues);

    lua_setglobal(mState, pcName);
}

}

// GLOBAL: SWBF2 0x01e5799c
static int mStackPos;

namespace LuaHelper
{

// GLOBAL: SWBF2 0x01e57998
unsigned int m_uiOpenScriptNameHash;

// FUNCTION: SWBF2 0x005a0d40
void CleanupState()
{
    if (mbInitialized)
    {
        mbInitialized = false;
        mState = 0;
    }
}

// FUNCTION: SWBF2 0x005a0d60
bool OpenScript(char const* pcName)
{
    if (m_uiOpenScriptNameHash)
    {
        return false;
    }

    m_uiOpenScriptNameHash = PblHash(pcName);
    LuaScriptFactory* pFactory = LuaScriptFactory::Find(m_uiOpenScriptNameHash);
    if (!pFactory)
    {
        return false;
    }

    unsigned int uiLen = pFactory->GetScriptLen();
    const char* pcScript = pFactory->GetScript();
    if (!pcScript)
    {
        m_uiOpenScriptNameHash = 0;
        return false;
    }

    InitState();
    LuaEventManager::SetLuaState(mState);

    char acName[256];
    strlcpy(acName, pcName, sizeof(acName));
    strlcat(acName, ".lua", sizeof(acName));
    lua_dobuffer(mState, pcScript, uiLen, acName);
    pFactory->Purge();

    lua_setgcthreshold(mState, lua_getgccount(mState));
    return true;
}

// FUNCTION: SWBF2 0x005a0e40
bool CallProc(char const* pcName)
{
    return CallGlobalProc(pcName);
}

// FUNCTION: SWBF2 0x005a0e50
bool CloseScript()
{
    if (!m_uiOpenScriptNameHash)
    {
        return false;
    }
    m_uiOpenScriptNameHash = 0;
    return true;
}

// FUNCTION: SWBF2 0x005a0ec0
bool PushRefProc(int iRef)
{
    lua_getref(mState, iRef);
    if (!lua_isfunction(mState, -1))
    {
        lua_pop(mState, 1);
        return false;
    }
    mStackPos = lua_gettop(mState);
    return true;
}

// FUNCTION: SWBF2 0x005a0f10
bool PushGlobalProc(char const* pcFunc)
{
    lua_pushstring(mState, pcFunc);
    lua_gettable(mState, LUA_GLOBALSINDEX);
    if (!lua_isfunction(mState, -1))
    {
        lua_pop(mState, 1);
        return false;
    }
    mStackPos = lua_gettop(mState);
    return true;
}

// FUNCTION: SWBF2 0x005a0f80
bool PushTableProc(char const* pcTable, char const* pcFunc)
{
    lua_pushstring(mState, pcTable);
    lua_gettable(mState, LUA_GLOBALSINDEX);
    if (!lua_istable(mState, -1))
    {
        lua_pop(mState, 1);
        return false;
    }

    lua_pushstring(mState, pcFunc);
    lua_gettable(mState, -2);
    if (!lua_isfunction(mState, -1))
    {
        lua_pop(mState, 2);
        return false;
    }

    lua_remove(mState, -2);
    mStackPos = lua_gettop(mState);
    return true;
}

// FUNCTION: SWBF2 0x005a1030
bool PushTableProcWThis(char const* pcTable, char const* pcFunc)
{
    if (!PushTableProc(pcTable, pcFunc))
    {
        return false;
    }
    lua_pushstring(mState, pcTable);
    lua_gettable(mState, LUA_GLOBALSINDEX);
    return true;
}

// FUNCTION: SWBF2 0x005a1070
int CallProc(int aReturns)
{
    int aNumArgs = GetTop() - mStackPos;
    int iResult = lua_pcall(mState, aNumArgs, aReturns, 0);
    if (iResult)
    {
        lua_pop(mState, 1);
    }
    mStackPos = 0;
    return iResult;
}

// FUNCTION: SWBF2 0x005a10f0
bool CallGlobalProc(char const* pcFunc)
{
    if (!PushGlobalProc(pcFunc))
    {
        return false;
    }
    CallProc(0);
    return true;
}

// FUNCTION: SWBF2 0x005a1140
bool CallGlobalProc(char const* pcFunc, float fArg)
{
    if (!PushGlobalProc(pcFunc))
    {
        return false;
    }
    PushFloat(fArg);
    CallProc(0);
    return true;
}

// FUNCTION: SWBF2 0x005a11b0
bool CallTableProcWThis(char const* pcTable, char const* pcFunc)
{
    if (!PushTableProcWThis(pcTable, pcFunc))
    {
        return false;
    }
    CallProc(0);
    return true;
}

// FUNCTION: SWBF2 0x005a1210
bool CallTableProcWThis(char const* pcTable, char const* pcFunc, unsigned short const* pusArg)
{
    if (!PushTableProcWThis(pcTable, pcFunc))
    {
        return false;
    }
    PushString(pusArg);
    CallProc(0);
    return true;
}

// FUNCTION: SWBF2 0x005a1290
void TableReadVal(char const* pcKey, float& fVal, bool aNilOk)
{
    PushString(pcKey);
    RawGetFromTable(-2);
    if (IsNumber(-1))
    {
        fVal = GetFloat(-1);
    }
    else
    {
        fVal = 0.0f;
        if (aNilOk)
        {
            IsNil(-1);
        }
    }
    Pop(1);
}

// FUNCTION: SWBF2 0x005a1300
void TableReadVal(char const* pcKey, int& iVal, bool aNilOk)
{
    PushString(pcKey);
    RawGetFromTable(-2);
    if (IsNumber(-1))
    {
        iVal = LuaHelper::GetInt(-1);
    }
    else
    {
        iVal = 0;
        if (aNilOk)
        {
            IsNil(-1);
        }
    }
    Pop(1);
}

// FUNCTION: SWBF2 0x005a13e0
void TableReadVal(char const* pcKey, char* pcVal, unsigned int uSize, bool aNilOk)
{
    PushString(pcKey);
    RawGetFromTable(-2);
    memset(pcVal, 0, uSize);
    if (IsString(-1))
    {
        const char* Str = GetString(-1);
        strlcpy(pcVal, Str, uSize);
    }
    else
    {
        if (aNilOk)
        {
            IsNil(-1);
        }
    }
    Pop(1);
}

// FUNCTION: SWBF2 0x005a1480
void TableWriteVal(char const* pcKey, float fVal)
{
    PushString(pcKey);
    PushFloat(fVal);
    RawSetToTable(-3);
}

// FUNCTION: SWBF2 0x005a14d0
void TableWriteVal(char const* pcKey, int iVal)
{
    PushString(pcKey);
    PushInt(iVal);
    RawSetToTable(-3);
}

// FUNCTION: SWBF2 0x005a1570
void TableWriteVal(char const* pcKey, unsigned short const* pusVal)
{
    PushString(pcKey);
    PushString(pusVal);
    RawSetToTable(-3);
}

// FUNCTION: SWBF2 0x005a1540
void TableWriteVal(char const* pcKey, char const* pcVal)
{
    PushString(pcKey);
    PushString(pcVal);
    RawSetToTable(-3);
}

// FUNCTION: SWBF2 0x005a1390
void TableReadVal(char const* pcKey, bool& bVal)
{
    PushString(pcKey);
    RawGetFromTable(-2);
    bVal = GetBool(-1);
    Pop(1);
}

// FUNCTION: SWBF2 0x005a15a0
void TableWriteNil(char const* pcKey)
{
    PushString(pcKey);
    PushNil();
    RawSetToTable(-3);
}

}

// FUNCTION: SWBF2 0x005a10c0
void LuaHelper::PushString(unsigned short const* pusString)
{
    lua_pushlstring(mState, (const char*)pusString, PblUnicode::ustrlen(pusString) * 2 + 2);
}

namespace LuaHelper
{

// FUNCTION: SWBF2 0x005a1110
bool CallGlobalProc(char const* pcFunc, bool bArg)
{
    if (!PushGlobalProc(pcFunc))
    {
        return false;
    }
    PushBool(bArg);
    CallProc(0);
    return true;
}
}

namespace LuaHelper
{

// FUNCTION: SWBF2 0x005a1180
bool CallGlobalProc(char const* pcFunc, unsigned short const* pusArg)
{
    if (!PushGlobalProc(pcFunc))
    {
        return false;
    }
    PushString(pusArg);
    CallProc(0);
    return true;
}
}

// FUNCTION: SWBF2 0x005a11d0
bool LuaHelper::CallTableProcWThis(char const* pcTable, char const* pcFunc, float fArg)
{
    if (!PushTableProcWThis(pcTable, pcFunc))
    {
        return false;
    }
    PushFloat(fArg);
    CallProc(0);
    return true;
}

// FUNCTION: SWBF2 0x005a1230
bool LuaHelper::CallTableProcWThis(char const* pcTable, char const* pcFunc, float fArg1, float fArg2)
{
    if (!PushTableProcWThis(pcTable, pcFunc))
    {
        return false;
    }
    PushFloat(fArg1);
    PushFloat(fArg2);
    CallProc(0);
    return true;
}

namespace LuaHelper
{

// FUNCTION: SWBF2 0x005a1510
void TableWriteVal(char const* pcKey, bool bVal)
{
    PushString(pcKey);
    PushBool(bVal);
    RawSetToTable(-3);
}
}
