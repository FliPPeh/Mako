#ifndef _BOT_REGUSER_H_
#define _BOT_REGUSER_H_

struct reguser
{
    char name[256];
    char mask[256];
};

/*
 * Load and save regusers lists
 */
void regusers_load(struct bot *bot, const char *file);
int regusers_save(const struct bot *bot, const char *file);

/*
 * Add and delete regusers
 */
int reguser_add(struct bot *bot, const char *name, const char *mask);
int reguser_del(struct bot *bot, struct reguser *adm);

/*
 * Query regusers
 */
struct reguser *reguser_get(const struct bot *bot, const char *name);
struct reguser *reguser_get_by_mask(const struct bot *bot, const char *pre);

int _reguser_find_by_name(const void *data, const void *userdata);
int _reguser_find_by_mask(const void *data, const void *userdata);

#endif /* defined _BOT_REGUSER_H_ */
