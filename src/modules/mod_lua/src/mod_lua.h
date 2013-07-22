#ifndef _MOD_LUA_H_
#define _MOD_LUA_H_

#include <lua.h>

#include "util/log.h"

struct mod_lua_state
{
    lua_State *L;
    struct log_context log;
};

extern const char _mod_lua_key;
extern struct mod_lua_state mod_lua_state;

#endif /* defined _MOD_LUA_H_ */
