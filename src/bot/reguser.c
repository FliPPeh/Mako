#include "bot/bot.h"
#include "bot/reguser.h"
#include "util/log.h"
#include "util/util.h"

#include <libutil/container/list.h>

#include <strings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
                    log_debug("Name: '%s', RegEx: '%s', Flags: %s",
                            a, c, b);
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
        struct hashtable_iterator iter;
        void *k;
        void *v;

        hashtable_iterator_init(&iter, bot->regusers);
        while (hashtable_iterator_next(&iter, &k, &v)) {
            char flgstr[33] = {0};
            struct reguser *usr = v;

            _reguser_flgtostr(usr->flags, flgstr, sizeof(flgstr));
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
    return hashtable_lookup(bot->regusers, name);
}

struct reguser *reguser_find(const struct bot *bot, const char *pre)
{
    struct hashtable_iterator iter;
    void *k;
    void *v;

    hashtable_iterator_init(&iter, bot->regusers);
    while (hashtable_iterator_next(&iter, &k, &v))
        if (regex_match(((struct reguser *)v)->mask, pre))
            return v;

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

    hashtable_insert(bot->regusers, strdup(usr->name), usr);
    return 0;
}

int reguser_del(struct bot *bot, struct reguser *usr)
{
    hashtable_remove(bot->regusers, usr->name);
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

    return 0;
};

const char *reguser_flagstr(const struct reguser *usr)
{
    _reguser_flgtostr(usr->flags,
            ((struct reguser *)usr)->_flagstr, sizeof(usr->_flagstr));

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
uint32_t _reguser_strtoflg(const char *src)
{
    uint32_t res = 0;

    for (int i = FLAGS_MAX - 1; i >= 0; --i)
        if (strchr(src, _reguser_flags[i]))
            res |= M(i);

    return res;
}

void _reguser_flgtostr(uint32_t flags, char *dst, size_t dsts)
{
    size_t stroff = 0;

    memset(dst, 0, dsts);

    for (int i = FLAGS_MAX - 1; (stroff < dsts) && (i >= 0); --i)
        if (flags & M(i))
            dst[stroff++] = _reguser_flags[i];
}
