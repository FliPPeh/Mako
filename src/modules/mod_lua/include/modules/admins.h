#ifndef _MOD_LUA_MODULES_REGUSER_H_
#define _MOD_LUA_MODULES_REGUSER_H_

#include <lua.h>


int mod_lua_register_reguser();

int mod_lua_reguser_add(lua_State *L);
int mod_lua_reguser_del(lua_State *L);
int mod_lua_reguser_get(lua_State *L);
int mod_lua_reguser_find(lua_State *L);

#endif /* defined _MOD_LUA_MODULES_REGUSER_H_ */
