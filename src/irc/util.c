#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "irc/irc.h"
#include "irc/util.h"
#include "util/util.h"


/*
 * Split a prefix
 */
int irc_split_prefix(struct irc_prefix_parts *dst, const char *prefix)
{
    const char *excl = strchr(prefix, '!');
    const char *at = strchr(prefix, '@');

    if (!excl || !at)
        return 1;

    memset(dst, 0, sizeof(*dst));

    strncpy(dst->nick, prefix,
            MIN(sizeof(dst->nick) - 1, (size_t)(excl - prefix)));

    strncpy(dst->user, excl + 1,
            MIN(sizeof(dst->user) - 1, (size_t)(at - excl)));

    strncpy(dst->host, at + 1,
            MIN(sizeof(dst->host) - 1, strlen(prefix) - (size_t)(at - prefix)));

    return 0;
}


int irc_mkmessage(struct irc_message *dest, const char *command, ...)
{
    va_list args;
    int ret;

    va_start(args, command);
    ret = irc_vmkmessage(dest, command, args);
    va_end(args);

    return ret;
}

int irc_vmkmessage(struct irc_message *dest, const char *command, va_list args)
{
    const char *fmt = NULL;

    memset(dest, 0, sizeof(*dest));

    strncpy(dest->command, command, sizeof(dest->command) - 1);

    irc_vmkmessage_params(dest, args);

    if ((fmt = va_arg(args, const char *)))
        irc_vmkmessage_msg(dest, fmt, args);

    return 0;
}

int irc_mkmessage_params(struct irc_message *dest, ...)
{
    va_list args;
    int ret;

    va_start(args, dest);
    ret = irc_vmkmessage_params(dest, args);
    va_end(args);

    return ret;
}

int irc_vmkmessage_params(struct irc_message *dest, va_list args)
{
    const char *arg = NULL;

    memset(dest->params, 0, sizeof(dest->params));

    dest->paramcount = 0;
    while ((arg = va_arg(args, const char *)))
        strncpy(dest->params[dest->paramcount++], arg,
                sizeof(dest->params[0]) - 1);

    return 0;
}

int irc_mkmessage_msg(struct irc_message *dest, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = irc_vmkmessage_msg(dest, fmt, args);
    va_end(args);

    return ret;
}

int irc_vmkmessage_msg(struct irc_message *dest, const char *fmt, va_list pfa)
{
    memset(dest->msg, 0, sizeof(dest->msg));

    return vsnprintf(dest->msg, sizeof(dest->msg), fmt, pfa);
}


/*
 * Specific command utils
 */

/*
 * Generic direct message creation. It would probably be shorter to do these
 * 2 lines within mkprivmsg and mknotice, but repeated code is bad code :(
 */
int irc_vmkdirectmessage(struct irc_message *dest,
        const char *type, const char *target, const char *fmt, va_list args)
{
    irc_mkmessage(dest, type, target, NULL, NULL);
    irc_vmkmessage_msg(dest, fmt, args);

    return 0;
}

int irc_mkprivmsg(struct irc_message *dest,
        const char *target, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = irc_vmkdirectmessage(dest, "PRIVMSG", target, fmt, args);
    va_end(args);

    return ret;
}

int irc_mknotice(struct irc_message *dest,
        const char *target, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = irc_vmkdirectmessage(dest, "NOTICE", target, fmt, args);
    va_end(args);

    return ret;
}

int irc_mkprivmsg_resp(struct irc_message *dest,
        const char *target, const char *rtarget, const char *fmt, ...)
{
    va_list args;
    char buf[IRC_MESSAGE_MAX] = {0};

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    return irc_mkprivmsg(dest, target, "%s: %s", rtarget, buf);
}

int irc_mkctcp_request(struct irc_message *dest,
        const char *target, const char *ctcp, const char *msgfmt, ...)
{
    va_list args;
    char buf[IRC_MESSAGE_MAX] = {0};

    if (msgfmt) {
        va_start(args, msgfmt);
        vsnprintf(buf, sizeof(buf), msgfmt, args);
        va_end(args);

        return irc_mkprivmsg(dest, target, MKCTCP("%s %s"), ctcp, buf);
    } else {
        return irc_mkprivmsg(dest, target, MKCTCP("%s"), ctcp);
    }
}

int irc_mkctcp_response(struct irc_message *dest,
        const char *target, const char *ctcp, const char *msgfmt, ...)
{
    va_list args;
    char buf[IRC_MESSAGE_MAX] = {0};

    va_start(args, msgfmt);
    vsnprintf(buf, sizeof(buf), msgfmt, args);
    va_end(args);

    return irc_mknotice(dest, target, MKCTCP("%s %s"), ctcp, buf);
}

/*
 * Test commands
 */
int irc_is_channel(const char *arg)
{
    char channel_valids[] = "&#+!"; /* According to RFC 2811 */

    return (strchr(channel_valids, *arg) != NULL);
}

/*
 * Determine proper response target based on original target (orig_target).
 *
 * If original target was a channel, target it again (orig_target).
 * If original target was a user (in all likelyhood this program),
 * target the original sender (opt_user).
 */
const char *irc_proper_target(const char *orig_target, const char *opt_user)
{
    return (irc_is_channel(orig_target)
        ? orig_target
        : opt_user);
}
