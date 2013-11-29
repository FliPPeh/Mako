#include "modules/module.h"
#include "irc/irc.h"
#include "irc/session.h"
#include "irc/util.h"

#include "bot/helpers.h"
#include "bot/reguser.h"

#include "util/log.h"

#include <libutil/dstring.h>
#include <libutil/json.h>
#include <libutil/container/hashtable.h>

#include <sys/utsname.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

// Because I'm too lazy for semantic versioning and Pi is fun
#define PISTR "3.14159265358979323846264338327950288419716939937510582097494"

// How many digits to use for version string. 2 ("3.") + 4 => 3.1415
#define VERSION_IDX (2 + 9)

// Yes!
const char *reks[] = {
    "il bash ye fookin ead in i sware on me mom",
    "if u got sumthn 2 say say it 2 me face",
    "ur a lil gey boi lol i cud own u irl fite me",
    "1v1 fite me irl fgt il rek u",
    "u better shut ur moth u cheeky lil kunt",
    "i swer 2 christ il hook u in the gabber m8",
    "do u evn lift m8?",
    "il rek ur fokn shit i swer 2 christ",
    "il teece u 4 spekin 2 me lik that u little cunt m8"
};

#define NREKS (sizeof(reks) / sizeof(reks[0]))

enum coinflip_result
{
    HEADS,
    TAILS,
    SIDE
};

#define PROB_SIDE  0.02
#define PROB_HEADS (0.50 - ((double)PROB_SIDE / 2))
#define PROB_TAILS (0.50 - ((double)PROB_SIDE / 2))


/*
 * Provide standard CTCP replies that are nonessential for normal IRC usage
 */
int base_handle_ctcp(const char *prefix,
                     const char *target,
                     const char *ctcp,
                     const char *args);

int base_handle_command(const char *prefix,
                        const char *target,
                        const char *command);


void format_timediff(char *b, size_t bs, time_t tdiff);
enum coinflip_result flip_coin();

struct mod mod_info = {
    .name = "Base",
    .descr = "Provide basic functions",

    .hooks = M(EVENT_PRIVATE_CTCP_REQUEST)
           | M(EVENT_PUBLIC_COMMAND)
           | M(EVENT_INVITE)
};

char versionstr[256];

int mod_init()
{
    memset(versionstr, 0, sizeof(versionstr));

    struct utsname un;
    uname(&un);

    char compilerver[128] = "unknown compiler";

#ifdef __clang__
    snprintf(compilerver, sizeof(compilerver),
            "clang v%d.%d.%d",
            __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
    snprintf(compilerver, sizeof(compilerver),
            "gcc v%d.%d.%d",
            __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif

    snprintf(versionstr, sizeof(versionstr),
            "Mako v%.*s running on %s %s (%s), "
            "compiled %s, %s using %s",
                VERSION_IDX, PISTR,
                un.sysname, un.release, un.machine,
                __DATE__, __TIME__, compilerver);

    return 0;
}

int mod_exit()
{
    return 0;
}

int mod_handle_event(struct mod_event *event)
{
    switch (event->type) {
    case EVENT_PRIVATE_CTCP_REQUEST:
        ;; /* hack! */
        struct mod_event_ctcp *req = &event->event.ctcp;

        return base_handle_ctcp(
                req->prefix,
                req->target,
                req->ctcp,
                req->args);

    case EVENT_PUBLIC_COMMAND:
        ;;
        struct mod_event_command *cmd = &event->event.command;

        return base_handle_command(
                cmd->prefix,
                cmd->target,
                cmd->command);

    case EVENT_INVITE:
        ;;
        struct mod_event_invite *iv = &event->event.invite;
        struct reguser *usr = NULL;

        if ((usr = reguser_find(BOTREF, iv->prefix)) != NULL) {
            struct irc_message join;
            irc_mkmessage(&join, "JOIN",
                    (const char *[]){ iv->channel }, 1, NULL);

            bot_send_message(BOTREF, &join);
        }

        break;

    default:
        /* We may have bigger problems if this is ever reached... */
        log_warn_n("mod_base", "Called for unknown event %X", event->type);
        break;
    }

    return 0;
}

int base_handle_ctcp(
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args)
{
    (void)target;

    struct irc_prefix_parts user;
    irc_split_prefix(&user, prefix);

    if (!strcmp(ctcp, "CLIENTINFO")) {
        ctcp_response(BOTREF, user.nick, ctcp,
            "CLIENTINFO PING VERSION ACTION SOURCE TIME USERINFO");

    } else if (!strcmp(ctcp, "PING")) {
        ctcp_response(BOTREF, user.nick, ctcp, "%s", args);

    } else if (!strcmp(ctcp, "VERSION")) {
        ctcp_response(BOTREF, user.nick, ctcp, versionstr);

    } else if (!strcmp(ctcp, "SOURCE")) {
        ctcp_response(BOTREF, user.nick, ctcp,
            "https://github.com/FliPPeh/Modbot/");

    } else if (!strcmp(ctcp, "TIME")) {
        time_t now = time(NULL);
        struct tm *nowtm = localtime(&now);
        char timestr[256] = {0};

        strftime(timestr, sizeof(timestr), "%Y-%m-%dT%T UTC%z (%Z)", nowtm);

        ctcp_response(BOTREF, user.nick, ctcp, timestr);

    } else if (!strcmp(ctcp, "USERINFO")) {
        ctcp_response(BOTREF, user.nick, ctcp, "Hello :)");
    }

    return 0;
}

int base_handle_command(
        const char *prefix,
        const char *target,
        const char *command)
{
    const char *flip_strings[]     = { "heads", "tails", "side" };
    static unsigned flip_results[] = { 0,       0,       0 };

    int argc = 0;
    char **argv = dstrshlex(command, &argc);

    if (!argc)
        return 0;

    const char *cmd = argv[0];
    const char *args = argv[1];

    if (!strcmp(cmd, "ping")) {
        respond(BOTREF, target, irc_get_nick(prefix), "pong");

    } else if (!strcmp(cmd, "version")) {
        respond(BOTREF, target, irc_get_nick(prefix), versionstr);
    } else if (!strcmp(cmd, "rek")) {
        struct irc_user *usr = irc_channel_get_user(
                                   irc_channel_get(SESSION, target), args);

        if (usr) {
            respond(BOTREF, target, irc_get_nick(usr->prefix),
                reks[rand() % NREKS]);
        } else {
            respond(BOTREF, target, irc_get_nick(prefix),
                    "You suck, %s isn't even here.",
                    args);
        }

    } else if (!strcmp(cmd, "uptime")) {
        char runtime[128] = {0};
        char contime[128] = {0};
        time_t now = time(NULL);

        format_timediff(runtime, sizeof(runtime), now - SESSION->start);
        format_timediff(contime, sizeof(contime),
               now - SESSION->session_start);

        respond(BOTREF, target, irc_get_nick(prefix),
                "Running for %s, connected for %s", runtime, contime);

    } else if (!strcmp(cmd, "flipcoin")) {
        srand(time(NULL));
        enum coinflip_result res = flip_coin();

        switch (res) {
            case HEADS: flip_results[HEADS]++; break;
            case TAILS: flip_results[TAILS]++; break;
            case SIDE:  flip_results[SIDE]++;  break;
        }

        respond(BOTREF, target, irc_get_nick(prefix), "%s!", flip_strings[res]);

    } else if (!strcmp(cmd, "coinstats")) {
        unsigned total = flip_results[HEADS]
                       + flip_results[TAILS]
                       + flip_results[SIDE];

        respond(BOTREF, target, irc_get_nick(prefix),
                "%u total, %.2lf%% heads, %.2lf%% tails, %.2lf%% side",
                    total, (double)flip_results[HEADS] / total,
                           (double)flip_results[TAILS] / total,
                           (double)flip_results[SIDE] / total);
    }

    dstrlstfree(argv);

    return 0;
}

void format_timediff(char *b, size_t bs, time_t tdiff)
{
    unsigned divs[]     = {60,       60,       24,     7,     1};
    const char *names[] = {"second", "minute", "hour", "day", "week"};

    unsigned durs[] = {0, 0, 0, 0, 0}; // w, d, h, m, s

    for (size_t i = 0; i < 5; ++i) {
        if (divs[i] != 1)
            durs[i] = tdiff % divs[i];
        else
            durs[i] = tdiff;

        tdiff = (tdiff - durs[i]) / divs[i];
    }

    char *pos = b;
    char *end = b + bs;

    for (size_t i = 0; i < 5; ++i) {
        size_t id = 4 - i;

        if (durs[id] > 0) {
            pos += snprintf(pos, end - pos, "%u %s%s",
                    durs[id], names[id], durs[id] == 1 ? "" : "s");

            unsigned nonzero = 0;
            for (size_t j = i + 1; j < 5; ++j)
                if (durs[4 - j] > 0)
                    nonzero++;

            if (nonzero > 1)
                pos += snprintf(pos, end - pos, ", ");
            else if (nonzero == 1)
                pos += snprintf(pos, end - pos, " and ");
            else
                break; // no nonzeros following, we can quit here.
        }
    }
}

enum coinflip_result flip_coin()
{
    double res = (double)rand() / RAND_MAX;
    if (res < PROB_HEADS)
        return HEADS;
    else if (res < (PROB_HEADS + PROB_TAILS))
        return TAILS;

    return SIDE;
}
