#ifndef _BOT_MODULE_H
#define _BOT_MODULE_H

#include "bot/bot.h"
#include "modules/module.h"

#define MOD_NAME_MAX 256
#define MOD_PATH_MAX 256

struct mod_loaded
{
    struct mod_loaded *next;

    /*
     * References the global module state within each module. Makes tempering
     * just as possible for rogue modules as it does self-managed hooks for nice
     * modules.
     */
    struct mod *state;

    /*
     * libdl handle
     */
    void *dlhandle;
    int (*handler_func)(struct mod_event *event);

    char name[MOD_NAME_MAX];
    char path[MOD_PATH_MAX]; /* for the moment, the same as "./mod_<name>.so" */
};

int mod_load(struct bot *bot, const char *name);
int mod_unload(struct bot *bot, struct mod_loaded *mod);
void mod_free(void *arg);

struct mod_loaded *mod_get(const struct bot *bot, const char *name);
const struct bot *mod_to_bot(const struct mod *mod);

void *mod_get_symbol(const struct mod_loaded *mod, const char *sym);

int mod_load_autoload(struct bot *bot, const char *file);

#endif /* defined _BOT_MODULE_H_ */
