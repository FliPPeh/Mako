#ifndef IRC_UTIL_H
#define IRC_UTIL_H

#include "irc/irc.h"

#include <stdarg.h>

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

void irc_mkmessage(struct irc_message *dest,
                   enum irc_command cmd,
                   const char *args[], size_t n,
                   const char *fmt, ...);

void irc_vmkmessage(struct irc_message *dest,
                    enum irc_command cmd,
                    const char *args[], size_t n,
                    const char *fmt, va_list vargs);
/*
 * Specific command utils
 */
void irc_vmkdirectmessage(struct irc_message *dest,
                          enum irc_command cmd,
                          const char *target,
                          const char *fmt, va_list args);

void irc_mkprivmsg(struct irc_message *dest,
                   const char *target,
                   const char *fmt, ...);

void irc_mknotice(struct irc_message *dest,
                  const char *target,
                  const char *fmt, ...);

void irc_mkprivmsg_resp(struct irc_message *dest,
                        const char *target,
                        const char *rtarget,
                        const char *fmt, ...);

void irc_mkctcp_request(struct irc_message *dest,
                        const char *target,
                        const char *ctcp,
                        const char *msgfmt, ...);

void irc_mkctcp_response(struct irc_message *dest,
                         const char *target,
                         const char *ctcp,
                         const char *msgfmt, ...);

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

/*
 * Perform a case insensitive wildcard comparation with a pattern using the
 * common '*' and '?' wildcards. Returns 0 on success, and nonzero on a mis-
 * match. Mostly used for matching prefix masks against prefixes.
 */
int irc_strwcmp(const char *str, const char *pattern);

/*
 * Convenience functions the return the individual parts of a prefix. They all
 * use a static buffer for storing to result and return a pointer to it, so
 * multiple invocations to it in the same evaluation will yield the same result
 * and are only save if different parts with the same prefix are accessed or
 * different prefixes are split and copied into different structures, i.e
 *
 *   struct irc_prefix_parts a = *irc_get_prefix_parts(prefix_a);
 *   struct irc_prefix_parts b = *irc_get_prefix_parts(prefix_b);
 *
 * irc_get_nick() and irc_get_user() return NULL if prefix is not a user prefix,
 * irc_get_host() will return the user host or server host in any case.
 */
const struct irc_prefix_parts *irc_get_prefix_parts(const char *prefix);
const char *irc_get_nick(const char *prefix);
const char *irc_get_user(const char *prefix);
const char *irc_get_host(const char *prefix);

/*
 * Helper functions for common commands.
 */
void join(struct irc_session *s, const char *chan, const char *key);
void part(struct irc_session *s, const char *chan, const char *reasonfmt, ...);
void quit(struct irc_session *s, const char *reasonfmt, ...);
void kick(struct irc_session *s, const char *chan,
                                 const char *who,
                                 const char *reasonfmt, ...);


void privmsg(struct irc_session *s, const char *target, const char *fmt, ...);
void notice(struct irc_session *s, const char *target, const char *fmt, ...);

void respond(struct irc_session *s,
             const char *target,  // channel or user to send the response to
             const char *rtarget, // user to respond to
             const char *fmt, ...);

void ctcp_response(struct irc_session *s,
                   const char *target,
                   const char *ctcp,
                   const char *fmt, ...);

void ctcp_request(struct irc_session *s,
                  const char *target,
                  const char *ctcp,
                  const char *fmt, ...);


#endif /* defined IRC_UTIL_H */
