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

#include "modules/admins.h"


int mod_lua_register_reguser()
{
    const struct luaL_Reg reguser_functions[] = {
        { "add", mod_lua_reguser_add },
        { "del", mod_lua_reguser_del },
        { "get", mod_lua_reguser_get },   /* get by ID */
        { "find", mod_lua_reguser_find }, /* find by prefix */
        { NULL, NULL }
    };

    lua_State *L = mod_lua_state.L;

    /* Stack: api table */
    luaL_newlib(L, core_functions);

    /* Stack: api fn table, api table */
    lua_setglobal(L, "reguser");

    /* Stack: api table */
    return 0;
}

int mod_lua_reguser_add(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    const char *mask = luaL_checkstring(L, 2);

    lua_pushboolean(L, !reguser_add(name, mask));
    return 1;
}

int mod_lua_reguser_del(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    struct admin *adm = NULL;

    if ((adm = reguser_get(name))) {
        lua_pushboolean(L, !reguser_del(adm));
    }

    lua_pushboolean(L, 0);
    return 1;
}

int mod_lua_reguser_get(lua_State *L)
{
    const char *name = lua_tostring(L, 1);
    struct list *ptr = NULL;

    LIST_FOREACH(reguser_get_all

    if (name)

}

int mod_lua_reguser_find(lua_State *L);
