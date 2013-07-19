#ifndef _MOD_LUA_MODULES_LOG_H_
#define _MOD_LUA_MODULES_LOG_H_

#include <lua.h>


int mod_lua_register_log();

int mod_lua_log_log(lua_State *L);
int mod_lua_log_push(lua_State *L);
int mod_lua_log_pop(lua_State *L);

#endif /* defined _MOD_LUA_MODULES_LOG_H_ */

