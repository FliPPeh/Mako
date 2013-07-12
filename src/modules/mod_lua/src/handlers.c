#include "irc/irc.h"

#include "lua_util.h"


int mod_lua_on_event(const struct irc_message *m)
{
    return mod_lua_call("__on_event", "m", m);
}

int mod_lua_on_ping()
{
    return mod_lua_call("__on_ping", NULL);
}

int mod_lua_on_privmsg(
        const char *prefix,
        const char *target,
        const char *msg)
{
    return mod_lua_call("__on_privmsg", "sss", prefix, target, msg);
}

int mod_lua_on_notice(
        const char *prefix,
        const char *target,
        const char *msg)
{
    return mod_lua_call("__on_notice", "sss", prefix, target, msg);
}

int mod_lua_on_join(const char *prefix, const char *channel)
{
    return mod_lua_call("__on_join", "ss", prefix, channel);
}

int mod_lua_on_part(
        const char *prefix,
        const char *channel,
        const char *reason)
{
    return mod_lua_call("__on_part", "sss", prefix, channel, reason);
}

int mod_lua_on_quit(const char *prefix, const char *reason)
{
    return mod_lua_call("__on_quit", "ss", prefix, reason);
}

int mod_lua_on_kick(
        const char *prefix_kicker,
        const char *prefix_kicked,
        const char *channel,
        const char *reason)
{
    return mod_lua_call("__on_kick", "ssss",
            prefix_kicker, prefix_kicked, channel, reason);
}

int mod_lua_on_nick(
        const char *prefix_old,
        const char *prefix_new)
{
    return mod_lua_call("__on_nick", "ss", prefix_old, prefix_new);
}

int mod_lua_on_invite(const char *prefix, const char *channel)
{
    return mod_lua_call("__on_invite", "ss", prefix, channel);
}

int mod_lua_on_topic(
        const char *prefix,
        const char *channel,
        const char *topic_old,
        const char *topic_new)
{
    return mod_lua_call("__on_topic", "ssss",
            prefix, channel, topic_old, topic_new);
}

int mod_lua_on_mode_set(
        const char *prefix,
        const char *channel,
        char mode,
        const char *target)
{
    return mod_lua_call("__on_mode_set", "sscs", prefix, channel, mode, target);
}

int mod_lua_on_mode_unset(
        const char *prefix,
        const char *channel,
        char mode,
        const char *target)
{
    return mod_lua_call("__on_mode_unset", "sscs",
            prefix, channel, mode, target);
}


int mod_lua_on_modes(const char *prefix, const char *channel, const char *modes)
{
    return mod_lua_call("__on_modes", "sss", prefix, channel, modes);
}


int mod_lua_on_idle(time_t lastidle)
{
    return mod_lua_call("__on_idle", "t", lastidle);
}

int mod_lua_on_connect()
{
    return mod_lua_call("__on_connect", NULL);
}

int mod_lua_on_disconnect()
{
    return mod_lua_call("__on_disconnect", NULL);
}

int mod_lua_on_ctcp(
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args)
{
    return mod_lua_call("__on_ctcp", "ssss", prefix, target, ctcp, args);
}

int mod_lua_on_ctcp_response(
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args)
{
    return mod_lua_call("__on_ctcp_response", "ssss", prefix, target, ctcp, args);
}

int mod_lua_on_action(const char *prefix, const char *target, const char *msg)
{
    return mod_lua_call("__on_action", "sss", prefix, target, msg);
}

int mod_lua_on_command(
        const char *prefix,
        const char *target,
        const char *command,
        const char *args)
{
    return mod_lua_call("__on_command", "ssss", prefix, target, command, args);
}
