#ifndef _BOT_H_
#define _BOT_H_

#include "irc/irc.h"
#include "util/list.h"
#include "util/hashtable.h"

/* Default settings */
#define DEFAULT_NICK "Mako"
#define DEFAULT_USER "mako"
#define DEFAULT_REAL "MAKO!"

#define TRIGGERS ":,;"

struct bot
{
    struct irc_session *sess;

    struct hashtable *modules;
    struct hashtable *regusers;
};

int bot_send_message(const struct bot *bot, const struct irc_message *msg);

#endif /* defined _BOT_H_ */