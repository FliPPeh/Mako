#ifndef _MOD_LUA_MODULES_LOG_H_
#define _MOD_LUA_MODULES_LOG_H_

#include <lua.h>


int mod_lua_register_log();

int mod_lua_log_get_logger(lua_State *L);
int mod_lua_logger_log(lua_State *L);
int mod_lua_logger_derive(lua_State *L);
int mod_lua_logger_gc(lua_State *L);

#endif /* defined _MOD_LUA_MODULES_LOG_H_ */

