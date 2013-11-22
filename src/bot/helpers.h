#ifndef HELPERS_H
#define HELPERS_H

#include "bot/bot.h"

/*
 * Common helper functions go here.
 */

// No function module prefixes for once.
void privmsg(struct bot *b, const char *target, const char *fmt, ...);
void notice(struct bot *b, const char *target, const char *fmt, ...);

void respond(struct bot *b,
             const char *target,  // channel or user to send the response to
             const char *rtarget, // user to respond to
             const char *fmt, ...);

void ctcp_response(struct bot *b,
                   const char *target,
                   const char *ctcp,
                   const char *fmt, ...);

void ctcp_request(struct bot *b,
                  const char *target,
                  const char *ctcp,
                  const char *fmt, ...);

#endif /* defined HELPERS_H */
