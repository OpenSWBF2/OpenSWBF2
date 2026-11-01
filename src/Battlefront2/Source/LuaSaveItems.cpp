#include "LuaSaveItems.h"

#include <string.h>

#include "LuaFL/include/lauxlib.h"

namespace LuaSaveItems
{

// FUNCTION: SWBF2 0x0058a390
static int FindLuaGlobal(lua_State* L, void const* pValue)
{
    lua_pushnil(L);
    while (lua_next(L, LUA_GLOBALSINDEX))
    {
        if (lua_topointer(L, -1) == pValue)
        {
            lua_settop(L, -2);
            return 1;
        }
        lua_settop(L, -2);
    }
    return 0;
}

static bool WriteBytes(unsigned char*& pWrite, const unsigned char* pEnd, const void* pData, size_t uSize)
{
    if ((size_t)(pEnd - pWrite) < uSize)
    {
        return false;
    }
    memcpy(pWrite, pData, uSize);
    pWrite += uSize;
    return true;
}

// FUNCTION: SWBF2 0x0058a410
static int WriteLuaItem(lua_State* L, unsigned char* pOut, const unsigned char* pEnd, int iIndex)
{
    if (pOut >= pEnd || !lua_checkstack(L, 3))
    {
        return -1;
    }
    lua_pushvalue(L, iIndex);
    const int iType = lua_type(L, -1);
    *pOut = static_cast<unsigned char>(iType);
    unsigned char* pWrite = pOut + 1;
    bool bOk = true;
    switch (iType)
    {
    case LUA_TBOOLEAN:
    {
        const unsigned char uValue = lua_toboolean(L, -1) != 0;
        bOk = WriteBytes(pWrite, pEnd, &uValue, sizeof(uValue));
        break;
    }
    case LUA_TLIGHTUSERDATA:
    {
        void const* pValue = lua_topointer(L, -1);
        bOk = WriteBytes(pWrite, pEnd, &pValue, sizeof(pValue));
        break;
    }
    case LUA_TNUMBER:
    {
        float fValue = static_cast<float>(luaL_checknumber(L, -1));
        bOk = WriteBytes(pWrite, pEnd, &fValue, sizeof(fValue));
        break;
    }
    case LUA_TSTRING:
    {
        const char* pcValue = luaL_checklstring(L, -1, 0);
        bOk = WriteBytes(pWrite, pEnd, pcValue, strlen(pcValue) + 1);
        break;
    }
    case LUA_TTABLE:
        lua_pushnil(L);
        while (lua_next(L, -2))
        {
            const int iKey = WriteLuaItem(L, pWrite, pEnd, -2);
            const int iValue = iKey < 0 ? -1 : WriteLuaItem(L, pWrite + iKey, pEnd, -1);
            if (iValue < 0)
            {
                lua_settop(L, -3);
                bOk = false;
                break;
            }
            pWrite += iKey + iValue;
            lua_settop(L, -2);
        }
        if (bOk)
        {
            const unsigned char uEnd = 0;
            bOk = WriteBytes(pWrite, pEnd, &uEnd, sizeof(uEnd));
        }
        break;
    case LUA_TFUNCTION:
    {
        void const* pValue = lua_topointer(L, -1);
        if (FindLuaGlobal(L, pValue))
        {
            const char* pcName = luaL_checklstring(L, -1, 0);
            bOk = WriteBytes(pWrite, pEnd, pcName, strlen(pcName) + 1);
            lua_settop(L, -2);
        }
        else
        {
            const unsigned char uEnd = 0;
            bOk = WriteBytes(pWrite, pEnd, &uEnd, sizeof(uEnd));
        }
        break;
    }
    default:
        break;
    }
    lua_settop(L, -2);
    return bOk ? static_cast<int>(pWrite - pOut) : -1;
}

static const unsigned char* FindTerminator(const unsigned char* pRead, const unsigned char* pEnd)
{
    return static_cast<const unsigned char*>(memchr(pRead, 0, pEnd - pRead));
}

// FUNCTION: SWBF2 0x0058a5c0
static int ReadLuaItem(lua_State* L, const unsigned char* pData, const unsigned char* pEnd)
{
    if (pData >= pEnd || !lua_checkstack(L, 3))
    {
        return -1;
    }
    const unsigned char* pRead = pData + 1;
    switch (*pData)
    {
    case LUA_TBOOLEAN:
        if (pRead >= pEnd)
        {
            return -1;
        }
        lua_pushboolean(L, *pRead++ != 0);
        break;
    case LUA_TLIGHTUSERDATA:
        if ((size_t)(pEnd - pRead) < sizeof(void*))
        {
            return -1;
        }
        pRead += sizeof(void*);
        lua_pushnil(L);
        break;
    case LUA_TNUMBER:
    {
        float fValue;
        if ((size_t)(pEnd - pRead) < sizeof(fValue))
        {
            return -1;
        }
        memcpy(&fValue, pRead, sizeof(fValue));
        pRead += sizeof(fValue);
        lua_pushnumber(L, fValue);
        break;
    }
    case LUA_TSTRING:
    {
        const unsigned char* pTerminator = FindTerminator(pRead, pEnd);
        if (!pTerminator)
        {
            return -1;
        }
        lua_pushlstring(L, reinterpret_cast<const char*>(pRead), pTerminator - pRead);
        pRead = pTerminator + 1;
        break;
    }
    case LUA_TTABLE:
        lua_newtable(L);
        for (;;)
        {
            if (pRead >= pEnd)
            {
                return -1;
            }
            if (!*pRead)
            {
                break;
            }
            const int iKey = ReadLuaItem(L, pRead, pEnd);
            if (iKey < 0)
            {
                return -1;
            }
            pRead += iKey;
            const int iValue = ReadLuaItem(L, pRead, pEnd);
            if (iValue < 0)
            {
                return -1;
            }
            pRead += iValue;
            if (lua_isnil(L, -2))
            {
                lua_settop(L, -3);
            }
            else if (lua_type(L, -2) == LUA_TNUMBER && lua_tonumber(L, -2) != lua_tonumber(L, -2))
            {
                lua_settop(L, -3);
            }
            else
            {
                lua_rawset(L, -3);
            }
        }
        ++pRead;
        break;
    case LUA_TFUNCTION:
    {
        const unsigned char* pTerminator = FindTerminator(pRead, pEnd);
        if (!pTerminator)
        {
            return -1;
        }
        lua_pushstring(L, reinterpret_cast<const char*>(pRead));
        lua_gettable(L, LUA_GLOBALSINDEX);
        pRead = pTerminator + 1;
        break;
    }
    default:
        lua_pushnil(L);
        break;
    }
    return static_cast<int>(pRead - pData);
}

unsigned int Write(lua_State* L, unsigned char* pOut, unsigned int uCapacity)
{
    const int iNumArgs = lua_gettop(L);
    if (iNumArgs <= 0 || iNumArgs > 0xff || !uCapacity)
    {
        return 0;
    }
    const unsigned char* pEnd = pOut + uCapacity;
    pOut[0] = static_cast<unsigned char>(iNumArgs);
    unsigned int uWritten = 1;
    for (int i = 1; i <= iNumArgs; ++i)
    {
        const int iItem = WriteLuaItem(L, pOut + uWritten, pEnd, i);
        if (iItem < 0)
        {
            return 0;
        }
        uWritten += iItem;
    }
    return uWritten;
}

int Read(lua_State* L, const unsigned char* pData, unsigned int uSize)
{
    if (!uSize)
    {
        return 0;
    }
    const unsigned char* pEnd = pData + uSize;
    const int iTop = lua_gettop(L);
    const int iCount = pData[0];
    unsigned int uRead = 1;
    for (int i = 0; i < iCount; ++i)
    {
        const int iItem = ReadLuaItem(L, pData + uRead, pEnd);
        if (iItem < 0)
        {
            lua_settop(L, iTop);
            return 0;
        }
        uRead += iItem;
    }
    return iCount;
}

}
