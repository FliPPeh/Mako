#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bot/bot.h"
#include "bot/reguser.h"

#include "util/list.h"
#include "util/log.h"
#include "util/util.h"

/*
 * Generate a few arrays
 */
#define X(e, str, flag) flag,
const char _reguser_flags[] = { FLAGS '\0' };
#undef X

#define X(e, str, flag) str,
const char *_reguser_flags_repr[FLAGS_MAX] = { FLAGS };
#undef X


void regusers_load(struct bot *bot, const char *file)
{
    char linebuf[512] = {0};
    FILE *f;

    if (!(f = fopen(file, "r"))) {
        log_perror("fopen()", LOG_ERROR);
        return;
    } else {
        while (fgets(linebuf, sizeof(linebuf), f)) {
            if (!strlen(linebuf))
                continue;

            char *line = strstrp(linebuf);
            char *p = NULL;

            char a[256] = {0}; /* name */
            char b[33] = {0};  /* flags */
            char c[256] = {0}; /* mask */
            char r[512] = {0}; /* rest */

            if ((p = strpart(line, ':', a, sizeof(a) - 1, r, sizeof(r) - 1))) {
                if (strpart(p, ':',  b, sizeof(b) - 1, c, sizeof(c) - 1)) {

                    reguser_add(bot, a, _reguser_strtoflg(b), c);
                    log_debug("Name: '%s', RegEx: '%s', Flags: %d",
                            a, c, _reguser_strtoflg(b));
                }
            }

            memset(linebuf, 0, sizeof(linebuf));
        }
    }

    fclose(f);
    return;
}

int regusers_save(const struct bot *bot, const char *file)
{
    FILE *f;

    if ((f = fopen(file, "w"))) {
        struct list *ptr = NULL;

        LIST_FOREACH(bot->regusers, ptr) {
            char flgstr[33] = {0};
            struct reguser *usr = list_data(ptr, struct reguser *);

            _reguser_flgtostr(usr->flags, flgstr);
            fprintf(f, "%s:%s:%s\n", usr->name, flgstr, usr->mask);
        }
    } else {
        log_perror("fopen()", LOG_ERROR);
        return 1;
    }

    fclose(f);
    return 0;
}

struct reguser *reguser_get(const struct bot *bot, const char *name)
{
    struct list *r = list_find_custom(
            bot->regusers, name, _reguser_find_by_name);

    if (r)
        return list_data(r, struct reguser *);
    else
        return NULL;
}

struct reguser *reguser_find(const struct bot *bot, const char *pre)
{
    struct list *r = list_find_custom(
            bot->regusers, pre, _reguser_find_by_mask);

    if (r)
        return list_data(r, struct reguser *);
    else
        return NULL;
}

int reguser_add(struct bot *bot, const char *name, uint32_t fl, const char *m)
{
    struct reguser *usr = reguser_get(bot, name);

    if (usr)
        return 1;

    usr = malloc(sizeof(*usr));
    memset(usr, 0, sizeof(*usr));

    strncpy(usr->name, name, sizeof(usr->name) - 1);
    usr->flags = fl;
    strncpy(usr->mask, m, sizeof(usr->mask) - 1);

    bot->regusers = list_append(bot->regusers, usr);
    return 0;
}

int reguser_del(struct bot *bot, struct reguser *usr)
{
    bot->regusers = list_remove(bot->regusers, usr, free);
    return 0;
}

/*
 * Query flags
 */
int reguser_match(const struct reguser *usr, uint32_t t, enum reguser_check c)
{
    switch (c) {
    case CK_ALL:
        return ((usr->flags & t) == t);

    case CK_ANY:
        return ((usr->flags & t) > 0u);

    case CK_MIN:
        return usr->flags >= t;
    }
};

const char *reguser_flagstr(const struct reguser *usr)
{
    _reguser_flgtostr(usr->flags, ((struct reguser *)usr)->_flagstr);
    return usr->_flagstr;
}

/*
 * Set and unset flags
 */
void reguser_set_flags(struct reguser *usr, uint32_t flags)
{
    usr->flags |= flags;
}

void reguser_unset_flags(struct reguser *usr, uint32_t flags)
{
    usr->flags &= ~flags;
}


/*
 * Internals
 */
int _reguser_find_by_name(const void *data, const void *userdata)
{
    return strcasecmp(((struct reguser *)data)->name, (const char *)userdata);
}

int _reguser_find_by_mask(const void *data, const void *userdata)
{
    return !regex_match(((struct reguser *)data)->mask, (const char *)userdata);
}

uint32_t _reguser_strtoflg(char src[static 33])
{
    uint32_t res = 0;

    for (int i = FLAGS_MAX - 1; i >= 0; --i)
        if (strchr(src, _reguser_flags[i]))
            res |= M(i);

    return res;
}

void _reguser_flgtostr(uint32_t flags, char dst[static 33])
{
    size_t stroff = 0;

    memset(dst, 0, 33 * sizeof(char));

    for (int i = FLAGS_MAX - 1; i >= 0; --i)
        if (flags & M(i))
            dst[stroff++] = _reguser_flags[i];
}
