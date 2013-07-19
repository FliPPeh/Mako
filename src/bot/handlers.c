#include <string.h>
#include <ctype.h>

#include "bot/handlers.h"
#include "bot/bot.h"
#include "bot/module.h"

#include "module/module.h"

#include "irc/irc.h"
#include "irc/util.h"
#include "irc/session.h"

#include "util/list.h"
#include "util/log.h"
#include "util/util.h"


int bot_split_ctcp(const char *source,
        char *dctcp, size_t sctcp,
        char *dargs, size_t sargs)
{
    const char *argpos = NULL;

    memset(dctcp, 0, sctcp);
    memset(dargs, 0, sargs);

    if ((argpos = strchr(source, ' '))) {
        strncpy(dctcp, source + 1, MIN(sctcp - 1, argpos - source - 1));
        strncpy(dargs, argpos + 1, MIN(sargs - 1,
                    strlen(source) - (argpos - source) - 2));
    } else {
        strncpy(dctcp, source + 1, strlen(source) - 2);
    }

    return 0;
}

int bot_dispatch_event(struct bot *bot, struct mod_event *ev)
{
    struct list *mod = NULL;

    LIST_FOREACH(bot->modules, mod) {
        struct mod_loaded *m = list_data(mod, struct mod_loaded *);

        if (m->state->hooks & MASK(ev->type))
            m->handler_func(ev);
    }

    return 0;
}

int bot_handle_command(struct bot *bot,
        const char *prefix,
        const char *target,
        const char *cmd,
        const char *args)
{
    struct irc_prefix_parts user;
    struct irc_message response;
    int priv = !irc_is_channel(target);

    irc_split_prefix(&user, prefix);

    if (!strcmp(cmd, "load_so")) {
        if (!mod_load(bot, args)) {
            struct mod_loaded *mod = mod_get(bot, args);

            irc_mkprivmsg(&response, irc_proper_target(target, user.nick),
                    "Successfully loaded module '%s' (%s)",
                    mod->path, mod->state->name);
        } else {
            irc_mkprivmsg(&response, irc_proper_target(target, user.nick),
                "Failed loading module './%s.so'", args);
        }

        sess_sendmsg(bot->sess, &response);
    } else if (!strcmp(cmd, "unload_so")) {
        struct mod_loaded *mod = mod_get(bot, args);

        if (mod) {
            if (!mod_unload(bot, mod))
                irc_mkprivmsg(&response, irc_proper_target(target, user.nick),
                    "Successfully unloaded module './%s.so'", args);
            else
                irc_mkprivmsg(&response, irc_proper_target(target, user.nick),
                    "Failed unloading module '%s'", args);
        } else {
            irc_mkprivmsg(&response, irc_proper_target(target, user.nick),
                "No such module './%s.so'", args);
        }

        sess_sendmsg(bot->sess, &response);
    } else if (!strcmp(cmd, "echo")) {
        irc_mkprivmsg_resp(&response,
                irc_proper_target(target, user.nick), user.nick,
                "%s", args);

        sess_sendmsg(bot->sess, &response);
    }

    return bot_dispatch_event(bot, &(struct mod_event) {
        .type = priv ? EVENT_PRIVATE_COMMAND : EVENT_PUBLIC_COMMAND,
        .command = {
            .prefix = prefix,
            .target = priv ? NULL : target,
            .command = cmd,
            .args = args
        }
    });
}

int bot_on_event(void *arg, const struct irc_message *m)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_RAW,
        .raw = {
            .msg = m
        }
    });
}

int bot_on_ping(void *arg)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_PING
    });
}

int bot_on_privmsg(void *arg,
        const char *prefix,
        const char *target,
        const char *msg)
{
    struct bot *bot = arg;

    if ((msg[0] == 0x01) && (msg[strlen(msg) - 1] == 0x01)) {
        /* CTCP */
        char ctcp[IRC_MESSAGE_MAX] = {0};
        char args[IRC_MESSAGE_MAX] = {0};

        bot_split_ctcp(msg, ctcp, sizeof(ctcp), args, sizeof(args));

        return bot_on_ctcp(bot, prefix, target, ctcp, args);
    } else {
        /* Message starts with trigger? Fire zeh handler */
        if ((strstr(msg, bot->sess->nick) == msg) &&
             strchr(TRIGGERS, msg[strlen(bot->sess->nick)])) {

            const char *start = msg + strlen(bot->sess->nick) + 1;
            char *ptr = NULL;

            char cmd[32] = {0};
            char arg[256] = {0};

            char copy[IRC_MESSAGE_MAX] = {0};

            while (isspace(*start))
                start++;

            strncpy(copy, start, sizeof(copy) - 1);

            if ((ptr = strtok(copy, " "))) {
                strncpy(cmd, ptr, sizeof(cmd) - 1);

                if ((ptr = strtok(NULL, "")))
                    strncpy(arg, ptr, sizeof(arg) - 1);

                return bot_handle_command(bot, prefix, target, cmd, arg);
            }
        } else {
            int priv = !irc_is_channel(target);

            return bot_dispatch_event(bot, &(struct mod_event) {
                .type = priv ? EVENT_PRIVATE_MESSAGE : EVENT_PUBLIC_MESSAGE,
                .message = {
                    .prefix = prefix,
                    .target = priv ? NULL : target,
                    .msg = msg
                }
            });
        }
    }

    return 0;
}

int bot_on_notice(void *arg,
        const char *prefix,
        const char *target,
        const char *msg)
{
    struct bot *bot = arg;

    if ((msg[0] == 0x01) && (msg[strlen(msg) - 1] == 0x01)) {
        /* CTCP response */
        char ctcp[IRC_MESSAGE_MAX] = {0};
        char args[IRC_MESSAGE_MAX] = {0};

        bot_split_ctcp(msg, ctcp, sizeof(ctcp), args, sizeof(args));

        return bot_on_ctcp_response(bot, prefix, target, ctcp, args);
    } else {
        int priv = !irc_is_channel(target);

        return bot_dispatch_event(bot, &(struct mod_event) {
            .type = priv ? EVENT_PRIVATE_NOTICE : EVENT_PUBLIC_NOTICE,
            .message = {
                .prefix = prefix,
                .target = priv ? NULL : target,
                .msg = msg
            }
        });
    }
}

int bot_on_join(void *arg, const char *prefix, const char *channel)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_JOIN,
        .join = {
            .prefix = prefix,
            .channel = channel
        }
    });
}

int bot_on_part(void *arg,
        const char *prefix,
        const char *channel,
        const char *reason)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_PART,
        .part = {
            .prefix = prefix,
            .channel = channel,
            .msg = reason
        }
    });
}

int bot_on_quit(void *arg, const char *prefix, const char *reason)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_QUIT,
        .quit = {
            .prefix = prefix,
            .msg = reason
        }
    });
}

int bot_on_kick(void *arg,
        const char *prefix_kicker,
        const char *prefix_kicked,
        const char *channel,
        const char *reason)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_KICK,
        .kick = {
            .prefix_kicker = prefix_kicker,
            .prefix_kicked = prefix_kicked,
            .channel = channel,
            .msg = reason
        }
    });
}

int bot_on_nick(void *arg, const char *prefix_old, const char *prefix_new)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_NICK,
        .nick = {
            .prefix_old = prefix_old,
            .prefix_new = prefix_new
        }
    });
}

int bot_on_invite(void *arg, const char *prefix, const char *channel)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_INVITE,
        .invite = {
            .prefix = prefix,
            .channel = channel
        }
    });
}

int bot_on_topic(void *arg,
        const char *prefix,
        const char *channel,
        const char *topic_old,
        const char *topic_new)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_TOPIC,
        .topic = {
            .prefix = prefix,
            .channel = channel,
            .topic_old = topic_old,
            .topic_new = topic_new
        }
    });
}

int bot_on_mode_set(void *arg,
        const char *prefix,
        const char *channel,
        char mode,
        const char *target)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_CHANNEL_MODE_SET,
        .mode_change = {
            .prefix = prefix,
            .channel = channel,
            .mode = mode,
            .arg = target
        }
    });
}

int bot_on_mode_unset(void *arg,
        const char *prefix,
        const char *channel,
        char mode,
        const char *target)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_CHANNEL_MODE_UNSET,
        .mode_change = {
            .prefix = prefix,
            .channel = channel,
            .mode = mode,
            .arg = target
        }
    });
}

int bot_on_modes(void *arg,
        const char *prefix,
        const char *channel,
        const char *modes)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_CHANNEL_MODES,
        .modes = {
            .prefix = prefix,
            .channel = channel,
            .modes = modes
        }
    });
}

int bot_on_idle(void *arg, time_t lastidle)
{
    return bot_dispatch_event(arg, &(struct mod_event){
        .type = EVENT_IDLE,
        .idle = {
            .last = lastidle
        }
    });
}

int bot_on_connect(void *arg)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_CONNECT
    });
}

int bot_on_disconnect(void *arg)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_DISCONNECT
    });
}


int bot_on_ctcp(struct bot *bot,
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args)
{
#if 0
    struct irc_prefix_parts user;
    struct irc_message response;
#endif
    int priv = !irc_is_channel(target);

#if 0
    irc_split_prefix(&user, prefix);

    if (!strcmp(ctcp, "ACTION")) {
        /* Core Pseudo-CTCP: ACTION, don't treat as real CTCP */
        return bot_on_action(bot, prefix, target, args);
    } else if (!strcmp(ctcp, "LOAD_MODULE")) {
        if (!mod_load(bot, args)) {
            struct mod_loaded *mod = mod_get(bot, args);

            irc_mkctcp_response(&response, user.nick, ctcp,
                    "Successfully loaded module '%s' (%s)",
                        mod->path, mod->state->name);
        } else {
            irc_mkctcp_response(&response, user.nick, ctcp,
                "Failed loading module './mod_%s.so'", args);
        }

        sess_sendmsg(bot->sess, &response);
    } else if (!strcmp(ctcp, "UNLOAD_MODULE")) {
        struct mod_loaded *mod = mod_get(bot, args);

        if (mod) {
            if (!mod_unload(bot, mod))
                irc_mkctcp_response(&response, user.nick, ctcp,
                    "Successfully unloaded module './mod_%s.so'", args);
            else
                irc_mkctcp_response(&response, user.nick, ctcp,
                    "Failed unloading module '%s'", args);
        } else {
            irc_mkctcp_response(&response, user.nick, ctcp,
                "No such module './mod_%s.so'", args);
        }

        sess_sendmsg(bot->sess, &response);
    } else if (!strcmp(ctcp, "MODULES")) {
        struct list *modptr = NULL;
        struct irc_message response;
        int n = 0;

        LIST_FOREACH(bot->modules, modptr) {
            struct mod_loaded *mod = modptr->data;

            irc_mkctcp_response(&response, user.nick, ctcp,
                "#%d: %s (%s) - '%s'",
                    ++n, mod->state->name, mod->path, mod->state->descr);
            sess_sendmsg(bot->sess, &response);
        }

        irc_mkctcp_response(&response, user.nick, ctcp, "%d modules loaded", n);
        sess_sendmsg(bot->sess, &response);
    }
#endif

    return bot_dispatch_event(bot, &(struct mod_event) {
        .type = priv ? EVENT_PRIVATE_CTCP_REQUEST : EVENT_PUBLIC_CTCP_REQUEST,
        .command = {
            .prefix = prefix,
            .target = priv ? NULL : target,
            .command = ctcp,
            .args = args
        }
    });
}

int bot_on_ctcp_response(struct bot *bot,
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args)
{
    int priv = !irc_is_channel(target);

    if (!strcmp(ctcp, "ACTION"))
        return bot_on_action(bot, prefix, target, args);

    return bot_dispatch_event(bot, &(struct mod_event) {
        .type = priv ? EVENT_PRIVATE_CTCP_RESPONSE : EVENT_PUBLIC_CTCP_RESPONSE,
        .command = {
            .prefix = prefix,
            .target = priv ? NULL : target,
            .command = ctcp,
            .args = args
        }
    });
}

int bot_on_action(struct bot *bot,
        const char *prefix,
        const char *target,
        const char *msg)
{
    int priv = !irc_is_channel(target);

    return bot_dispatch_event(bot, &(struct mod_event) {
        .type = priv ? EVENT_PRIVATE_ACTION : EVENT_PUBLIC_ACTION,
        .message = {
            .prefix = prefix,
            .target = priv ? NULL : target,
            .msg = msg
        }
    });
}
