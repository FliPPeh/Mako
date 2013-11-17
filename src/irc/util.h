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


void irc_mkmessage(struct irc_message *dest,
                   const char *cmd,
                   const char *args[], size_t n,
                   const char *fmt, ...);

void irc_vmkmessage(struct irc_message *dest,
                    const char *cmd,
                    const char *args[], size_t n,
                    const char *fmt, va_list vargs);
/*
 * Specific command utils
 */
void irc_vmkdirectmessage(struct irc_message *dest,
                          const char *type,
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

#endif /* defined _IRC_UTIL_H_ */
