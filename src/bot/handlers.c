#include "bot/handlers.h"
#include "bot/bot.h"
#include "bot/reguser.h"
#include "bot/module.h"
#include "bot/helpers.h"

#include "modules/module.h"

#include "irc/irc.h"
#include "irc/util.h"
#include "irc/session.h"

#include "util/log.h"
#include "util/util.h"

#include <libutil/container/list.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


int bot_split_ctcp(const char *source,
        char *dctcp, size_t sctcp,
        char *dargs, size_t sargs)
{
    char copy[IRC_MESSAGE_MAX] = {0};
    char *line = NULL;
    char *ptr = NULL;


    strncpy(copy, source, sizeof(copy) - 1);

    memset(dctcp, 0, sctcp);
    memset(dargs, 0, sargs);

    line = strstrp(copy);

    if (!(ptr = strpart(line, ' ', dctcp, sctcp - 1, dargs, sargs - 1))) {
        strncpy(dctcp, line, sctcp - 1);
    }

    return 0;
}

int bot_dispatch_event(struct bot *bot, struct mod_event *ev)
{
    struct hashtable_iterator iter;
    void *k;
    void *v;

    hashtable_iterator_init(&iter, bot->modules);
    while (hashtable_iterator_next(&iter, &k, &v))
        if (((struct mod_loaded *)v)->state->hooks & M(ev->type))
            ((struct mod_loaded *)v)->handler_func(ev);

    return 0;
}

int bot_handle_command(struct bot *bot,
        const char *prefix,
        const char *target,
        const char *cmd,
        const char *args)
{
    int priv = !irc_is_channel(target);
    struct reguser *usr = NULL;

    if ((usr = reguser_find(bot, prefix))
            && reguser_match(usr, M(FLAG_MASTER), CK_MIN)) {

        struct irc_prefix_parts user;

        irc_split_prefix(&user, prefix);

        if (!strcmp(cmd, "load_so")) {
            if (!mod_load(bot, args)) {
                struct mod_loaded *mod = mod_get(bot, args);

                respond(bot, target, user.nick,
                        "Successfully loaded module '%s' (%s)",
                            mod->path, mod->state->name);
            } else {
                respond(bot, target, user.nick,
                        "Failed loading module './%s.so'", args);
            }
        } else if (!strcmp(cmd, "unload_so")) {
            struct mod_loaded *mod = mod_get(bot, args);

            if (mod) {
                if (!mod_unload(bot, mod))
                    respond(bot, target, user.nick,
                            "Successfully unloaded module './%s.so'", args);
                else
                    respond(bot, target, user.nick,
                            "Failed unloading module '%s'", args);
            } else {
                respond(bot, target, user.nick,
                        "No such module './%s.so'", args);
            }
        } else if (!strcmp(cmd, "reload_so")) {
            struct mod_loaded *mod = mod_get(bot, args);
            bool unloaded = false;

            if (mod != NULL)
                unloaded = !mod_unload(bot, mod);

            if (!mod_load(bot, args)) {
                struct mod_loaded *mod = mod_get(bot, args);

                if (unloaded)
                    respond(bot, target, user.nick,
                        "Successfully reloaded module '%s' (%s)",
                            mod->path, mod->state->name);
                else
                    respond(bot, target, user.nick,
                        "Successfully loaded module '%s' (%s)",
                            mod->path, mod->state->name);
            } else {
                respond(bot, target, user.nick,
                    "Failed to load module '%s'", args);
            }

        } else if (!strcmp(cmd, "echo")) {
            respond(bot, target, user.nick, "%s", args);
        }
    }

    return bot_dispatch_event(bot, &(struct mod_event) {
        .type = priv ? EVENT_PRIVATE_COMMAND : EVENT_PUBLIC_COMMAND,
        .event = {
            .command = {
                .prefix = prefix,
                .target = priv ? NULL : target,
                .command = cmd,
                .args = args
            }
        }
    });
}

int bot_on_event(void *arg, const struct irc_message *m)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_RAW,
        .event = {
            .raw = {
                .msg = m
            }
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

            /* Start of the line after strstrp() */
            char *start = NULL;

            /* Result or strpart() */
            char *ptr = NULL;

            /* Destination buffers for strpart() */
            char cmd[32] = {0};
            char args[256] = {0};

            /* Working copy for strstrp() and strpart() */
            char copy[IRC_MESSAGE_MAX] = {0};


            /* Copy message (offset past trigger) to working copy */
            strncpy(copy, msg + strlen(bot->sess->nick) + 1, sizeof(copy) - 1);

            /* Strip whitespace, if any */
            start = strstrp(copy);

            /* Split args, if any */
            if ((ptr = strpart(start, ' ',
                            cmd, sizeof(cmd) - 1,
                            args, sizeof(args) - 1))) {
                return bot_handle_command(bot, prefix, target, cmd, args);
            } else {
                return bot_handle_command(bot, prefix, target, start, NULL);
            }

        } else {
            int priv = !irc_is_channel(target);

            return bot_dispatch_event(bot, &(struct mod_event) {
                .type = priv ? EVENT_PRIVATE_MESSAGE : EVENT_PUBLIC_MESSAGE,
                .event = {
                    .message = {
                        .prefix = prefix,
                        .target = priv ? NULL : target,
                        .msg = msg
                    }
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
            .event = {
                .message = {
                    .prefix = prefix,
                    .target = priv ? NULL : target,
                    .msg = msg
                }
            }
        });
    }
}

int bot_on_join(void *arg, const char *prefix, const char *channel)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_JOIN,
        .event = {
            .join = {
                .prefix = prefix,
                .channel = channel
            }
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
        .event = {
            .part = {
                .prefix = prefix,
                .channel = channel,
                .msg = reason
            }
        }
    });
}

int bot_on_quit(void *arg, const char *prefix, const char *reason)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_QUIT,
        .event = {
            .quit = {
                .prefix = prefix,
                .msg = reason
            }
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
        .event = {
            .kick = {
                .prefix_kicker = prefix_kicker,
                .prefix_kicked = prefix_kicked,
                .channel = channel,
                .msg = reason
            }
        }
    });
}

int bot_on_nick(void *arg, const char *prefix_old, const char *prefix_new)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_NICK,
        .event = {
            .nick = {
                .prefix_old = prefix_old,
                .prefix_new = prefix_new
            }
        }
    });
}

int bot_on_invite(void *arg, const char *prefix, const char *channel)
{
    return bot_dispatch_event(arg, &(struct mod_event) {
        .type = EVENT_INVITE,
        .event = {
            .invite = {
                .prefix = prefix,
                .channel = channel
            }
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
        .event = {
            .topic = {
                .prefix = prefix,
                .channel = channel,
                .topic_old = topic_old,
                .topic_new = topic_new
            }
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
        .event = {
            .mode_change = {
                .prefix = prefix,
                .channel = channel,
                .mode = mode,
                .arg = target
            }
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
        .event = {
            .mode_change = {
                .prefix = prefix,
                .channel = channel,
                .mode = mode,
                .arg = target
            }
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
        .event = {
            .modes = {
                .prefix = prefix,
                .channel = channel,
                .modes = modes
            }
        }
    });
}

int bot_on_idle(void *arg, time_t lastidle)
{
    return bot_dispatch_event(arg, &(struct mod_event){
        .type = EVENT_IDLE,
        .event = {
            .idle = {
                .last = lastidle
            }
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
    int priv = !irc_is_channel(target);

    return bot_dispatch_event(bot, &(struct mod_event) {
        .type = priv ? EVENT_PRIVATE_CTCP_REQUEST : EVENT_PUBLIC_CTCP_REQUEST,
        .event = {
            .command = {
                .prefix = prefix,
                .target = priv ? NULL : target,
                .command = ctcp,
                .args = args
            }
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
        .event = {
            .command = {
                .prefix = prefix,
                .target = priv ? NULL : target,
                .command = ctcp,
                .args = args
            }
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
        .event = {
            .message = {
                .prefix = prefix,
                .target = priv ? NULL : target,
                .msg = msg
            }
        }
    });
}
