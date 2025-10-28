#ifndef LUA_EXTENDED_H
#define LUA_EXTENDED_H

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

// Initialize Extended Lua functions
void LuaExtended_init(lua_State *L);

#endif // LUA_EXTENDED_H