#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <dlfcn.h>

#include "bot/module.h"
#include "util/list.h"
#include "util/log.h"

#include "irc/irc.h"
#include "irc/session.h"

#include "irc/net/socket.h"


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

    if (!(lib_handle = dlopen(path, RTLD_LAZY))) {
        log_error("Error loading '%s': %s", path, dlerror());

        goto exit_err_noalloc;
    }

    mod = malloc(sizeof(struct mod_loaded));
    mod->dlhandle = lib_handle;
    strncpy(mod->name, name, sizeof(mod->name) - 1);
    strncpy(mod->path, path, sizeof(mod->path) - 1);

    bot->modules = list_append(bot->modules, mod);

    if (!(modstate = mod_get_symbol(mod, "info"))) {
        log_error("Couldn't locate module root struct");

        goto exit_err;
    } else {
        if (!modstate->name || !modstate->descr)
            log_warn("No module name or description given");

        modstate->backref = bot;
        mod->state = modstate;
    }

    if (!(modhandler = mod_get_symbol(mod, "handle_event"))) {
        log_error("Couldn't locate module event handlers");

        goto exit_err;
    } else {
        mod->handler_func = modhandler;
    }

    if ((init = mod_get_symbol(mod, "init"))) {
        if (init()) {
            log_info("Module '%s' has aborted initialization", name);

            goto exit_err;
        }
    }

    return 0;

exit_err:
    mod_unload(bot, mod);

exit_err_noalloc:
    return 1;
}

int mod_unload(struct bot *bot, struct mod_loaded *mod)
{
    bot->modules = list_remove(bot->modules, mod, mod_free);

    return 0;
}

void mod_free(void *arg)
{
    int (*exit)();
    struct mod_loaded *mod = arg;

    if ((exit = mod_get_symbol(mod, "exit"))) {
        exit();
    }

    log_debug("Unloading '%s'", mod->path);

    dlclose(mod->dlhandle);
    free(mod);
}

struct mod_loaded *mod_get(const struct bot *bot, const char *name)
{
    struct list *ptr = NULL;

    LIST_FOREACH(bot->modules, ptr)
        if (!strcmp(list_data(ptr, struct mod_loaded *)->name, name))
            return ptr->data;

    return NULL;
}

const struct bot *mod_to_bot(const struct mod *mod)
{
    return ((const struct bot *)(mod->backref));
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

/*
 * Functions exported in module/module.h
 */
int mod_sendmsg(const struct mod *mod, const struct irc_message *msg)
{
    return sess_sendmsg(mod_to_bot(mod)->sess, msg);
}

int mod_sendln(const struct mod *mod, const char *ln)
{
    return socket_sendfln(mod_to_bot(mod)->sess->fd, "%s", ln);
}

int mod_get_identity(const struct mod *mod, struct mod_identity *ident)
{
    const struct irc_session *sess = mod_to_bot(mod)->sess;

    ident->nick = sess->nick;
    ident->user = sess->user;
    ident->real = sess->real;

    return 0;
}

struct list *mod_get_server_capabilities(const struct mod *mod)
{
    return mod_to_bot(mod)->sess->capabilities;
}

struct list *mod_get_channels(const struct mod *mod)
{
    return mod_to_bot(mod)->sess->channels;
}

int mod_load_autoload(struct bot *bot, const char *file)
{
    char linebuf[MOD_NAME_MAX] = {0};
    FILE *f;

    if (!(f = fopen(file, "r"))) {
        log_error("Failed opening '%s' for reading: %s", file, strerror(errno));

        return 1;

    } else {
        while (fgets(linebuf, sizeof(linebuf), f)) {
            if (strlen(linebuf) > 0) {
                char *endptr = linebuf + strlen(linebuf) - 1;

                while (isspace(*endptr))
                    *(endptr--) = '\0';

                if (mod_load(bot, linebuf))
                    log_error("Failed loading './%s.so'", linebuf);
            }

            memset(linebuf, 0, sizeof(linebuf));
        }

        fclose(f);
        return 0;
    }
}
