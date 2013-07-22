#ifndef _IRC_UTIL_H_
#define _IRC_UTIL_H_

#include <stdarg.h>

#include "irc/irc.h"

/* Simple macro that wraps a string literal between two \1 chars for CTCP. */
#define MKCTCP(lit) "\x01" lit "\x01"

struct irc_prefix_parts
{
    char nick[IRC_NICK_MAX];
    char user[IRC_USER_MAX];
    char host[IRC_HOST_MAX];
};

/*
 * Split a prefix
 */
int irc_split_prefix(struct irc_prefix_parts *dst, const char *prefix);

/*
 * Weird function for easily creating formatted messages.
 *
 * command = the command to set
 * varargs: 2 parts, separated with a NULL.
 *            Part 1: Message params,
 *            Part 2: First arg = format string
 *                    Following = format args
 *
 * ex: irc_mkmessage(&msg, "PRIVMSG", "#channel", NULL, "Hello %s", "world");
 *
 * => PRIVMSG #channel :Hello World
 *
 */
int irc_mkmessage(struct irc_message *dest, const char *command, ...);
int irc_vmkmessage(struct irc_message *dest, const char *command, va_list args);

int irc_mkmessage_params(struct irc_message *dest, ...);
int irc_vmkmessage_params(struct irc_message *dest, va_list args);

int irc_mkmessage_msg(struct irc_message *dest, const char *fmt, ...);
int irc_vmkmessage_msg(struct irc_message *dest, const char *fmt, va_list pfa);

/*
 * Specific command utils
 */
int irc_vmkdirectmessage(struct irc_message *dest,
        const char *type, const char *target, const char *fmt, va_list args);

int irc_mkprivmsg(struct irc_message *dest,
        const char *target, const char *fmt, ...);

int irc_mknotice(struct irc_message *dest,
        const char *target, const char *fmt, ...);

int irc_mkprivmsg_resp(struct irc_message *dest,
        const char *target, const char *rtarget, const char *fmt, ...);

int irc_mkctcp_request(struct irc_message *dest,
        const char *target, const char *ctcp, const char *msgfmt, ...);

int irc_mkctcp_response(struct irc_message *dest,
        const char *target, const char *ctcp, const char *msgfmt, ...);

/*
 * Test commands
 */
int irc_is_channel(const char *arg);

/*
 * Determine proper response target based on original target (orig_target).
 *
 * If original target was a channel, target the channel (opt_chan).
 * If original target was a user (in all likelyhood this program),
 * target the original sender (opt_user).
 */
const char *irc_proper_target(const char *orig_target, const char *opt_user);

#endif /* defined _IRC_UTIL_H_ */
