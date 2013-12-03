#include "irc/irc.h"
#include "irc/util.h"
#include "irc/session.h"
#include "util/util.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>


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


void irc_mkmessage(struct irc_message *dest,
                   enum irc_command cmd,
                   const char *args[], size_t n,
                   const char *fmt, ...)
{
    va_list vargs;

    va_start(vargs, fmt);
    irc_vmkmessage(dest, cmd, args, n, fmt, vargs);
    va_end(vargs);
}

void irc_vmkmessage(struct irc_message *dest,
                    enum irc_command cmd,
                    const char *args[], size_t n,
                    const char *fmt, va_list vargs)
{
    memset(dest, 0, sizeof(*dest));

    dest->command = cmd;

    for (size_t i = 0; (i < n) && (i < IRC_PARAM_COUNT_MAX); ++i)
        strncpy(dest->params[i], args[i], sizeof(dest->params[i]) - 1);

    dest->paramcount = n;

    if (fmt != NULL)
        vsnprintf(dest->msg, sizeof(dest->msg), fmt, vargs);
}


/*
 * Specific command utils
 */

/*
 * Generic direct message creation. It would probably be shorter to do these
 * 2 lines within mkprivmsg and mknotice, but repeated code is bad code :(
 */
void irc_vmkdirectmessage(struct irc_message *dest,
                          enum irc_command type,
                          const char *target,
                          const char *fmt, va_list args)
{
    irc_vmkmessage(dest, type, (const char *[]){ target }, 1, fmt, args);
}

void irc_mkprivmsg(struct irc_message *dest,
                   const char *target,
                   const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    irc_vmkdirectmessage(dest, CMD_PRIVMSG, target, fmt, args);
    va_end(args);
}

void irc_mknotice(struct irc_message *dest,
                  const char *target,
                  const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    irc_vmkdirectmessage(dest, CMD_NOTICE, target, fmt, args);
    va_end(args);
}

void irc_mkprivmsg_resp(struct irc_message *dest,
                        const char *target,
                        const char *rtarget,
                        const char *fmt, ...)
{
    va_list args;
    char buf[IRC_MESSAGE_MAX] = {0};

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    irc_mkprivmsg(dest, target, "%s: %s", rtarget, buf);
}

void irc_mkctcp_request(struct irc_message *dest,
                        const char *target,
                        const char *ctcp,
                        const char *msgfmt, ...)
{
    va_list args;
    char buf[IRC_MESSAGE_MAX] = {0};

    if (msgfmt) {
        va_start(args, msgfmt);
        vsnprintf(buf, sizeof(buf), msgfmt, args);
        va_end(args);

        irc_mkprivmsg(dest, target, MKCTCP("%s %s"), ctcp, buf);
    } else {
        irc_mkprivmsg(dest, target, MKCTCP("%s"), ctcp);
    }
}

void irc_mkctcp_response(struct irc_message *dest,
                         const char *target,
                         const char *ctcp,
                         const char *msgfmt, ...)
{
    va_list args;
    char buf[IRC_MESSAGE_MAX] = {0};

    va_start(args, msgfmt);
    vsnprintf(buf, sizeof(buf), msgfmt, args);
    va_end(args);

    irc_mknotice(dest, target, MKCTCP("%s %s"), ctcp, buf);
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

int irc_strwcmp(const char *str, const char *pat)
{
    while (*pat) {
        if (*pat == '?') {
            if (!*str)
                return 1;

            ++str;
            ++pat;
        } else if (*pat == '*') {
            return !((!irc_strwcmp(str, pat+1))
                    || (*str && !irc_strwcmp(str+1, pat)));
        } else {
            if (((*str++) | (1 << 5)) != ((*pat++) | (1 << 5)))
                return 1;
        }
    }

    return !(!*str && !*pat);
}

const struct irc_prefix_parts *irc_get_prefix_parts(const char *prefix)
{
    static struct irc_prefix_parts parts;

    if (irc_split_prefix(&parts, prefix))
        return NULL;

    return &parts;
}

const char *irc_get_nick(const char *prefix)
{
    const struct irc_prefix_parts *parts = irc_get_prefix_parts(prefix);

    if (parts)
        return parts->nick;

    return NULL;
}

const char *irc_get_user(const char *prefix)
{
    const struct irc_prefix_parts *parts = irc_get_prefix_parts(prefix);

    if (parts)
        return parts->user;

    return NULL;
}

const char *irc_get_host(const char *prefix)
{
    const struct irc_prefix_parts *parts = irc_get_prefix_parts(prefix);

    if (parts)
        return parts->host;

    return prefix;
}

void join(struct irc_session *s, const char *chan, const char *key)
{
    struct irc_message msg;

    if (key)
        irc_mkmessage(&msg, CMD_JOIN, (const char *[]){ chan, key }, 2, NULL);
    else
        irc_mkmessage(&msg, CMD_JOIN, (const char *[]){ chan }, 1, NULL);

    sess_sendmsg(s, &msg);
}

void part(struct irc_session *s, const char *chan, const char *reasonfmt, ...)
{
    struct irc_message msg;

    va_list args;

    va_start(args, reasonfmt);
    irc_vmkmessage(&msg, CMD_PART,
            (const char *[]){ chan }, 1, reasonfmt, args);
    va_end(args);

    sess_sendmsg(s, &msg);
}

void quit(struct irc_session *s, const char *reasonfmt, ...)
{
    struct irc_message msg;

    va_list args;

    va_start(args, reasonfmt);
    irc_vmkmessage(&msg, CMD_QUIT, NULL, 0, reasonfmt, args);
    va_end(args);

    sess_sendmsg(s, &msg);
}

void kick(struct irc_session *s, const char *chan,
                                 const char *who,
                                 const char *reasonfmt, ...)
{
    struct irc_message msg;

    va_list args;

    va_start(args, reasonfmt);
    irc_vmkmessage(&msg, CMD_KICK, (const char *[]){ chan, who }, 2,
            reasonfmt, args);
    va_end(args);

    sess_sendmsg(s, &msg);
}

void privmsg(struct irc_session *s, const char *target, const char *fmt, ...)
{
    struct irc_message msg;

    va_list args;

    va_start(args, fmt);
    irc_vmkdirectmessage(&msg, CMD_PRIVMSG, target, fmt, args);
    va_end(args);

    sess_sendmsg(s, &msg);
}

void notice(struct irc_session *s, const char *target, const char *fmt, ...)
{
    struct irc_message msg;

    va_list args;

    va_start(args, fmt);
    irc_vmkdirectmessage(&msg, CMD_NOTICE, target, fmt, args);
    va_end(args);

    sess_sendmsg(s, &msg);
}

void respond(struct irc_session *s,
             const char *target,  /* where the message we're responding to was
                                     received from */
             const char *rtarget, /* user nickname to whom the response is */
             const char *fmt,
             ...)
{
    char buf[IRC_MESSAGE_MAX] = {0};
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    privmsg(s, irc_proper_target(target, rtarget), "%s: %s", rtarget, buf);
}

void ctcp_response(struct irc_session *s,
                   const char *target,
                   const char *ctcp,
                   const char *fmt, ...)
{
    if (fmt != NULL) {
        va_list args;
        char buf[IRC_MESSAGE_MAX] = {0};

        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        notice(s, target, "\x01%s %s\x01", ctcp, buf);
    } else {
        notice(s, target, "\x01%s\x01", ctcp);
    }
}

void ctcp_request(struct irc_session *s,
                  const char *target,
                  const char *ctcp,
                  const char *fmt, ...)
{
    if (fmt != NULL) {
        va_list args;
        char buf[IRC_MESSAGE_MAX] = {0};

        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        privmsg(s, target, "\x01%s %s\x01", ctcp, buf);
    } else {
        privmsg(s, target, "\x01%s\x01", ctcp);
    }
}
