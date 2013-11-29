#ifndef IRC_H
#define IRC_H

#include <time.h>

/*
 * Rationale for using static buffer sizes:
 *
 * IRC has a few well defined limits, so we can assume that certain strings
 * or arrays of strings will not realistically surpass a certain size, so we
 * can save a malloc() and the obligatory error check in favour of a statically
 * known size.
 */
#define IRC_MESSAGE_MAX         512
#define IRC_PARAM_COUNT_MAX     16
#define IRC_PARAM_MAX           128
#define IRC_PREFIX_MAX          128
#define IRC_COMMAND_MAX         16
#define IRC_TRAILING_MAX        512
#define IRC_FLAGS_MAX           32
#define IRC_CHANNEL_MAX         128
#define IRC_TOPIC_MAX           512
#define IRC_CHANNEL_PREFIX_MAX  16

#define IRC_NICK_MAX 32
#define IRC_USER_MAX 16
#define IRC_REAL_MAX 64
#define IRC_HOST_MAX 80

#include <libutil/container/hashtable.h>
#include <libutil/container/list.h>

#include "irc/channel.h"

/*
 * Used to classify IRC modes to correctly parse arguments.
 */
enum irc_mode_style
{
    MODE_LIST,
    MODE_REQARG,
    MODE_SETARG,
    MODE_NOARG
};

struct irc_message
{
    char prefix[IRC_PREFIX_MAX];
    char command[IRC_COMMAND_MAX];
    char params[IRC_PARAM_COUNT_MAX][IRC_PARAM_MAX];
    int paramcount;

    char msg[IRC_TRAILING_MAX];
};

/*
 * IRC message handling
 */
int irc_parse_message(const char *line, struct irc_message *msg);
void irc_print_message(const struct irc_message *i);

unsigned irc_message_to_string(const struct irc_message *i,
                               char *dest,
                               size_t n);

unsigned irc_message_size(const struct irc_message *i);

/*
 * User comparison based on nick/host
 */
int irc_user_cmp(const char *a, const char *b);

#endif /* defined IRC_H */
