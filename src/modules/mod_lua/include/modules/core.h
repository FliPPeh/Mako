#ifndef _MOD_LUA_MODULES_CORE_H_
#define _MOD_LUA_MODULES_CORE_H_

#include <lua.h>


int mod_lua_register_core();

int mod_lua_core_sendmsg(lua_State *L);

int mod_lua_core_get_channels(lua_State *L);
int mod_lua_core_get_channel_meta(lua_State *L);
int mod_lua_core_get_channel_users(lua_State *L);
int mod_lua_core_get_channel_modes(lua_State *L);
int mod_lua_core_get_server_caps(lua_State *L);

#endif /* defined _MOD_LUA_MODULES_CORE_H_ */
