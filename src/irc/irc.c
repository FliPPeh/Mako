#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "irc/irc.h"
#include "util/util.h"
#include "util/list.h"


/*
 * IRC message handling
 */
int irc_parse_message(const char *line, struct irc_message *msg)
{
    const char *prev = line;
    const char *next = line;
    size_t len;

    memset(msg, 0, sizeof(*msg));

    /* Check for prefix */
    if (*prev == ':') {
        ++prev; /* skip the ':' */

        /* Prefix must not be the last part in a message */
        if (!(next = strchr(prev, ' ')))
            return 1;

        /* Calculate token lenght and skip the space */
        len = (size_t)(next - prev);
        while (*next == ' ')
            ++next;

        strncpy(msg->prefix, prev, MIN(len, sizeof(msg->prefix) - 1));
        prev = next;
    }

    /* Command also must not be the last argument in a message */
    if (!(next = strchr(next, ' ')))
        return 1;

    len = (size_t)(next - prev);
    while (*next == ' ')
        ++next;

    strncpy(msg->command, prev, MIN(len, sizeof(msg->command) - 1));

    do {
        prev = next;
        next = strchr(next, ' ') ? strchr(next, ' ') : next + strlen(next);
        len = (size_t)(next - prev);

        while (*next == ' ')
            ++next;

        if (*prev != ':') {
            strncpy(msg->params[msg->paramcount++], prev,
                    MIN(len, sizeof(msg->params[0]) - 1));
        } else {
            ++prev;
            len = strlen(prev);

            strncpy(msg->msg, prev, MIN(len, sizeof(msg->msg) - 1));

            break;
        }
    } while (next < line + strlen(line));

    return 0;
}

void irc_print_message(const struct irc_message *i)
{
    if (strlen(i->prefix))
        printf(":%s ", i->prefix);

    printf("%s", i->command);

    for (int j = 0; j < i->paramcount; j++)
        printf(" %s", i->params[j]);

    if (strlen(i->msg))
        printf(" :%s", i->msg);

    printf("\n");
}


/*
 * User comparison based on nick/host
 */
int irc_user_cmp(const char *a, const char *b)
{
    /*
     * Since nicks are enough to identify someone uniquely on a network,
     * we can skip the whole address part and compare complete prefixes
     * only until the end of the nick part.
     */
    const char *p = NULL;

    size_t reallen_a = (p = strchr(a, '!')) ? (size_t)(p - a) : strlen(a);
    size_t reallen_b = (p = strchr(b, '!')) ? (size_t)(p - b) : strlen(b);
    size_t cmplen = MIN(reallen_a, reallen_b);

    return (reallen_a != reallen_b) ? 1 : strncasecmp(a, b, cmplen);
}

/* Server capabilities */
const char *irc_capability_get(struct list *cl, const char *cap)
{
    struct list *pos = list_find_custom(cl, cap, _irc_cap_by_key);

    if (pos)
        return (list_data(pos, struct irc_capability *))->value;

    return NULL;
}

int irc_capability_set(
        struct list **cl,
        const char *cap,
        const char *val)
{
    *cl = list_append(*cl, _irc_capability_new(cap, val));

    return 0;
}

/* Utility functions */
int _irc_cap_by_key(const void *list, const void *search)
{
    return strcmp(((struct irc_capability *)list)->capability, search);
}

struct irc_capability *_irc_capability_new(const char *key, const char *val)
{
    struct irc_capability *cap = malloc(sizeof(*cap));
    ASSERT(cap != NULL, LOG_ERROR, return NULL);

    strncpy(cap->capability, key, sizeof(cap->capability) - 1);
    strncpy(cap->value, val ? val : "", sizeof(cap->value) - 1);

    return cap;
}
