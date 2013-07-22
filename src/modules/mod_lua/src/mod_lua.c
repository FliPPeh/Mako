#include <string.h>
#include <time.h>
#include <stdarg.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "modules/module.h"

#include "mod_lua.h"
#include "lua_util.h"

#include "modules/core.h"
#include "modules/log.h"
#include "modules/reguser.h"


struct mod_lua_state mod_lua_state;
const char _mod_lua_key = '#';


struct mod mod_info = {
    .name = "Lua",
    .descr = "Provide a Lua interface",

    .hooks = ~0 /* all */
};


int mod_init()
{
    lua_State *L = NULL;

    if (!(mod_lua_state.L = luaL_newstate())) {
        log_error_n("mod_lua", "Unable to initialize Lua state");

        goto exit_err;
    }

    L = mod_lua_state.L;

    luaL_openlibs(L);
    lua_pushlightuserdata(L, (void *)&_mod_lua_key);
    lua_pushlightuserdata(L, (void *)&mod_info);
    lua_settable(L, LUA_REGISTRYINDEX);

    log_info_n("mod_lua", "Loading Lua bootstrapper...");

    mod_lua_register_core();
    mod_lua_register_log();
    mod_lua_register_reguser();

    if (luaL_dofile(L, "bootstrap.lua")) {
        log_error_n("mod_lua", "Unable to open Lua bootstrapper: %s",
                lua_tostring(L, -1));

        goto exit_err;
    }

    log_info_n("mod_lua", "Success!");
    lua_pop(L, -1);

    return 0;

exit_err:
    return 1;

}

int mod_handle_event(struct mod_event *event)
{
    /*
     * No handling needed, just pass it on to Lua, which will take care of
     * getting the events where they need to go.
     */
    switch (event->type) {
    case EVENT_RAW:
        mod_lua_dispatch(event->type, "m", event->raw.msg);
        break;

    case EVENT_PUBLIC_MESSAGE:
    case EVENT_PUBLIC_ACTION:
    case EVENT_PUBLIC_NOTICE:
        mod_lua_dispatch(event->type, "sss",
                event->message.prefix,
                event->message.target,
                event->message.msg);
        break;

    case EVENT_PRIVATE_MESSAGE:
    case EVENT_PRIVATE_ACTION:
    case EVENT_PRIVATE_NOTICE:
        mod_lua_dispatch(event->type, "ss",
                event->message.prefix,
                event->message.msg);
        break;

    case EVENT_JOIN:
        mod_lua_dispatch(event->type, "ss",
                event->join.prefix,
                event->join.channel);
        break;

    case EVENT_PART:
        mod_lua_dispatch(event->type, "sss",
                event->part.prefix,
                event->part.channel,
                event->part.msg);
        break;

    case EVENT_QUIT:
        mod_lua_dispatch(event->type, "ss",
                event->quit.prefix,
                event->quit.msg);
        break;

    case EVENT_KICK:
        mod_lua_dispatch(event->type, "ssss",
                event->kick.prefix_kicker,
                event->kick.prefix_kicked,
                event->kick.channel,
                event->kick.msg);
        break;

    case EVENT_NICK:
        mod_lua_dispatch(event->type, "ss",
                event->nick.prefix_old,
                event->nick.prefix_new);
        break;

    case EVENT_INVITE:
        mod_lua_dispatch(event->type, "ss",
                event->invite.prefix,
                event->invite.channel);
        break;

    case EVENT_TOPIC:
        mod_lua_dispatch(event->type, "ssss",
                event->topic.prefix,
                event->topic.channel,
                event->topic.topic_old,
                event->topic.topic_new);
        break;

    case EVENT_CHANNEL_MODE_SET:
    case EVENT_CHANNEL_MODE_UNSET:
        mod_lua_dispatch(event->type, "sscs",
                event->mode_change.prefix,
                event->mode_change.channel,
                event->mode_change.mode,
                event->mode_change.arg);
        break;

    case EVENT_CHANNEL_MODES:
        mod_lua_dispatch(event->type, "sss",
                event->modes.prefix,
                event->modes.channel,
                event->modes.modes);
        break;

    case EVENT_IDLE:
        mod_lua_dispatch(event->type, "t", event->idle.last);
        break;

    case EVENT_CONNECT:
    case EVENT_DISCONNECT:
    case EVENT_PING:
        mod_lua_dispatch(event->type, NULL);
        break;

    case EVENT_PUBLIC_CTCP_REQUEST:
    case EVENT_PUBLIC_CTCP_RESPONSE:
    case EVENT_PUBLIC_COMMAND:
        mod_lua_dispatch(event->type, "ssss",
                event->command.prefix,
                event->command.target,
                event->command.command,
                event->command.args);
        break;

    case EVENT_PRIVATE_CTCP_REQUEST:
    case EVENT_PRIVATE_CTCP_RESPONSE:
    case EVENT_PRIVATE_COMMAND:
        mod_lua_dispatch(event->type, "sss",
                event->command.prefix,
                event->command.command,
                event->command.args);
        break;
    }

    return 0;
}

int mod_exit()
{
    if (mod_lua_state.L)
        lua_close(mod_lua_state.L);

    return 0;
}

