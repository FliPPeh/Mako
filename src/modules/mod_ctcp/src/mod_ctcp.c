#include <string.h>

#include "module/module.h"
#include "irc/irc.h"
#include "irc/util.h"

#include "util/log.h"

/*
 * Provide standard CTCP replies that are nonessential for normal IRC usage
 */
int ctcp_handle_ctcp(
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args);

struct mod mod_info = {
    .name = "CTCP",
    .descr = "Provide default CTCP replies",

    .hooks = M(EVENT_PRIVATE_CTCP_REQUEST)
};

int mod_init()
{
    return 0;
}

int mod_exit()
{
    return 0;
}

int mod_handle_event(struct mod_event *event)
{
    switch (event->type) {
    case EVENT_PRIVATE_CTCP_REQUEST:
        ;; /* hack! */
        struct mod_event_command *req = &event->command;

        return ctcp_handle_ctcp(
                req->prefix,
                req->target,
                req->command,
                req->args);
    default:
        /* We may have bigger problems if this is ever reached... */
        log_warn_n("mod_ctcp", "Called for unknown event %X", event->type);
        break;
    }

    return 0;
}

int ctcp_handle_ctcp(
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args)
{
    struct irc_message response;
    struct irc_prefix_parts user;

    irc_split_prefix(&user, prefix);

    if (!strcmp(ctcp, "PING")) {
        irc_mkctcp_response(&response, user.nick, ctcp, "%s", args);
        bot_send_message(BOTREF, &response);

    } else if (!strcmp(ctcp, "VERSION")) {
        irc_mkctcp_response(&response, user.nick, ctcp, "TinyBot v0xC0FFEE");
        bot_send_message(BOTREF, &response);

    }

    return 0;
}
