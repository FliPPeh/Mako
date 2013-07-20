#ifndef _MODULE_H_
#define _MODULE_H_

#include <stdlib.h>
#include <stdint.h>

#include "irc/irc.h"


#define MASK(x) (1 << ((x) & 31))

/*
 * Defines identifiers for events with an internal ID (enum value) and a human
 * readable name. Public and private kinds differ only by the type identifier
 * and a NULL-pointer where the target would be in the public kind.
 */
#define EVENTS \
    X(RAW,                   "raw")                                 \
    X(PING,                  "ping")                                \
                                                                    \
    /*                                                              \
     * Public events, where public means addressed to a             \
     * channel, not just us.                                        \
     */                                                             \
    X(PUBLIC_MESSAGE,        "public_message")                      \
    X(PUBLIC_NOTICE,         "public_notice")                       \
    X(PUBLIC_CTCP_REQUEST,   "public_ctcp_request")                 \
    X(PUBLIC_CTCP_RESPONSE,  "public_ctcp_response")                \
    X(PUBLIC_ACTION,         "public_action")                       \
    X(PUBLIC_COMMAND,        "public_command")                      \
                                                                    \
    /*                                                              \
     * Private events, where private means addressed just           \
     * to us, not a channel.                                        \
     */                                                             \
    X(PRIVATE_MESSAGE,       "private_message")                     \
    X(PRIVATE_NOTICE,        "private_notice")                      \
    X(PRIVATE_CTCP_REQUEST,  "private_ctcp_request")                \
    X(PRIVATE_CTCP_RESPONSE, "private_ctcp_response")               \
    X(PRIVATE_ACTION,        "private_action")                      \
    X(PRIVATE_COMMAND,       "private_command")                     \
    X(JOIN,                  "join")                                \
    X(PART,                  "part")                                \
    X(QUIT,                  "quit")                                \
    X(KICK,                  "kick")                                \
    X(NICK,                  "nick")                                \
    X(INVITE,                "invite")                              \
    X(TOPIC,                 "topic")                               \
    X(CHANNEL_MODE_SET,      "channel_mode_set")                    \
    X(CHANNEL_MODE_UNSET,    "channel_mode_unset")                  \
    X(CHANNEL_MODES,         "channel_modes_raw")                   \
    X(IDLE,                  "idle")                                \
    X(CONNECT,               "connect")                             \
    X(DISCONNECT,            "disconnect")

#define X(type, name) EVENT_ ## type,
enum mod_event_type
{
    EVENTS
};
#undef X

#define X(type, name) name,
static const char *_mod_event_name[] = { EVENTS };
#undef X

struct mod_event
{
    enum mod_event_type type;

    union
    {
        struct mod_event_raw
        {
            const struct irc_message *msg;
        } raw;

        /* privmsg, notice and action */
        struct mod_event_message
        {
            const char *prefix;
            const char *target; /* NULL if private */
            const char *msg;
        } message;

        /* ctcp response, ctcp request, triggered commands */
        struct mod_event_command
        {
            const char *prefix;
            const char *target; /* NULL if private */
            const char *command;
            const char *args;
        } command;

        struct mod_event_join
        {
            const char *prefix;
            const char *channel;
        } join;

        struct mod_event_part
        {
            const char *prefix;
            const char *channel;
            const char *msg;
        } part;

        struct mod_event_quit
        {
            const char *prefix;
            const char *msg;
        } quit;

        struct mod_event_kick
        {
            const char *prefix_kicker;
            const char *prefix_kicked;
            const char *channel;
            const char *msg;
        } kick;

        struct mod_event_nick
        {
            const char *prefix_old;
            const char *prefix_new;
        } nick;

        struct mod_event_invite
        {
            const char *prefix;
            const char *channel;
        } invite;

        struct mod_event_topic
        {
            const char *prefix;
            const char *channel;
            const char *topic_old;
            const char *topic_new;
        } topic;

        /* mode set and mode unset - only called for channel modes */
        struct mod_event_mode_change
        {
            const char *prefix;
            const char *channel;
            char mode;
            const char *arg;
        } mode_change;

        /* raw mode string, not broken up - only called for channel modes */
        struct mod_event_modes
        {
            const char *prefix;
            const char *channel;
            const char *modes;
        } modes;

        /* aux. event, called every second (default) */
        struct mod_event_idle
        {
            time_t last;
        } idle;
    };
};

struct mod_identity
{
    const char *nick;
    const char *user;
    const char *real;
};

/*
 * Top level compound structure combining everything needed to identify a module
 * and provide a backref to the host code.
 */
struct mod
{
    const char *name;
    const char *descr;

    /*
     * Bitfield of events to hook on.
     *
     * If left 0, module will not be called for any event.
     *
     * "MASK(EVENT_JOIN) | MASK(EVENT_PART)" means module will be
     * called for joins and parts only.
     *
     * ~0 or -1 means module will be called for any event.
     */
    uint64_t hooks;

    /*
     * Set via calling code, most likely a reference to the main host
     * structure, implementation details or layout not relevant to modules.
     */
    const void *backref;
};

extern struct mod mod_info;

/*
 * "Officially" exported functions. You could use any function defined anywhere
 * in the host code, but that wouldn't be very nice, would it? Besides, only the
 * exported functions know how to work with a "struct mod_state". But someone
 * who would hack around exported functions would also hack around that, so
 * whatever.
 */
ssize_t mod_sendmsg(const struct mod *mod, const struct irc_message *msg);
ssize_t mod_sendln(const struct mod *mod, const char *msg);
int mod_get_identity(const struct mod *mod, struct mod_identity *ident);

struct list *mod_get_server_capabilities(const struct mod *mod);
struct list *mod_get_channels(const struct mod *mod);

const char *mod_get_reguser(const struct mod *mod, const char *prefix);

/* Elaborate lazyness */
#define send_message(msg) mod_sendmsg(&mod_info, (msg))
#define send_line(msg) mod_sendln(&mod_info, (msg))
#define get_identity(i) mod_get_identity(&mod_info, (i))

#define get_server_capabilities() mod_get_server_capabilities(&mod_info)
#define get_channels() mod_get_channels(&mod_info)

#define get_reguser(prefix) mod_get_reguser(&mod_info, (prefix))

#endif /* defined _MODULE_H_ */
