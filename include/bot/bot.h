#ifndef _BOT_H_
#define _BOT_H_

#include "irc/irc.h"
#include "util/list.h"

/* Default settings */
#define DEFAULT_NICK "Tiny"
#define DEFAULT_USER "rawr"
#define DEFAULT_REAL "Yup"

#define TRIGGERS ":,;"

struct bot
{
    struct irc_session *sess;
    struct list *modules;
    struct list *regusers;
};

int bot_send_message(const struct bot *bot, const struct irc_message *msg);

#endif /* defined _BOT_H_ */
