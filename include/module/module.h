#ifndef _MODULE_H_
#define _MODULE_H_

#include "irc/irc.h"


/*
 * Defines identifiers for events with an internal ID (enum value) and a human
 * readable name
 */
#define EVENTS \
    X(RAW,        "raw") \
    X(PING,       "ping") \
    X(PRIVMSG,    "privmsg") \
    X(NOTICE,     "notice") \
    X(JOIN,       "join") \
    X(PART,       "part") \
    X(QUIT,       "quit") \
    X(KICK,       "kick") \
    X(NICK,       "nick") \
    X(INVITE,     "invite") \
    X(TOPIC,      "topic") \
    X(MODESET,    "mode-set") \
    X(MODEUNSET,  "mode-unset") \
    X(MODES,      "modes-raw") \
    X(IDLE,       "idle") \
    X(CONNECT,    "connect") \
    X(DISCONNECT, "disconnect") \
    X(CTCPREQ,    "ctcp-request") \
    X(CTCPRESP,   "ctcp-response") \
    X(ACTION,     "action") \
    X(COMMAND,    "command")

#define X(type, name) EVENT_ ## type,
enum mod_event_type
{
    EVENTS
};
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

        struct mod_event_privmsg
        {
            const char *prefix;
            const char *target;
            const char *msg;
        } privmsg;

        struct mod_event_notice
        {
            const char *prefix;
            const char *target;
            const char *msg;
        } notice;

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

        struct mod_event_mode_set
        {
            const char *prefix;
            const char *channel;
            char mode;
            const char *arg;
        } mode_set;

        struct mod_event_mode_unset
        {
            const char *prefix;
            const char *channel;
            char mode;
            const char *arg;
        } mode_unset;

        struct mod_event_modes
        {
            const char *prefix;
            const char *channel;
            const char *modes;
        } modes;

        struct mod_event_idle
        {
            time_t last;
        } idle;

        struct mod_event_ctcp_req
        {
            const char *prefix;
            const char *target;
            const char *ctcp;
            const char *arg;
        } ctcp_req;

        struct mod_event_ctcp_resp
        {
            const char *prefix;
            const char *target;
            const char *ctcp;
            const char *arg;
        } ctcp_resp;

        struct mod_event_action
        {
            const char *prefix;
            const char *target;
            const char *msg;
        } action;

        struct mod_event_command
        {
            const char *prefix;
            const char *target;
            const char *command;
            const char *args;
        } command;
    };
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
int mod_sendmsg(const struct mod *mod, const struct irc_message *msg);
int mod_sendln(const struct mod *mod, const char *msg);

struct list *mod_get_server_capabilities(const struct mod *mod);
struct list *mod_get_channels(const struct mod *mod);

/* Elaborate lazyness */
#define send_message(msg) mod_sendmsg(&mod_info, (msg))
#define send_line(msg) mod_sendln(&mod_info, (msg))

#define get_server_capabilities() mod_get_server_capabilities(&mod_info)
#define get_channels() mod_get_channels(&mod_info)

#endif /* defined _MODULE_H_ */
