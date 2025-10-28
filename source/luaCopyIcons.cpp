/*---------------------------------------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#------  This File is Part Of : ---------------------------------------------------------------------------------------#
#------- _  -------------------  ______  _   --------------------------------------------------------------------------#
#------ | | ------------------- (_____ \| |  --------------------------------------------------------------------------#
#------ | | ---  _   _   ____   _____) )| |  ____  _   _   ____   ____   ----------------------------------------------#
#------ | | --- | | | | / _  |  |  ____/| | / _  || | | | / _  ) / ___)  ----------------------------------------------#
#------ | |_____| |_| |( ( | |  | |     | |( ( | || |_| |( (/ / | |  --------------------------------------------------#
#------ |_______)\____| \_||_|  |_|     |_| \_||_| \__  | \____)|_|  --------------------------------------------------#
#------------------------------------------------- (____/ -------------------------------------------------------------#
#------------------------   ______   _   ------------------------------------------------------------------------------#
#------------------------  (_____ \ | |  ------------------------------------------------------------------------------#
#------------------------   _____) )| | _   _   ___   -----------------------------------------------------------------#
#------------------------  |  ____/ | || | | | /___)  -----------------------------------------------------------------#
#------------------------  | |      | || |_| ||___ |  -----------------------------------------------------------------#
#------------------------  |_|      |_| \____|(___/   -----------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- Licensed under the GPL License -------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- Copyright (c) Nanni <lpp.nanni@gmail.com> --------------------------------------------------------------------------#
#- Copyright (c) Rinnegatamante <rinnegatamante@gmail.com> ------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- This file :---------------------------------------------------------------------------------------------------------#
#- Created by jimbob4000 to extend functionality for RetroFlow Launcher: ----------------------------------------------#
#- https://github.com/jimbob4000/RetroFlow-Launcher -------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- Credits : ----------------------------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- cy33hc for Copyicons functions made possible by compiling, cy33hc's copyicons code into a lib ----------------------#
#- https://github.com/cy33hc/copyicons --------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- cy33hc's works are licensed under GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007 -------------------------------#
#----------------------------------------------------------------------------------------------------------------------*/


// use the project's luaplayer header which includes the correct C++ Lua headers
#include "include/luaplayer.h"

// copyicons library provides these C functions in the static library; declare them here
extern "C" {
    int copy_decrypted_icon(const char *app_path);
    int copy_decrypted_pic(const char *app_path);
}

static int l_system_copy_decrypted_icon(lua_State *L) {
    const char *app_path = luaL_checkstring(L, 1);
    int result = copy_decrypted_icon(app_path);
    lua_pushboolean(L, result != 0);
    return 1;
}

static int l_system_copy_decrypted_pic(lua_State *L) {
    const char *app_path = luaL_checkstring(L, 1);
    int result = copy_decrypted_pic(app_path);
    lua_pushboolean(L, result != 0);
    return 1;
}

//Register our LuaCopyIcons Functions
static const luaL_Reg LuaCopyIcons_functions[] = {
    {"copyDecryptedIcon",         l_system_copy_decrypted_icon},
    {"copyDecryptedPic",          l_system_copy_decrypted_pic},
    {0, 0}
};

void LuaCopyIcons_init(lua_State *L) {
    lua_newtable(L);
    luaL_setfuncs(L, LuaCopyIcons_functions, 0);
    lua_setglobal(L, "CopyIcons");
}

// use the project's luaplayer header which includes the correct C++ Lua headers
#include "include/luaplayer.h"

// copyicons library provides these C functions in the static library; declare them here
extern "C" {
    int copy_decrypted_icon(const char *app_path);
    int copy_decrypted_pic(const char *app_path);
}

static int l_system_copy_decrypted_icon(lua_State *L) {
    const char *app_path = luaL_checkstring(L, 1);
    int result = copy_decrypted_icon(app_path);
    lua_pushboolean(L, result != 0);
    return 1;
}

static int l_system_copy_decrypted_pic(lua_State *L) {
    const char *app_path = luaL_checkstring(L, 1);
    int result = copy_decrypted_pic(app_path);
    lua_pushboolean(L, result != 0);
    return 1;
}

//Register our LuaCopyIcons Functions
static const luaL_Reg LuaCopyIcons_functions[] = {
	{"copyDecryptedIcon",         l_system_copy_decrypted_icon},
	{"copyDecryptedPic",          l_system_copy_decrypted_pic},
	{0, 0}
};

void LuaCopyIcons_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, LuaCopyIcons_functions, 0);
	lua_setglobal(L, "CopyIcons");
}