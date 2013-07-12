#include <string.h>
#include <time.h>
#include <stdarg.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "module/module.h"

#include "mod_lua.h"
#include "handlers.h"
#include "lua_util.h"

#include "modules/log.h"
#include "modules/core.h"


struct mod_lua_state mod_lua_state;
const char _mod_lua_key = '#';


struct mod mod_info = {
    .name = "Lua",
    .descr = "Provide a Lua interface"
};


int mod_init()
{
    struct log_context log;

    lua_State *L = NULL;

    log_derive(&mod_lua_state.log, NULL, "mod_lua");
    log_derive(&log, &mod_lua_state.log, __func__);

    if (!(mod_lua_state.L = luaL_newstate())) {
        log_error(&log, "Unable to initialize Lua state");

        goto exit_err;
    }

    L = mod_lua_state.L;

    luaL_openlibs(L);
    lua_pushlightuserdata(L, (void *)&_mod_lua_key);
    lua_pushlightuserdata(L, (void *)&mod_info);
    lua_settable(L, LUA_REGISTRYINDEX);

    log_info(&log, "Loading Lua bootstrapper...");

    mod_lua_register_core();
    mod_lua_register_log();

    if (luaL_dofile(L, "bootstrap.lua")) {
        log_error(&log, "Unable to open Lua bootstrapper: %s",
                lua_tostring(L, -1));

        goto exit_err;
    }

    log_info(&log, "Success!");
    lua_pop(L, -1);

    log_destroy(&log);
    return 0;

exit_err:
    log_destroy(&log);
    return 1;

}

int mod_handle_event(struct mod_event *event)
{
    switch (event->type) {
    case EVENT_RAW:
        mod_lua_on_event(event->raw.msg);
        break;

    case EVENT_PRIVMSG:
        mod_lua_on_privmsg(
                event->privmsg.prefix,
                event->privmsg.target,
                event->privmsg.msg);
        break;

    case EVENT_NOTICE:
        mod_lua_on_notice(
                event->notice.prefix,
                event->notice.target,
                event->notice.msg);
        break;

    case EVENT_JOIN:
        mod_lua_on_join(event->join.prefix, event->join.channel);
        break;

    case EVENT_PART:
        mod_lua_on_part(
                event->part.prefix,
                event->part.channel,
                event->part.msg);
        break;

    case EVENT_QUIT:
        mod_lua_on_quit(event->quit.prefix, event->quit.msg);
        break;

    case EVENT_KICK:
        mod_lua_on_kick(
                event->kick.prefix_kicker,
                event->kick.prefix_kicked,
                event->kick.channel,
                event->kick.msg);
        break;

    case EVENT_NICK:
        mod_lua_on_nick(event->nick.prefix_old, event->nick.prefix_new);
        break;

    case EVENT_INVITE:
        mod_lua_on_invite(event->invite.prefix, event->invite.channel);
        break;

    case EVENT_TOPIC:
        mod_lua_on_topic(
                event->topic.prefix,
                event->topic.channel,
                event->topic.topic_old,
                event->topic.topic_new);
        break;

    case EVENT_MODESET:
        mod_lua_on_mode_set(
                event->mode_set.prefix,
                event->mode_set.channel,
                event->mode_set.mode,
                event->mode_set.arg);
        break;

    case EVENT_MODEUNSET:
        mod_lua_on_mode_unset(
                event->mode_unset.prefix,
                event->mode_unset.channel,
                event->mode_unset.mode,
                event->mode_unset.arg);
        break;

    case EVENT_MODES:
        mod_lua_on_modes(
                event->modes.prefix,
                event->modes.channel,
                event->modes.modes);
        break;

    case EVENT_IDLE:
        mod_lua_on_idle(event->idle.last);
        break;

    case EVENT_CONNECT:
        mod_lua_on_connect();
        break;

    case EVENT_DISCONNECT:
        mod_lua_on_disconnect();
        break;

    case EVENT_CTCPREQ:
        mod_lua_on_ctcp(
                event->ctcp_req.prefix,
                event->ctcp_req.target,
                event->ctcp_req.ctcp,
                event->ctcp_req.arg);
        break;

    case EVENT_CTCPRESP:
        mod_lua_on_ctcp_response(
                event->ctcp_resp.prefix,
                event->ctcp_resp.target,
                event->ctcp_resp.ctcp,
                event->ctcp_resp.arg);
        break;

    case EVENT_ACTION:
        mod_lua_on_action(
                event->action.prefix,
                event->action.target,
                event->action.msg);
        break;

    case EVENT_PING:
        mod_lua_on_ping();
        break;

    case EVENT_COMMAND:
        mod_lua_on_command(
                event->command.prefix,
                event->command.target,
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

    log_destroy(&mod_lua_state.log);

    return 0;
}

