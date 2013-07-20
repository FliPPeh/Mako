#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bot/bot.h"
#include "bot/admins.h"

#include "util/list.h"
#include "util/log.h"
#include "util/util.h"


void admins_load(struct bot *bot, const char *file)
{
    char linebuf[512] = {0};
    FILE *f;

    if (!(f = fopen(file, "r"))) {
        log_perror("fopen()", LOG_ERROR);
        return;
    } else {
        while (fgets(linebuf, sizeof(linebuf), f)) {
            if (strlen(linebuf) > 0) {
                char *line = strstrp(linebuf);
                char *ptr = NULL;

                struct admin tmp;
                memset(&tmp, 0, sizeof(tmp));

                if ((ptr = strpart(line, ':',
                                tmp.name, sizeof(tmp.name) - 1,
                                tmp.mask, sizeof(tmp.mask) - 1))) {

                    admins_add(bot, tmp.name, tmp.mask);
                    log_debug("Name: '%s', RegEx: '%s'", tmp.name, tmp.mask);
                }
            }

            memset(linebuf, 0, sizeof(linebuf));
        }
    }

    fclose(f);
    return;
}

int admins_save(const struct bot *bot, const char *file)
{
    FILE *f;

    if ((f = fopen(file, "w"))) {
        struct list *ptr = NULL;

        LIST_FOREACH(bot->admins, ptr) {
            struct admin *adm = list_data(ptr, struct admin *);

            fprintf(f, "%s:%s\n", adm->name, adm->mask);
        }
    } else {
        log_perror("fopen()", LOG_ERROR);
        return 1;
    }

    fclose(f);
    return 0;
}

struct admin *admins_get(const struct bot *bot, const char *name)
{
    struct list *res = list_find_custom(bot->admins, name, _admin_find_by_name);

    if (res)
        return list_data(res, struct admin *);
    else
        return NULL;
}

struct admin *admins_get_by_mask(const struct bot *bot, const char *pref)
{
    struct list *res = list_find_custom(bot->admins, pref, _admin_find_by_mask);

    if (res)
        return list_data(res, struct admin *);
    else
        return NULL;
}

int admins_add(struct bot *bot, const char *name, const char *mask)
{
    struct admin *adm = admins_get(bot, name);

    if (adm)
        return 1;

    adm = malloc(sizeof(*adm));
    memset(adm, 0, sizeof(*adm));

    strncpy(adm->name, name, sizeof(adm->name) - 1);
    strncpy(adm->mask, mask, sizeof(adm->mask) - 1);

    bot->admins = list_append(bot->admins, adm);
    return 0;
}

int admins_del(struct bot *bot, struct admin *adm)
{
    bot->admins = list_remove(bot->admins, adm, free);
    return 0;
}

int _admin_find_by_name(const void *data, const void *userdata)
{
    return strcasecmp(((struct admin *)data)->name, (const char *)userdata);
}

int _admin_find_by_mask(const void *data, const void *userdata)
{
    return !regex_match(((struct admin *)data)->mask, (const char *)userdata);
}
