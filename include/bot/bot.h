#ifndef _BOT_H_
#define _BOT_H_

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
};

#endif /* defined _BOT_H_ */
