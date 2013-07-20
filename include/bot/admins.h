#ifndef _BOT_ADMINS_H_
#define _BOT_ADMINS_H_

struct admin
{
    char name[256];
    char mask[256];
};


void admins_load(struct bot *bot, const char *file);
int admins_save(const struct bot *bot, const char *file);

struct admin *admins_get(const struct bot *bot, const char *name);
struct admin *admins_get_by_mask(const struct bot *bot, const char *pref);

int admins_add(struct bot *bot, const char *name, const char *mask);
int admins_del(struct bot *bot, struct admin *adm);

int _admin_find_by_name(const void *data, const void *userdata);
int _admin_find_by_mask(const void *data, const void *userdata);

#endif /* defined _BOT_ADMINS_H_ */
