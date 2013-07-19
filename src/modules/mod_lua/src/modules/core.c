#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "module/module.h"
#include "irc/irc.h"
#include "irc/util.h"
#include "util/list.h"

#include "mod_lua.h"
#include "lua_util.h"

#include "modules/core.h"


int mod_lua_register_core()
{
    const struct luaL_Reg core_functions[] = {
        { "sendmsg", mod_lua_core_sendmsg },
        { "get_identity", mod_lua_core_get_identity },
        { "get_channels", mod_lua_core_get_channels },
        { "get_channel_meta", mod_lua_core_get_channel_meta },
        { "get_channel_modes", mod_lua_core_get_channel_modes },
        { "get_channel_users", mod_lua_core_get_channel_users },
        { "get_server_capabilities", mod_lua_core_get_server_caps },
        { NULL, NULL }
    };

    lua_State *L = mod_lua_state.L;

    /* Stack: api table */
    luaL_newlib(L, core_functions);

    /* Stack: api fn table, api table */
    lua_setglobal(L, "bot");

    /* Stack: api table */
    return 0;
}

int mod_lua_core_sendmsg(lua_State *L)
{
    struct mod *mod = lua_util_check_mod(L);
    struct irc_message msg;

    if (lua_util_check_irc_message(L, &msg))
        return luaL_error(L, "incomplete or invalid irc message table");

    mod_sendmsg(mod, &msg);

    return 0;
}

int mod_lua_core_get_identity(lua_State *L)
{
    const char *kind = lua_tostring(L, 1);
    int itab = (lua_newtable(L), lua_gettop(L));
    struct mod_identity ident;

    get_identity(&ident);

    if (!kind || !strcmp(kind, "*a")) {
        lua_pushstring(L, ident.nick);
        lua_setfield(L, itab, "nick");

        lua_pushstring(L, ident.user);
        lua_setfield(L, itab, "user");

        lua_pushstring(L, ident.real);
        lua_setfield(L, itab, "realname");

    } else if (!strcmp(kind, "n")) {
        lua_pushstring(L, ident.nick);

    } else if (!strcmp(kind, "u")) {
        lua_pushstring(L, ident.user);

    } else if (!strcmp(kind, "r")) {
        lua_pushstring(L, ident.real);

    } else {
        lua_pushnil(L);
    }

    return 1;
}

int mod_lua_core_get_channels(lua_State *L)
{
    struct list *channels = get_channels();
    struct list *ptr = NULL;

    int clist = (lua_newtable(L), lua_gettop(L));
    int i = 1;

    LIST_FOREACH(channels, ptr) {
        struct irc_channel *channel = list_data(ptr, struct irc_channel *);

        lua_pushstring(L, channel->name);
        lua_rawseti(L, clist, i++);
    }

    return 1;
}

int mod_lua_core_get_channel_meta(lua_State *L)
{
    const char *chan = luaL_checkstring(L, 1);

    struct list *channels = get_channels();
    struct irc_channel *channel = irc_channel_get(channels, chan);

    if (!channel)
        return luaL_error(L, "no such channel '%s'", chan);

    return lua_util_push_irc_channel_meta(L, channel);
}

int mod_lua_core_get_channel_modes(lua_State *L)
{
    const char *chan = luaL_checkstring(L, 1);

    struct list *channels = get_channels();
    struct irc_channel *channel = irc_channel_get(channels, chan);

    if (!channel)
        return luaL_error(L, "no such channel '%s'", chan);

    return lua_util_push_irc_channel_modes(L, channel);
}

int mod_lua_core_get_channel_users(lua_State *L)
{
    const char *chan = luaL_checkstring(L, 1);

    struct list *channels = get_channels();
    struct irc_channel *channel = irc_channel_get(channels, chan);

    if (!channel)
        return luaL_error(L, "no such channel '%s'", chan);

    return lua_util_push_irc_channel_users(L, channel);
}

int mod_lua_core_get_server_caps(lua_State *L)
{
    struct list *caps = get_server_capabilities();

    return lua_util_push_irc_caps(L, caps);
}
