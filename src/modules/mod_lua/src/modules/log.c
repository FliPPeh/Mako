#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "util/log.h"

#include "mod_lua.h"
#include "lua_util.h"

#include "modules/log.h"


int mod_lua_register_log()
{
    const struct luaL_Reg log_functions[] = {
        { "log", mod_lua_log_log },
        { NULL, NULL }
    };

    int logtab;
    int i = 1;

    lua_State *L = mod_lua_state.L;

    /* Stack: api table */
    luaL_newlib(L, log_functions);

    /* Stack: log fn table */
    logtab = (lua_newtable(L), lua_gettop(L));

#define X(lvl, name, fun, prefmt, postfmt) \
    lua_pushstring(L, name);               \
    lua_rawseti(L, logtab, i++);           \

    LOGLEVELS
#undef X
    ;

    lua_setfield(L, -2, "levels");

    /* Stack: log fn table, api table */
    lua_setglobal(L, "log");

    /* Stack: api table */
    return 0;
}

int mod_lua_log_log(lua_State *L)
{
    int n = lua_gettop(L);

    const char *nam = (n == 3)
        ? luaL_checkstring(L, 1)
        : NULL;

    int lv = lua_util_check_log_level(L, (n == 3) ? 2 : 1);
    const char *txt = luaL_checkstring(L, (n == 3) ? 3 : 2);

    if (lv < 0) {
        return luaL_error(L, "invalid log level `%s'", lua_tostring(L, 2));
    } else {
        log_logf((enum loglevel)lv, nam, "%s", txt);

        return 0;
    }
}

