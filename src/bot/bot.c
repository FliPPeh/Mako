#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#include "bot/bot.h"
#include "bot/handlers.h"
#include "bot/module.h"

#include "irc/session.h"
#include "util/util.h"
#include "util/list.h"
#include "util/log.h"

/*
 * Startup
 */
void sigint(int sig);
int print_usage(const char *prgname);
void setup_callbacks(const struct bot *bot);
void load_admins(struct bot *bot, const char *file);

static int *_bot_kill = NULL;

void sigint(int sig)
{
    if (_bot_kill) {
        *_bot_kill = 1;
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    struct irc_session sess;

    struct bot bot;
    int autoload = 1;

    struct option lopts[] = {
        { "noautoload", no_argument, &autoload, 0 },
        { "help", no_argument,       NULL,  0  },
        { "host", required_argument, NULL, 'h' },
        { "port", required_argument, NULL, 'p' },
        { "nick", required_argument, NULL, 'n' },
        { "user", required_argument, NULL, 'u' },
        { "real", required_argument, NULL, 'r' },
        { NULL,   no_argument,       NULL,  0  }
    };

    log_init(NULL);
    log_set_minlevel(LOG_DEBUG);

    memset(&sess, 0, sizeof(sess));
    memset(&bot, 0, sizeof(bot));

    sess.start = time(NULL);
    sess.portno = 6667;

    bot.sess = &sess;

    strncpy(sess.nick, DEFAULT_NICK, sizeof(sess.nick) - 1);
    strncpy(sess.user, DEFAULT_USER, sizeof(sess.user) - 1);
    strncpy(sess.real, DEFAULT_REAL, sizeof(sess.real) - 1);

    for (;;) {
        int optidx = 0;
        int opt = getopt_long(argc, argv, "h:p:n:u:r:", lopts, &optidx);

        if (opt < 0)
            break;

        switch (opt) {
            case 0:
                /* Lookup long option by optidx */
                if (!strcmp(lopts[optidx].name, "help"))
                    return print_usage(*argv);

                break;

            case 'h':
                /* Set hostname */
                strncpy(sess.hostname, optarg, sizeof(sess.hostname) - 1);
                break;

            case 'n':
                /* Set nickname */
                strncpy(sess.nick, optarg, sizeof(sess.nick) - 1);
                break;

            case 'u':
                /* Set username */
                strncpy(sess.user, optarg, sizeof(sess.user) - 1);
                break;

            case 'r':
                /* Set realname */
                strncpy(sess.real, optarg, sizeof(sess.real) - 1);
                break;

            case 'p':
                /* Set port number */
                sess.portno = (uint16_t)atoi(optarg);
                break;

            case '?':
                /* Handle unknown flag */
                log_info("%s --help for additional information\n", argv[0]);
                return 1;

            default:
                break;
        }
    }

    if (!strcmp(sess.hostname, "")) {
        log_fatal("no hostname given, see --help for more information.");

        return 1;
    }

    setup_callbacks(&bot);

    log_info("Loading admin list...");
    load_admins(&bot, "admins.cfg");

    if (autoload) {
        log_info("Autoloading modules...");
        mod_load_autoload(&bot, "autoload.cfg");
    }

    log_info("Starting session...");

    _bot_kill = &sess.kill;
    signal(SIGINT, sigint);

    sess_main(&sess);

    log_debug("Cleaning up...");

    list_free_all(bot.modules, mod_free);
    list_free_all(bot.admins, free);

    log_info("Goodbye!");
    log_destroy();

    return 0;
}

int print_usage(const char *prgname)
{
    printf(
        "Usage: %s [OPTION]...\n\n"
        "  -h, --host=<HOST>     connect to hostname HOST\n"
        "  -p, --port=<PORT>     connect to port PORT\n"
        "  -n, --nick=<NICK>     set nickname to NICK\n"
        "  -u, --user=<USER>     set username to USER\n"
        "  -r, --real=<REAL>     set realname to REAL\n"
        "      --noautoload      supress autoloading of modules listed in "
                                "autoload.cfg\n"
        "      --help            display this help and exit\n", prgname);

    return 0;
}

void setup_callbacks(const struct bot *bot)
{
    struct irc_callbacks *cb = &bot->sess->cb;

    cb->arg = (void *)bot;

    cb->on_event = bot_on_event;
    cb->on_ping = bot_on_ping;
    cb->on_privmsg = bot_on_privmsg;
    cb->on_notice = bot_on_notice;
    cb->on_join = bot_on_join;
    cb->on_part = bot_on_part;
    cb->on_quit = bot_on_quit;
    cb->on_kick = bot_on_kick;
    cb->on_nick = bot_on_nick;
    cb->on_invite = bot_on_invite;
    cb->on_topic = bot_on_topic;
    cb->on_mode_set = bot_on_mode_set;
    cb->on_mode_unset = bot_on_mode_unset;
    cb->on_modes = bot_on_modes;
    cb->on_connect = bot_on_connect;
    cb->on_disconnect = bot_on_disconnect;
    cb->on_idle = bot_on_idle;
}

const char *bot_get_reguser(const struct bot *bot, const char *prefix)
{
    struct list *ptr = NULL;

    LIST_FOREACH(bot->admins, ptr) {
        struct admin *adm = list_data(ptr, struct admin *);

        if (regex_match(adm->mask, prefix))
            return adm->name;
    }

    return NULL;
}

void load_admins(struct bot *bot, const char *file)
{
    char linebuf[512] = {0};
    FILE *f;

    if (!(f = fopen(file, "r"))) {
        log_error("Failed opening '%s' for reading: %s", file, strerror(errno));
        return;
    } else {
        list_free_all(bot->admins, free);
        bot->admins = NULL;

        while (fgets(linebuf, sizeof(linebuf), f)) {
            if (strlen(linebuf) > 0) {
                char *endptr = linebuf + strlen(linebuf) - 1;

                while (isspace(*endptr))
                    *(endptr--) = '\0';

                char *sep = strchr(linebuf, ':');

                if (!sep) {
                    continue;
                } else {
                    struct admin *adm = malloc(sizeof(*adm));
                    memset(adm, 0, sizeof(*adm));

                    *sep = '\0';

                    strncpy(adm->name, linebuf,
                       MIN(sizeof(adm->name) - 1, (size_t)(sep - linebuf)));

                    strncpy(adm->mask, sep + 1,
                       MIN(sizeof(adm->mask) - 1,
                           strlen(linebuf) - (size_t)(sep - linebuf) - 2));

                    log_debug("Name: '%s', RegEx: '%s'", adm->name, adm->mask);
                    bot->admins = list_append(bot->admins, adm);
                }
            }

            memset(linebuf, 0, sizeof(linebuf));
        }
    }

    fclose(f);
    return;
}
