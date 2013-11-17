#include "bot/helpers.h"

#include "bot/bot.h"
#include "irc/util.h"

#include <stdarg.h>
#include <stdio.h>

void privmsg(struct bot *b, const char *target, const char *fmt, ...)
{
    struct irc_message msg;

    va_list args;

    va_start(args, fmt);
    irc_vmkdirectmessage(&msg, "PRIVMSG", target, fmt, args);
    va_end(args);

    bot_send_message(b, &msg);
}

void notice(struct bot *b, const char *target, const char *fmt, ...)
{
    struct irc_message msg;

    va_list args;

    va_start(args, fmt);
    irc_vmkdirectmessage(&msg, "NOTICE", target, fmt, args);
    va_end(args);

    bot_send_message(b, &msg);
}

void respond(struct bot *b,
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

    privmsg(b, irc_proper_target(target, rtarget), "%s: %s", rtarget, buf);
}
