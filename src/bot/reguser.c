#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bot/bot.h"
#include "bot/reguser.h"

#include "util/list.h"
#include "util/log.h"
#include "util/util.h"


void regusers_load(struct bot *bot, const char *file)
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

                struct reguser tmp;
                memset(&tmp, 0, sizeof(tmp));

                if ((ptr = strpart(line, ':',
                                tmp.name, sizeof(tmp.name) - 1,
                                tmp.mask, sizeof(tmp.mask) - 1))) {

                    reguser_add(bot, tmp.name, tmp.mask);
                    log_debug("Name: '%s', RegEx: '%s'", tmp.name, tmp.mask);
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
            struct reguser *adm = list_data(ptr, struct reguser *);

            fprintf(f, "%s:%s\n", adm->name, adm->mask);
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

struct reguser *reguser_get_by_mask(const struct bot *bot, const char *pre)
{
    struct list *r = list_find_custom(
            bot->regusers, pre, _reguser_find_by_mask);

    if (r)
        return list_data(r, struct reguser *);
    else
        return NULL;
}

int reguser_add(struct bot *bot, const char *name, const char *mask)
{
    struct reguser *adm = reguser_get(bot, name);

    if (adm)
        return 1;

    adm = malloc(sizeof(*adm));
    memset(adm, 0, sizeof(*adm));

    strncpy(adm->name, name, sizeof(adm->name) - 1);
    strncpy(adm->mask, mask, sizeof(adm->mask) - 1);

    bot->regusers = list_append(bot->regusers, adm);
    return 0;
}

int reguser_del(struct bot *bot, struct reguser *adm)
{
    bot->regusers = list_remove(bot->regusers, adm, free);
    return 0;
}

int _reguser_find_by_name(const void *data, const void *userdata)
{
    return strcasecmp(((struct reguser *)data)->name, (const char *)userdata);
}

int _reguser_find_by_mask(const void *data, const void *userdata)
{
    return !regex_match(((struct reguser *)data)->mask, (const char *)userdata);
}
