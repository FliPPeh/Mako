#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "bot/reguser.h"

#include "irc/irc.h"
#include "irc/util.h"
#include "util/list.h"

#include "mod_lua.h"
#include "lua_util.h"

#include "modules/reguser.h"

/*
 * Rationale for using strings instead of bitfields:
 *
 * Even though the reguser system is built upon bit manipulation and testing and
 * Lua 5.2 comes with the bit32 module, idiomatic Lua uses strings for many
 * operations. It also makes more sense in a higher level language to work
 * on higher leveled interfaces.
 *
 * Since everything is string based in Lua, there is no need to export the
 * reguser matching functions. Instead, Lua's string.find is used to test for
 * flags.
 *
 * set_flags and unset_flags receive the flagstring from Lua and convert it to
 * the bitstring, much like it does when reading the entries from a file.
 */

int mod_lua_register_reguser()
{
    const struct luaL_Reg reguser_functions[] = {
        { "add", mod_lua_reguser_add },
        { "del", mod_lua_reguser_del },
        { "get_all", mod_lua_reguser_getall },
        { "find", mod_lua_reguser_find }, /* find by prefix */
        { "set_flags", mod_lua_reguser_setflags },
        { "unset_flags", mod_lua_reguser_unsetflags },
        { NULL, NULL }
    };

    int flagtab;

    lua_State *L = mod_lua_state.L;

    /* Stack: api table */
    luaL_newlib(L, reguser_functions);

    /* Stack: api fn table */
    flagtab = (lua_newtable(L), lua_gettop(L));

    /*
     * Push table mapping flag names to their underlying values
     */
    for (int i = 0; i < FLAGS_MAX; ++i) {
        //lua_pushinteger(L, M(i));
        lua_pushstring(L, _reguser_flags_repr[i]);
        lua_setfield(L, flagtab, (char[2]) { _reguser_flags[i], '\0' });
    }

    lua_setfield(L, -2, "flags");

    /* Stack: api fn table, api table */
    lua_setglobal(L, "reguser");

    /* Stack: api table */
    return 0;
}

int mod_lua_reguser_add(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    const char *flags = luaL_checkstring(L, 2);
    const char *mask = luaL_checkstring(L, 3);

    uint32_t rflags = _reguser_strtoflg(flags);

    lua_pushboolean(L, !reguser_add(BOTREF, name, rflags, mask));
    return 1;
}

int mod_lua_reguser_del(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    struct reguser *usr = NULL;

    if ((usr = reguser_get(BOTREF, name)))
        lua_pushboolean(L, !reguser_del(BOTREF, usr));
    else
        lua_pushboolean(L, 0);

    return 1;
}

int mod_lua_reguser_getall(lua_State *L)
{
    int t = (lua_newtable(L), lua_gettop(L));

    struct list *ptr = NULL;

    LIST_FOREACH(BOTREF->regusers, ptr) {
        struct reguser *usr = list_data(ptr, struct reguser *);
        int t2 = (lua_newtable(L), lua_gettop(L));

        lua_pushstring(L, reguser_flagstr(usr));
        lua_setfield(L, t2, "flags");

        lua_pushstring(L, usr->mask);
        lua_setfield(L, t2, "mask");

        lua_pushinteger(L, usr->flags);
        lua_setfield(L, t2, "rawflags");

        lua_setfield(L, t, usr->name);
    }

    return 1;
}

int mod_lua_reguser_find(lua_State *L)
{
    const char *mask = luaL_checkstring(L, 1);
    struct reguser *usr = NULL;

    if ((usr = reguser_find(BOTREF, mask)))
        lua_pushstring(L, usr->name);
    else
        lua_pushnil(L);

    return 1;
}

int mod_lua_reguser_setflags(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    const char *flags = luaL_checkstring(L, 2);

    for (const char *ptr = flags; *ptr; ++ptr)
        if (!strchr(_reguser_flags, *ptr))
            return luaL_error(L, "no such flag '%c'", *ptr);


    struct reguser *usr = NULL;

    if ((usr = reguser_get(BOTREF, name))) {
        uint32_t rflags = _reguser_strtoflg(flags);

        reguser_set_flags(usr, rflags);
        return 0;
    }

    return luaL_error(L, "no such user '%s'", name);
}

int mod_lua_reguser_unsetflags(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    const char *flags = luaL_checkstring(L, 2);
    const char *ptr = NULL;

    for (struct reguser *ptr = flags; *ptr; ++ptr)
        if (!strchr(_reguser_flags, *ptr))
            return luaL_error(L, "no such flag '%c'", *ptr);


    if ((usr = reguser_get(BOTREF, name))) {
        uint32_t rflags = _reguser_strtoflg(flags);

        reguser_unset_flags(usr, rflags);
        return 0;
    }

    return luaL_error(L, "no such user '%s'", name);
}
