#ifndef _MOD_LUA_HANDLERS_H_
#define _MOD_LUA_HANDLERS_H_

#include <time.h>

int mod_lua_on_event(const struct irc_message *m);
int mod_lua_on_ping();

int mod_lua_on_privmsg(
        const char *prefix,
        const char *target,
        const char *msg);

int mod_lua_on_notice(
        const char *prefix,
        const char *target,
        const char *msg);

int mod_lua_on_join(const char *prefix, const char *channel);

int mod_lua_on_part(
        const char *prefix,
        const char *channel,
        const char *reason);

int mod_lua_on_quit(const char *prefix, const char *reason);

int mod_lua_on_kick(
        const char *prefix_kicker,
        const char *prefix_kicked,
        const char *channel,
        const char *reason);

int mod_lua_on_nick(const char *prefix_old, const char *prefix_new);
int mod_lua_on_invite(const char *prefix, const char *channel);

int mod_lua_on_topic(
        const char *prefix,
        const char *channel,
        const char *topic_old,
        const char *topic_new);

int mod_lua_on_mode_set(
        const char *prefix,
        const char *channel,
        char mode,
        const char *target);

int mod_lua_on_mode_unset(
        const char *prefix,
        const char *channel,
        char mode,
        const char *target);

int mod_lua_on_modes(
        const char *prefix,
        const char *channel,
        const char *modes);

int mod_lua_on_idle(time_t lastidle);
int mod_lua_on_connect();
int mod_lua_on_disconnect();

int mod_lua_on_ctcp(
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args);

int mod_lua_on_ctcp_response(
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args);

int mod_lua_on_action(const char *prefix, const char *target, const char *msg);

int mod_lua_on_command(
        const char *prefix,
        const char *target,
        const char *command,
        const char *args);

#endif /* defined _MOD_LUA_HANDLERS_H_ */
