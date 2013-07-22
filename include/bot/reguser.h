#ifndef _BOT_REGUSER_H_
#define _BOT_REGUSER_H_

#include <stdint.h>

#include "bot/bot.h"

/*
 * Turns any old number smaller then 32 into a power of two, used for
 * bit field masking
 */
#ifndef M
#   define M(x) (1 << ((x) & 31))
#endif

#define FLAGS \
    X(FRIEND,   "friend",   'f') \
    X(OPERATOR, "operator", 'o') \
    X(MASTER,   "master",   'm') \
    X(OWNER,    "owner",    'a')

/*
 * Generate flag enum
 */
#define X(e, str, flag) FLAG_ ## e,
enum reguser_flag
{
    FLAGS FLAGS_MAX
};
#undef X

enum reguser_check
{
    CK_ALL, /* test *all* flags for a match (and) */
    CK_ANY, /* test *any* flag for a match (or) */
    CK_MIN  /* test if user flags are larger or equal to the test
                value, numerically. Useful only if flags are ordered
                from lowest to highest privilege. */
};

extern const char _reguser_flags[];
extern const char *_reguser_flags_repr[FLAGS_MAX];

/*
 * Represent a registered user ("reguser") identified by a hostmask, an extended
 * POSIX regular expression (like egrep).
 */
struct reguser
{
    char name[256];
    char mask[256];

    uint32_t flags;

    /*
     * Used for the return value of reguser_flagstr(). Saves the caller from
     * allocating their own buffer at the cost of 33 bytes per reguser, which
     * isn't that big of an impact.
     */
    char _flagstr[33];
};

/*
 * Load and save regusers lists
 */
void regusers_load(struct bot *bot, const char *file);
int regusers_save(const struct bot *bot, const char *file);

/*
 * Add and delete regusers
 */
int reguser_add(struct bot *bot, const char *name, uint32_t fl, const char *m);
int reguser_del(struct bot *bot, struct reguser *usr);

/*
 * Query regusers
 */

/* Find by name */
struct reguser *reguser_get(const struct bot *bot, const char *name);

/* Find by mask */
struct reguser *reguser_find(const struct bot *bot, const char *pre);


/*
 * Query flags
 */
int reguser_match(const struct reguser *usr, uint32_t t, enum reguser_check c);

const char *reguser_flagstr(const struct reguser *usr);

/*
 * Set and unset flags
 */
void reguser_set_flags(struct reguser *usr, uint32_t flags);
void reguser_unset_flags(struct reguser *usr, uint32_t flags);

/*
 * Internals
 */
inline int _reguser_find_by_name(const void *data, const void *userdata);
inline int _reguser_find_by_mask(const void *data, const void *userdata);

uint32_t _reguser_strtoflg(const char *src);
void _reguser_flgtostr(uint32_t flags, char dst[static 33]);

#endif /* defined _BOT_REGUSER_H_ */
