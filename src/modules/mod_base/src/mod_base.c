#include "modules/module.h"
#include "irc/irc.h"
#include "irc/session.h"
#include "irc/util.h"

#include "bot/helpers.h"
#include "bot/reguser.h"

#include "util/log.h"

#include "container/hashtable.h"

#include <sys/utsname.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

// Because I'm too lazy for semantic versioning and Pi is fun
#define PISTR "3.14159265358979323846264338327950288419716939937510582097494"

// How many digits to use for version string. 2 ("3.") + 4 => 3.1415
#define VERSION_IDX (2 + 4)

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
int base_handle_ctcp(
        const char *prefix,
        const char *target,
        const char *ctcp,
        const char *args);

int base_handle_command(
        const char *prefix,
        const char *target,
        const char *cmd,
        const char *args);


void format_timediff(char *b, size_t bs, time_t tdiff);
enum coinflip_result flip_coin();

struct mod mod_info = {
    .name = "Base",
    .descr = "Provide basic functions",

    .hooks = M(EVENT_PRIVATE_CTCP_REQUEST) | M(EVENT_PUBLIC_COMMAND)
};

char versionstr[256];

int mod_init()
{
    memset(versionstr, 0, sizeof(versionstr));

    struct utsname un;
    uname(&un);

    snprintf(versionstr, sizeof(versionstr), "Mako v%.*s running on %s %s (%s)",
                VERSION_IDX, PISTR,
                un.sysname, un.release, un.machine);

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
        struct mod_event_command *req = &event->command;

        return base_handle_ctcp(
                req->prefix,
                req->target,
                req->command,
                req->args);

    case EVENT_PUBLIC_COMMAND:
        ;;
        struct mod_event_command *cmd = &event->command;

        return base_handle_command(
                cmd->prefix,
                cmd->target,
                cmd->command,
                cmd->args);

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

    struct irc_message response;
    struct irc_prefix_parts user;

    irc_split_prefix(&user, prefix);

    if (!strcmp(ctcp, "PING")) {
        irc_mkctcp_response(&response, user.nick, ctcp, "%s", args);
        bot_send_message(BOTREF, &response);

    } else if (!strcmp(ctcp, "VERSION")) {
        irc_mkctcp_response(&response, user.nick, ctcp, versionstr);
        bot_send_message(BOTREF, &response);

    }

    return 0;
}

int base_handle_command(
        const char *prefix,
        const char *target,
        const char *cmd,
        const char *args)
{
    struct irc_prefix_parts user;

    irc_split_prefix(&user, prefix);

    const char *flip_strings[]     = { "heads", "tails", "side" };
    static unsigned flip_results[] = { 0,       0,       0 };

    if (!strcmp(cmd, "ping")) {
        respond(BOTREF, target, user.nick, "pong");

    } else if (!strcmp(cmd, "version")) {
        respond(BOTREF, target, user.nick, versionstr);
    } else if (!strcmp(cmd, "rek")) {
        respond(BOTREF, target, args,
                reks[rand() % NREKS]);

    } else if (!strcmp(cmd, "uptime")) {
        char runtime[128] = {0};
        char contime[128] = {0};
        time_t now = time(NULL);

        format_timediff(runtime, sizeof(runtime), now - BOTREF->sess->start);
        format_timediff(contime, sizeof(contime),
               now - BOTREF->sess->session_start);

        respond(BOTREF, target, user.nick,
                "Running for %s, connected for %s", runtime, contime);
    } else if (!strcmp(cmd, "whoami")) {
        struct reguser *usr = NULL;

        if ((usr = reguser_find(BOTREF, prefix)) != NULL) {
            respond(BOTREF, target, user.nick,
                    "I know you as '%s' with the flags '%s'",
                        usr->name, reguser_flagstr(usr));
        } else {
            respond(BOTREF, target, user.nick, "Who are you indeed.");
        }
    } else if (!strcmp(cmd, "dumpusers")) {
        struct hashtable_iterator iter;

        void *channame;
        void *channel;

        hashtable_iterator_init(&iter, BOTREF->sess->channels);
        while (hashtable_iterator_next(&iter, &channame, &channel)) {
            log_info("Dumping userlist for channel '%s'...", channame);

            struct hashtable_iterator iter2;

            void *prefix;
            void *user;

            hashtable_iterator_init(&iter2,
                    ((struct irc_channel *)channel)->users);
            while (hashtable_iterator_next(&iter2, &prefix, &user)) {
                log_info("'%s' => '%s'",
                        prefix, ((struct irc_user *)user)->modes);
            }

            log_info("");
        }
    } else if (!strcmp(cmd, "flipcoin")) {
        srand(time(NULL));
        enum coinflip_result res = flip_coin();

        switch (res) {
            case HEADS: flip_results[HEADS]++; break;
            case TAILS: flip_results[TAILS]++; break;
            case SIDE:  flip_results[SIDE]++;  break;
        }

        respond(BOTREF, target, user.nick, "%s!", flip_strings[res]);

    } else if (!strcmp(cmd, "coinstats")) {
        unsigned total = flip_results[HEADS]
                       + flip_results[TAILS]
                       + flip_results[SIDE];

        respond(BOTREF, target, user.nick, "%u total, %.2lf%% heads, "
                                                    " %.2lf%% tails, "
                                                    " %.2lf%% side",
                total, (double)flip_results[HEADS] / total,
                       (double)flip_results[TAILS] / total,
                       (double)flip_results[SIDE] / total);

    }

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
