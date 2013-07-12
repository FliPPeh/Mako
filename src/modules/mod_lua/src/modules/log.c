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
        { "get_logger", mod_lua_log_get_logger },
        { NULL, NULL }
    };

    const struct luaL_Reg logger_methods[] = {
        { "log", mod_lua_logger_log },
        { "derive", mod_lua_logger_derive },
        { NULL, NULL }
    };

    lua_State *L = mod_lua_state.L;

    /* Stack: api table */
    luaL_newlib(L, log_functions);

    /* Stack: log fn table, api table */
    lua_setglobal(L, "log");

    /* Stack: api table */
    luaL_newmetatable(L, "mod_lua.logger");

    /* Stack: metatable, api table */
    luaL_newlib(L, logger_methods);
    lua_setfield(L, -2, "__index");

    /* Stack: metatable, api table */
    lua_pushcfunction(L, mod_lua_logger_gc);
    lua_setfield(L, -2, "__gc");

    lua_pop(L, 1);

    /* Stack: api table */
    return 0;
}

int mod_lua_log_get_logger(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    struct log_context *ud =
        (struct log_context *)lua_newuserdata(L, sizeof(struct log_context));

    luaL_getmetatable(L, "mod_lua.logger");
    lua_setmetatable(L, -2);

    log_derive(ud, &mod_lua_state.log, name);

    return 1;
}

int mod_lua_logger_log(lua_State *L)
{
    struct log_context *log = lua_util_check_logger(L, 1);
    int lv = lua_util_check_log_level(L, 2);
    const char *txt = luaL_checkstring(L, 3);

    if (lv < 0) {
        return luaL_error(L, "invalid log level `%s'", lua_tostring(L, 2));
    } else {
        log_logf(log, (enum loglevel)lv, "%s", txt);
        return 0;
    }
}

int mod_lua_logger_derive(lua_State *L)
{
    struct log_context *log = lua_util_check_logger(L, 1);
    const char *name = luaL_checkstring(L, 2);

    struct log_context *ud =
        (struct log_context *)lua_newuserdata(L, sizeof(struct log_context));

    luaL_getmetatable(L, "mod_lua.logger");
    lua_setmetatable(L, -2);

    log_derive(ud, log, name);
    return 1;
}

int mod_lua_logger_gc(lua_State *L)
{
    struct log_context *log = lua_util_check_logger(L, 1);

    log_destroy(log);
    return 0;
}
