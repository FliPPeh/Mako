#ifndef _BOT_H_
#define _BOT_H_

#include "util/list.h"

/* Default settings */
#define DEFAULT_NICK "Tiny"
#define DEFAULT_USER "rawr"
#define DEFAULT_REAL "Yup"

#define TRIGGERS ":,;"

struct admin
{
    char name[256];
    char mask[256];
};

struct bot
{
    struct irc_session *sess;
    struct list *modules;
    struct list *admins;
};

const char *bot_get_reguser(const struct bot *bot, const char *prefix);

#endif /* defined _BOT_H_ */
