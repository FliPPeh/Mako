#include "bot/bot.h"
#include "bot/module.h"
#include "irc/irc.h"
#include "irc/session.h"
#include "irc/net/socket.h"
#include "util/log.h"
#include "util/util.h"

#include <libutil/container/list.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <dlfcn.h>


int mod_load(struct bot *bot, const char *name)
{
    void *lib_handle = NULL;

    int (*init)();
    int (*modhandler)(struct mod_event *);

    char path[512] = {0};

    struct mod *modstate = NULL;
    struct mod_loaded *mod = mod_get(bot, name);

    if (mod) {
        log_warn("Module '%s' already loaded", name);

        goto exit_err_noalloc;
    }

    snprintf(path, sizeof(path), "./%s.so", name);

    log_debug("Loading module '%s' ('%s')...", name, path);

    if (!(lib_handle = dlopen(path, RTLD_NOW))) {
        log_error("Error loading '%s': %s", path, dlerror());

        goto exit_err_noalloc;
    }

    mod = malloc(sizeof(struct mod_loaded));
    memset(mod, 0, sizeof(*mod));

    mod->dlhandle = lib_handle;
    strncpy(mod->name, name, sizeof(mod->name) - 1);
    strncpy(mod->path, path, sizeof(mod->path) - 1);

    if (!(modstate = (struct mod *)mod_get_symbol(mod, "info"))) {
        log_error("Couldn't locate module root struct");

        goto exit_err;
    } else {
        if (!modstate->name || !modstate->descr)
            log_warn("No module name or description given");

        modstate->bot = bot;
        mod->state = modstate;
    }

    modhandler =
        (int (*)(struct mod_event *))mod_get_symbol(mod, "handle_event");

    if (!modhandler) {
        log_error("Couldn't locate module event handlers");

        goto exit_err;
    } else {
        mod->handler_func = modhandler;
    }

    if ((init = (int (*)())mod_get_symbol(mod, "init"))) {
        if (init()) {
            log_info("Module '%s' has aborted initialization", name);

            goto exit_err;
        }
    }

    hashtable_insert(bot->modules, strdup(mod->name), mod);
    return 0;

exit_err:
    mod_unload(bot, mod);

exit_err_noalloc:
    return 1;
}

int mod_unload(struct bot *bot, struct mod_loaded *mod)
{
    hashtable_remove(bot->modules, mod->name);

    return 0;
}

void mod_free(void *arg)
{
    int (*exit)();
    struct mod_loaded *mod = (struct mod_loaded *)arg;

    if ((exit = (int (*)())mod_get_symbol(mod, "exit")))
        exit();

    log_debug("Unloading '%s'", mod->path);

    dlclose(mod->dlhandle);
    free(mod);
}

struct mod_loaded *mod_get(const struct bot *bot, const char *name)
{
    return hashtable_lookup(bot->modules, name);
}

void *mod_get_symbol(const struct mod_loaded *mod, const char *sym)
{
    char fqsym[512] = {0};
    void *symref = NULL;

    snprintf(fqsym, sizeof(fqsym), "mod_%s", sym);
    log_trace("Looking up '%s'...", fqsym);

    /*
     * Clear a previous error
     */
    dlerror();
    if (!(symref = dlsym(mod->dlhandle, fqsym))) {
        const char *error = dlerror();

        if (error)
            log_error("%s", error);
    }

    return symref;
}

int mod_load_autoload(struct bot *bot, const char *file)
{
    char linebuf[MOD_NAME_MAX] = {0};
    FILE *f;

    if (!(f = fopen(file, "r"))) {
        log_perror("fopen()", LOG_ERROR);
        return 1;

    } else {
        while (fgets(linebuf, sizeof(linebuf), f)) {
            if (strlen(linebuf) > 0) {
                char *line = strstrp(linebuf);

                if (mod_load(bot, line))
                    log_error("Failed loading './%s.so'", line);
            }

            memset(linebuf, 0, sizeof(linebuf));
        }

        fclose(f);
        return 0;
    }
}
