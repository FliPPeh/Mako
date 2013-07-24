#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "mod_lua.h"

#include "lua_util.h"
#include "irc/irc.h"
#include "util/log.h"
#include "util/list.h"

#include "modules/module.h"


int mod_lua_dispatch(enum mod_event_type type, const char *argtypes, ...)
{
    const char *fn = "__handle_event";
    va_list args;
    int nargs = 1;

    lua_State *L = mod_lua_state.L;

    lua_getglobal(L, fn);
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);

        log_error_n("mod_lua",
                "attempt to call global non-function or nil '%s'", fn);

        return 1;
    }

    lua_pushstring(L, _mod_event_name[type]);

    if (argtypes) {
        va_start(args, argtypes);

        while (*argtypes) {
            char str[2] = {0};

            switch (*argtypes) {
            case 'c':
                str[0] = (char)va_arg(args, int);
                lua_pushstring(L, str);
                nargs++;
                break;

            case 'm':
                lua_util_push_irc_message(L,
                        va_arg(args, struct irc_message *));
                nargs++;
                break;

            case 's':
                lua_pushstring(L, va_arg(args, const char *));
                nargs++;
                break;

            case 't':
                lua_pushinteger(L, va_arg(args, time_t));
                nargs++;
                break;

            default:
                log_warn_n("mod_lua", "Unknown argtype '%c'", *argtypes);
            }

           argtypes++;
        }

        va_end(args);
    }

    if (lua_pcall(L, nargs, 0, 0) != 0) {
        log_error_n("mod_lua", "Error calling '%s' in corelib: %s",
                fn, lua_tostring(L, -1));

        lua_pop(L, 1);
    }

    lua_pop(L, -1);

    return 0;
}

/*
 * Pushers
 */
int lua_util_push_irc_message(lua_State *L, const struct irc_message *msg)
{
    int tab = (lua_newtable(L), lua_gettop(L));
    int argtab;

    lua_pushstring(L, msg->prefix);
    lua_setfield(L, tab, "prefix");

    lua_pushstring(L, msg->command);
    lua_setfield(L, tab, "command");

    lua_pushstring(L, msg->msg);
    lua_setfield(L, tab, "message");

    argtab = (lua_newtable(L), lua_gettop(L));
    for (int i = 0; i < msg->paramcount; ++i) {
        lua_pushstring(L, msg->params[i]);
        lua_rawseti(L, argtab, i + 1);
    }

    lua_setfield(L, tab, "args");

    return 1;
}

int lua_util_push_irc_channel_meta(lua_State *L, const struct irc_channel *ch)
{
    int chtab = (lua_newtable(L), lua_gettop(L));

    lua_pushstring(L, ch->name);
    lua_setfield(L, chtab, "name");

    lua_pushinteger(L, ch->created);
    lua_setfield(L, chtab, "created");

    lua_pushstring(L, ch->topic);
    lua_setfield(L, chtab, "topic");

    lua_pushstring(L, ch->topic_setter);
    lua_setfield(L, chtab, "topic_setter");

    lua_pushinteger(L, ch->topic_set);
    lua_setfield(L, chtab, "topic_set");

    return 1;
}

int lua_util_push_irc_channel_modes(lua_State *L, const struct irc_channel *ch)
{
    struct list *ptr = NULL;
    int  mtab = (lua_newtable(L), lua_gettop(L));

    LIST_FOREACH(ch->modes, ptr) {
        struct irc_mode *mode = list_data(ptr, struct irc_mode *);

        char modestr[2] = { mode->mode, '\0' };

        lua_getfield(L, mtab, modestr);
        if (lua_isnil(L, -1)) {
            /* nil, set modes[m] = arg */
            lua_pop(L, 1); /* discard */

            lua_pushstring(L, mode->arg);
            lua_setfield(L, mtab, modestr);
        } else if (lua_isstring(L, -1)) {
            /* string, set modes[m] = { modes[m], arg } */
            int matab = (lua_newtable(L), lua_gettop(L));

            /* Push old value to modes[m][1] */
            lua_rawseti(L, matab, 1);

            /* Push new value to modes[m][2] */
            lua_pushstring(L, mode->arg);
            lua_rawseti(L, matab, 2);

            lua_setfield(L, mtab, modestr);
        } else if (lua_istable(L, -1)) {
            /* table, append */
            int len = lua_rawlen(L, -1);

            lua_pushstring(L, mode->arg);
            lua_rawseti(L, -2, len);
        }
    }

    return 1;
}

int lua_util_push_irc_channel_users(lua_State *L, const struct irc_channel *ch)
{
    struct list *ptr = NULL;
    int ulist = (lua_newtable(L), lua_gettop(L));

    LIST_FOREACH(ch->users, ptr) {
        const struct irc_user *user = list_data(ptr, const struct irc_user *);

        lua_pushstring(L, user->modes);
        lua_setfield(L, ulist, user->prefix);
    }

    return 1;
}

int lua_util_push_irc_caps(lua_State *L, const struct list *caps)
{
    int clist = (lua_newtable(L), lua_gettop(L));
    const struct list *ptr = NULL;

    LIST_FOREACH(caps, ptr) {
        const struct irc_capability *cap =
            list_data(ptr, const struct irc_capability *);

        lua_pushstring(L, cap->value);
        lua_setfield(L, clist, cap->capability);
    }

    return 1;
}

/*
 * Checkers
 */
struct log_context *lua_util_check_logger(lua_State *L, int n)
{
    struct log_context *ud = luaL_checkudata(L, n, "mod_lua.logger");
    luaL_argcheck(L, ud != NULL, n, "'logger' expected");

    return ud;
}


int lua_util_check_log_level(lua_State *L, int n)
{
    const char *lvlstr = luaL_checkstring(L, n);

#define X(lvl, name, fun, prefmt, postfmt)  \
    if (!strcasecmp(name, lvlstr))          \
        return lvl;                         \
    else
    LOGLEVELS
#undef X
        return -1;
}

int lua_util_check_irc_message(lua_State *L, struct irc_message *msg)
{
    int t = lua_gettop(L);
    const char *command = NULL;
    const char *message = NULL;

    memset(msg, 0, sizeof(*msg));

    lua_getfield(L, t, "command"); /* Stack: command, table */
    lua_getfield(L, t, "message"); /* Stack: message, command, table */
    lua_getfield(L, t, "args"); /* Stack: args, message, command, table */

    command = lua_tostring(L, -3);
    message = lua_tostring(L, -2);

    if (!command && (!message || lua_isnil(L, -1)))
        return 1;

    strncpy(msg->command, command, sizeof(msg->command) - 1);

    if (message)
        strncpy(msg->msg, message, sizeof(msg->msg) - 1);

    if (!lua_isnil(L, -1)) {
        int n = lua_rawlen(L, -1);

        if (n > IRC_PARAM_COUNT_MAX)
            return 1;

        for (int i = 1; i <= n; ++i) {
            lua_rawgeti(L, -1, i);

            if (!lua_isnil(L, -1)) {
                const char *arg = lua_tostring(L, -1);

                strncpy(msg->params[msg->paramcount++], arg,
                        sizeof(msg->params[0]) - 1);
            }

            lua_pop(L, 1);
        }
    }

    lua_pop(L, 3);

    return 0;
}

struct mod *lua_util_check_mod(lua_State *L)
{
    struct mod *mod = NULL;

    lua_pushlightuserdata(L, (void *)&_mod_lua_key);
    lua_gettable(L, LUA_REGISTRYINDEX);

    mod = lua_touserdata(L, -1);
    lua_pop(L, 1);

    return mod;
}
