#ifndef HANDLERS_H
#define HANDLERS_H

#include "bot/bot.h"
#include "irc/irc.h"

#include "modules/module.h"

#include <time.h>


int bot_split_ctcp(const char *source,
        char *dctcp, size_t sctcp,
        char *dargs, size_t sargs);

int bot_dispatch_event(struct bot *bot, struct mod_event *ev);

int bot_handle_command(struct bot *bot,
        const char *prefix,
        const char *target,
        const char *args);

int bot_on_event(void *arg, const struct irc_message *m);
int bot_on_ping(void *arg);

int bot_on_privmsg(void *arg,
        const char *prefix,
        const char *target,
        const char *msg);

int bot_on_notice(void *arg,
        const char *prefix,
        const char *target,
        const char *msg);

int bot_on_join(void *arg, const char *prefix, const char *channel);

int bot_on_part(void *arg,
        const char *prefix,
        const char *channel,
        const char *reason);

int bot_on_quit(void *arg, const char *prefix, const char *reason);

int bot_on_kick(void *arg,
        const char *prefix_kicker,
        const char *prefix_kicked,
        const char *channel,
        const char *reason);

int bot_on_nick(void *arg, const char *prefix_old, const char *prefix_new);
int bot_on_invite(void *arg, const char *prefix, const char *channel);

int bot_on_topic(void *arg,
        const char *prefix,
        const char *channel,
        const char *topic_old,
        const char *topic_new);

int bot_on_mode_set(void *arg,
        const char *prefix,
        const char *channel,
        char mode,
        const char *target);

int bot_on_mode_unset(void *arg,
        const char *prefix,
        const char *channel,
        char mode,
        const char *target);

int bot_on_modes(void *arg,
        const char *prefix,
        const char *channel,
        const char *modes);

int bot_on_idle(void *arg, time_t lastidle);
int bot_on_connect(void *arg);
int bot_on_disconnect(void *arg);

int bot_on_ctcp(struct bot *bot,
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args);

int bot_on_ctcp_response(struct bot *bot,
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args);

int bot_on_action(struct bot *bot,
        const char *prefix,
        const char *target,
        const char *msg);

#endif /* defined _HANDLERS_H_ */
