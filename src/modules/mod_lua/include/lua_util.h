#ifndef _MOD_LUA_UTIL_H_
#define _MOD_LUA_UTIL_H_

#include <lua.h>

#include "irc/irc.h"


int mod_lua_call(const char *fn, const char *argtypes, ...);

/*
 * Pushers
 */
int lua_util_push_irc_message(lua_State *L, const struct irc_message *msg);

int lua_util_push_irc_channel_meta(lua_State *L, const struct irc_channel *ch);
int lua_util_push_irc_channel_modes(lua_State *L, const struct irc_channel *ch);
int lua_util_push_irc_channel_users(lua_State *L, const struct irc_channel *ch);

int lua_util_push_irc_caps(lua_State *L, const struct list *caps);

/*
 * Checkers
 */
struct log_context *lua_util_check_logger(lua_State *L, int n);
int lua_util_check_log_level(lua_State *L, int n);

int lua_util_check_irc_message(lua_State *L, struct irc_message *msg);

struct mod *lua_util_check_mod(lua_State *L);

#endif /* defined _MOD_LUA_UTIL_H_ */
