#include "irc/session.h"
#include "irc/irc.h"
#include "irc/util.h"
#include "irc/net/socket.h"
#include "util/log.h"
#include "util/util.h"

#include <libutil/container/hashtable.h>

#include <sys/select.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


void sess_init(struct irc_session *sess,
               const char *server,
               uint16_t port,
               const char *nick,
               const char *user,
               const char *real,
               const char *pass)
{
    memset(sess, 0, sizeof(*sess));

    sess->channels = hashtable_new_with_free(
            ascii_hash,
            ascii_equal,
            free,
            irc_channel_free);

    sess->capabilities = hashtable_new_with_free(
            ascii_hash,
            ascii_equal,
            free,
            free);

    sess->start = time(NULL);

    strncpy(sess->hostname, server, sizeof(sess->hostname) - 1);
    sess->portno = port;

    strncpy(sess->nick, nick, sizeof(sess->nick) - 1);
    strncpy(sess->user, user, sizeof(sess->user) - 1);
    strncpy(sess->real, real, sizeof(sess->real) - 1);
    strncpy(sess->serverpass, pass, sizeof(sess->serverpass) - 1);

    tokenbucket_init(&sess->quota, FLOODPROT_CAPACITY, FLOODPROT_RATE);
}

void sess_destroy(struct irc_session *sess)
{
    hashtable_free(sess->channels);
    hashtable_free(sess->capabilities);
}

/*
 * Util
 */
const char *sess_capability_get(struct irc_session *sess, const char *cap)
{
    return hashtable_lookup(sess->capabilities, cap);
}

int sess_capability_set(struct irc_session *sess, const char *cap,
                                                  const char *val)
{
    hashtable_insert(sess->capabilities, strdup(cap), strdup(val));
    return 0;
}

int sess_getln(struct irc_session *sess, char *linebuf, size_t linebufsiz)
{
    char *crlf = NULL;

    if ((crlf = strstr(sess->buffer, "\r\n"))) {
        size_t len = (size_t)(crlf - sess->buffer);

        /* bufuse = buffer usage after line has been taken out */
        sess->bufuse -= len + 2;

        /* Copy line over to line buffer */
        memset(linebuf, 0, linebufsiz);
        strncpy(linebuf, sess->buffer, MIN(len, linebufsiz - 1));

        /* Shift buffer forward, starting from the first byte after \r\n to the
         * end */
        memmove(sess->buffer, crlf + 2, sizeof(sess->buffer) - len + 2);

        log_debug("<< %s", linebuf);
        return 0;
    }

    return 1;
}

int sess_sendmsg(struct irc_session *sess, const struct irc_message *msg)
{
    struct irc_message msgcopy = *msg;

    if (sess->cb.on_send_message)
        if (sess->cb.on_send_message(sess->cb.arg, &msgcopy))
            return 0;

    if (sess->buffer_out_start == sess->buffer_out_end) {
        /* Buffer empty, try to send immediately */
        unsigned msglen = irc_message_size(&msgcopy) + 2; /* \r\n */

        /* "Round up" the size to a minimum size, so lots of tiny messages
         * are still effectively limited */
        msglen = MAX(MIN(msglen, IRC_MESSAGE_MAX), FLOODPROT_MIN);

        if (tokenbucket_consume(&sess->quota, msglen))
            /* Buffer is empty and enough quota is left, send for realsies */
            return sess_sendmsg_real(sess, &msgcopy);

        /* Not enough quota left, drop down and push into queue */
    } else if (((sess->buffer_out_end + 1) % FLOODPROT_BUFFER)
            == sess->buffer_out_end) {
        /* Buffer is full! Discard. */
        log_warn("Flood protection: Outgoing buffer full, discarding message!");
        return -1;
    }

    /* Buffer is neither empty nor full or message could not be sent. Append it
     * to the buffer */
    log_warn("Flood protection: message queued for later delivery");
    sess->buffer_out[sess->buffer_out_end] = msgcopy;
    sess->buffer_out_end = (sess->buffer_out_end + 1) % FLOODPROT_BUFFER;

    return 0;
}

int sess_sendmsg_real(struct irc_session *sess, const struct irc_message *msg)
{
    char buffer[IRC_MESSAGE_MAX] = {0};

    irc_message_to_string(msg, buffer, sizeof(buffer));
    log_debug(">> %s", buffer);

    return socket_sendfln(sess->fd, "%s", buffer) > 0;
}

int sess_connect(struct irc_session *sess)
{
    return ((sess->fd = socket_connect(sess->hostname, itoa(sess->portno))));
}

int sess_disconnect(struct irc_session *sess)
{
    return socket_disconnect(sess->fd);
}


/*
 * Main loop
 */
int sess_main(struct irc_session *sess)
{
    /* Jump into mainloop */
    while (!sess->kill) {
        time_t lastidle = time(NULL);
        time_t last_sign_of_life = time(NULL);

        if (sess_connect(sess) < 0)
            break;

        sess->session_start = time(NULL);
        sess_login(sess);

        /* Inner loop, receive and handle data */
        while (!sess->kill) {
            /*
             * Since both idle and flood protection are based on second
             * precision, this is an optimal timeout
             */
            struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };

            int res = sess_handle_data(sess, &timeout);

            if (res < 0) {
                break;
            } else if (res > 0) {
                last_sign_of_life = time(NULL);
            } else {
                if ((time(NULL) - last_sign_of_life) >= TIMEOUT) {
                    struct tm *tm = TIME_GETTIME(&last_sign_of_life);

                    char buffer[TIMEBUF_MAX] = {0};
                    strftime(buffer, sizeof(buffer), STRFTIME_FORMAT, tm);

                    log_info("Last sign of life was %d seconds "
                             "ago (%s). Pinging server...",
                                time(NULL) - last_sign_of_life, buffer);

                    struct irc_message ping;
                    irc_mkmessage(&ping, CMD_PING, NULL, 0, "%s",
                        sess->hostname);

                    if (sess_sendmsg_real(sess, &ping) <= 0)
                        break;
                }
            }

            /* Try to send messages in outbuffer */
            while (sess->buffer_out_start != sess->buffer_out_end) {
                struct irc_message *next =
                    &(sess->buffer_out[sess->buffer_out_start]);

                unsigned len = irc_message_size(next);
                len = MAX(len, FLOODPROT_MIN);

                if (tokenbucket_consume(&sess->quota, len)) {
                    sess_sendmsg_real(sess, next);

                    sess->buffer_out_start =
                        (sess->buffer_out_start + 1) % FLOODPROT_BUFFER;
                } else {
                    break;
                }
            }

            /* Check if we have to emit an idle event */
            if ((time(NULL) - lastidle) >= IDLE_INTERVAL) {
                if (sess->cb.on_idle)
                    sess->cb.on_idle(sess->cb.arg, lastidle);

                lastidle = time(NULL);
            }

            /* And regenerate some tokens */
            tokenbucket_generate(&sess->quota);
        }

        if (sess->cb.on_disconnect)
            sess->cb.on_disconnect(sess->cb.arg);

        /* Session is finished, free resources and start again unless killed */
        hashtable_clear(sess->channels);
        hashtable_clear(sess->capabilities);

        sess_disconnect(sess);
    }

    return 0;
}

int sess_login(struct irc_session *sess)
{
    struct irc_message nick;
    struct irc_message user;

    irc_mkmessage(&nick, CMD_NICK, (const char *[]){ sess->nick }, 1, NULL);
    irc_mkmessage(&user, CMD_USER,
            (const char *[]){ sess->user, "*", "*" }, 3, "%s", sess->real);

    sess_sendmsg_real(sess, &nick);
    sess_sendmsg_real(sess, &user);

    if (strcmp(sess->serverpass, "") != 0) {
        struct irc_message pass;

        irc_mkmessage(&pass, CMD_PASS,
                (const char*[]){ sess->serverpass }, 1, NULL);

        sess_sendmsg_real(sess, &pass);
    }

    return 0;
}

/*
 *  Waits for data on the socket, return value indicates one of three results:
 *  < 0: error
 *  > 0: number of bytes received (success)
 *    0: timeout while waiting
 */
int sess_handle_data(struct irc_session *sess, struct timeval *timeout)
{
    fd_set reads;

    FD_ZERO(&reads);
    FD_SET(sess->fd, &reads);

    int nfds = select(sess->fd + 1, &reads, NULL, NULL, timeout);

    if ((nfds > 0) && (FD_ISSET(sess->fd, &reads))) {
        struct irc_message msg;
        char line[IRC_MESSAGE_MAX] = {0};

        ssize_t data = socket_recv(
                        sess->fd,
                        sess->buffer + sess->bufuse,
                        sizeof(sess->buffer) - sess->bufuse - 1);

        if (data <= 0) {
            if (data == 0)
                log_info("Server closed connection");
            else
                log_error("Connection terminated unexpectedly: %s",
                        strerror(errno));

            return -1;
        }

        sess->bufuse += (size_t)data;

        /*
         * While the last blob of data received contains a full line,
         * process it
         */
        while (!sess_getln(sess, line, sizeof(line))) {
            if (!irc_parse_message(line, &msg)) {
                if (sess_handle_message(sess, &msg)) {
                    log_warn("message handler returned failure -- abort!");
                    return -1;
                }
            } else {
                log_warn("Failed to parse line '%s'", line);
            }
        }

        return data;
    }

    return 0;
}


/*
 * Logic
 */

/*
 * Centralize a few commonly used warning messages and checks.
 *
 * Macros are evil, but repeating yourself is worse.
 */
#define CHECK_ARGC(least, msg)                                     \
    if ((msg)->paramcount < least) {                               \
        WARN_BAD_ARGC((msg)->command,                              \
                (least), (msg)->paramcount);                       \
        goto exit_err;                                             \
    }

#define WARN_UNKNOWN_CHAN(dom, chname) \
    log_warn("%s: Unknown channel `%s'", irc_command_to_string(dom), (chname));

#define WARN_UNKNOWN_CHUSER(dom, chan, uname)                   \
    log_warn("%s: Unknown channel user `%s' for channel `%s'",  \
            irc_command_to_string(dom), (uname), (chan)->name);

#define WARN_BAD_ARGC(dom, e, h)                           \
    log_warn("%s: Expected at least %d arguments, got %d", \
            irc_command_to_string(dom), (e), (h));


int sess_handle_message(struct irc_session *sess, struct irc_message *msg)
{
    if (sess->cb.on_event)
        sess->cb.on_event(sess->cb.arg, msg);

    if (msg->command == CMD_PING) {
        struct irc_message pong;

        irc_mkmessage(&pong, CMD_PONG, NULL, 0, "%s", msg->msg);
        sess_sendmsg(sess, &pong);

        if (sess->cb.on_ping)
            sess->cb.on_ping(sess->cb.arg);

    } else if (msg->command == RPL_WELCOME) {
        if (sess->cb.on_connect)
            sess->cb.on_connect(sess->cb.arg);

    } else if (msg->command == RPL_ISUPPORT) {
        char capability[IRC_PARAM_MAX];
        char *eq = NULL;

        for (int i = 1; i < msg->paramcount; ++i) {
            memcpy(capability, msg->params[i], sizeof(capability));

            if (!(eq = strchr(msg->params[i], '='))) {
                sess_capability_set(sess, msg->params[i], NULL);
                sess_handle_isupport(sess, msg->params[i], NULL);
            } else {
                *eq = '\0';
                sess_capability_set(sess, msg->params[i], eq + 1);
                sess_handle_isupport(sess, msg->params[i], eq + 1);
            }
        }
    } else if (msg->command == RPL_TOPIC) {
        struct irc_channel *target = NULL;

        CHECK_ARGC(2, msg);

        if ((target = irc_channel_get(sess, msg->params[1])))
            irc_channel_set_topic(target, msg->msg);
        else
            WARN_UNKNOWN_CHAN(msg->command, msg->params[1]);


    } else if (msg->command == RPL_TOPICWHOTIME) {
        /* REPL_TOPIC_META */
        struct irc_channel *target = NULL;

        CHECK_ARGC(4, msg);

        if ((target = irc_channel_get(sess, msg->params[1])))
            irc_channel_set_topic_meta(target,
                msg->params[2], atoi(msg->params[3]));
        else
            WARN_UNKNOWN_CHAN(msg->command, msg->params[1]);


    } else if (msg->command == RPL_CHANNELMODEIS) {
        CHECK_ARGC(3, msg);

        sess_handle_mode_change(sess,
                msg->prefix,
                msg->params[1], /* channel */
                msg->params[2], /* modestring */
                msg->params, 3, msg->paramcount); /* param start and end */

    } else if (msg->command == RPL_CREATIONTIME) {
        struct irc_channel *target = NULL;

        CHECK_ARGC(3, msg);

        if ((target = irc_channel_get(sess, msg->params[1])))
            irc_channel_set_created(target, atoi(msg->params[2]));
        else
            WARN_UNKNOWN_CHAN(msg->command, msg->params[1]);


    } else if (msg->command == RPL_WHOREPLY) {
        struct irc_channel *channel = NULL;

        CHECK_ARGC(7, msg);

        if ((channel = irc_channel_get(sess, msg->params[1]))) {
            struct irc_user *user = NULL;

            if (!(user = irc_channel_get_user(channel, msg->params[5])))  {
                const char *prf = sess_capability_get(sess, "PREFIX");
                size_t prfsep = strlen(prf) / 2;

                char prefix[IRC_PREFIX_MAX] = {0};

                snprintf(prefix, sizeof(prefix), "%s!%s@%s",
                        msg->params[5], msg->params[2], msg->params[3]);

                irc_channel_add_user(channel, prefix);
                if (!(user = irc_channel_get_user(channel, prefix))) {
                    log_wtf("Newly added user '%s' not found", msg->params[5]);

                    goto exit_err;
                }

                /*
                 * Go over every flag within the users mode string and match
                 * them against the server's PREFIX to get the actual mode.
                 */
                for (size_t i = 0; i < strlen(msg->params[6]); ++i)
                    for (size_t j = 0; j < prfsep - 1; ++j)
                        if (msg->params[6][i] == prf[prfsep + j + 1])
                            irc_channel_user_set_mode(user, prf[j + 1]);
            } else {
                log_warn("%s: User '%s' already in channel",
                        irc_command_to_string(msg->command), msg->params[5]);
            }
        } else {
            WARN_UNKNOWN_CHAN(msg->command, msg->params[1]);
        }


    } else if (msg->command == RPL_BANLIST) {
        struct irc_channel *channel = NULL;

        CHECK_ARGC(3, msg);

        if ((channel = irc_channel_get(sess, msg->params[1])))
            irc_channel_set_mode(channel, 'b', msg->params[2]);
        else
            WARN_UNKNOWN_CHAN(msg->command, msg->params[1]);


    } else if (msg->command == RPL_ENDOFMOTD) {
        /* MOTD_END */

    } else if (msg->command == CMD_PRIVMSG) {
        CHECK_ARGC(1, msg);

        if (sess->cb.on_privmsg)
            sess->cb.on_privmsg(sess->cb.arg,
                    msg->prefix, msg->params[0], msg->msg);

    } else if (msg->command == CMD_NOTICE) {
        CHECK_ARGC(1, msg);

        if (sess->cb.on_notice)
            sess->cb.on_notice(sess->cb.arg,
                    msg->prefix, msg->params[0], msg->msg);

    } else if (msg->command == CMD_JOIN) {
        if (!irc_user_cmp(msg->prefix, sess->nick)) {
            struct irc_message who;
            struct irc_message mode;
            struct irc_message bans;

            const char *channel = msg->paramcount > 0
                ? msg->params[0]
                : msg->msg;

            irc_channel_add(sess, channel);

            irc_mkmessage(&who, CMD_WHO,
                    (const char *[]){ channel }, 1, NULL);
            irc_mkmessage(&mode, CMD_MODE,
                    (const char *[]){ channel }, 1, NULL);
            irc_mkmessage(&bans, CMD_MODE,
                    (const char *[]){ channel, "+b" }, 2, NULL);

            sess_sendmsg(sess, &who);
            sess_sendmsg(sess, &mode);
            sess_sendmsg(sess, &bans);
        } else {
            struct irc_channel *channel = NULL;

            if ((channel = irc_channel_get(sess, msg->params[0])))
                irc_channel_add_user(channel, msg->prefix);
            else
                WARN_UNKNOWN_CHAN(msg->command, msg->params[0]);
        }

        if (sess->cb.on_join)
            sess->cb.on_join(sess->cb.arg, msg->prefix, msg->params[0]);

    } else if (msg->command == CMD_PART) {
        struct irc_channel *target = NULL;

        CHECK_ARGC(1, msg);

        if (sess->cb.on_part)
            sess->cb.on_part(sess->cb.arg,
                msg->prefix, msg->params[0], msg->msg);

        if ((target = irc_channel_get(sess, msg->params[0]))) {
            if (!irc_user_cmp(msg->prefix, sess->nick)) {
                irc_channel_del(sess, target);
            } else {
                struct irc_user *usr = NULL;

                if ((usr = irc_channel_get_user(target, msg->prefix)))
                    irc_channel_del_user(target, usr);
                else
                    WARN_UNKNOWN_CHUSER(msg->command, target, msg->prefix);
            }
        } else {
            WARN_UNKNOWN_CHAN(msg->command, msg->params[0]);
        }

    } else if (msg->command == CMD_KICK) {
        struct irc_channel *target = NULL;

        CHECK_ARGC(2, msg);

        if ((target =  irc_channel_get(sess, msg->params[0]))) {
            struct irc_user *utarget = NULL;

            if ((utarget = irc_channel_get_user(target, msg->params[1]))) {
                if (sess->cb.on_kick)
                    sess->cb.on_kick(sess->cb.arg,
                                     msg->prefix,
                                     utarget->prefix,
                                     msg->params[0],
                                     msg->msg);

                if (!irc_user_cmp(msg->params[1], sess->nick))
                    irc_channel_del(sess, target);
                else
                    irc_channel_del_user(target, utarget);
            } else {
                WARN_UNKNOWN_CHUSER(msg->command, target, msg->params[1]);
            }
        } else {
            WARN_UNKNOWN_CHAN(msg->command, msg->params[0]);
        }


    } else if (msg->command == CMD_QUIT) {
        if (sess->cb.on_quit)
            sess->cb.on_quit(sess->cb.arg, msg->prefix, msg->msg);

        struct hashtable_iterator iter;
        void *k = NULL;
        void *v = NULL;

        hashtable_iterator_init(&iter, sess->channels);
        while (hashtable_iterator_next(&iter, &k, &v)) {
            struct irc_user *usr = irc_channel_get_user(v, msg->prefix);

            if (usr)
                irc_channel_del_user(v, usr);
        }

    } else if (msg->command == CMD_NICK) {
        struct irc_user *user = NULL;
        const char *newnick = msg->paramcount > 0
            ? msg->params[0]
            : msg->msg;

        /* TODO: Move this into irc_channel_rename_user? */
        char *oldpostfix = NULL;
        char newprefix[IRC_PREFIX_MAX] = {0};
        char oldprefix[IRC_PREFIX_MAX] = {0};

        if (!irc_user_cmp(msg->prefix, sess->nick)) {
            memset(sess->nick, 0, sizeof(sess->nick));
            strncpy(sess->nick, newnick, sizeof(sess->nick) - 1);
        }

        /* Check for a valid prefix before proceeding. */
        if ((oldpostfix = strchr(msg->prefix, '!')) != NULL) {
            strncpy(oldprefix, msg->prefix, sizeof(oldprefix) - 1);
            strncat(newprefix, newnick, sizeof(newprefix) - 1);
            strncat(newprefix, oldpostfix, sizeof(newprefix) - 1);

            struct hashtable_iterator iter;
            void *k = NULL;
            void *v = NULL;

            hashtable_iterator_init(&iter, sess->channels);
            while (hashtable_iterator_next(&iter, &k, &v))
                if ((user = irc_channel_get_user(v, msg->prefix)))
                    irc_channel_rename_user(v, user, newprefix);

            if (sess->cb.on_nick)
                sess->cb.on_nick(sess->cb.arg, oldprefix, newprefix);
        } else {
            log_warn("Invalid user prefix: `%s'", msg->prefix);
        }

    } else if (msg->command == CMD_INVITE) {
        if (sess->cb.on_invite)
            sess->cb.on_invite(
                    sess->cb.arg,
                    msg->prefix,
                    msg->paramcount > 1
                        ? msg->params[0]
                        : msg->msg);

    } else if (msg->command == CMD_TOPIC) {
        struct irc_channel *target = NULL;
        struct irc_message topic;

        CHECK_ARGC(1, msg);

        if (sess->cb.on_topic)
            sess->cb.on_topic(
                    sess->cb.arg,
                    msg->prefix,
                    msg->params[0],
                    target->topic,
                    msg->msg);

        irc_mkmessage(&topic, CMD_TOPIC,
                (const char *[]){ msg->params[0] }, 1, NULL);

        sess_sendmsg(sess, &topic);

    } else if (msg->command == CMD_MODE) {
        if ((msg->paramcount > 0) && (!irc_is_channel(msg->params[0]))) {
            log_debug("ignoring user mode on self");
        } else {
            CHECK_ARGC(2, msg);

            if (irc_user_cmp(msg->params[0], sess->nick))
                sess_handle_mode_change(sess,
                    msg->prefix,
                    msg->params[0],
                    msg->params[1],
                    msg->params, 2, msg->paramcount);
        }
    }

    return 0;

exit_err:
    return 1;
}

int sess_handle_isupport(struct irc_session *sess,
                         const char *sup,
                         const char *val)
{
    if (!strcmp(sup, "CHANMODES") && (val != NULL)) {
        char *ptr = (char *)val;
        char *nptr = NULL;

        char restbuf[IRC_PARAM_MAX] = {0};
        size_t i = 0;

        do {
            nptr = strpart(ptr, ',',
                    sess->chanmodes[i], sizeof(sess->chanmodes[i]) - 1,
                    restbuf,            sizeof(restbuf)            - 1);

            if (!nptr)
                strncpy(sess->chanmodes[i],
                        restbuf,
                        sizeof(sess->chanmodes[i]) - 1);
            else
                ptr = nptr;

            ++i;

        } while (nptr != NULL);
    } else if (!strcmp(sup, "PREFIX") && (val != NULL)) {
        size_t i;
        size_t len = strlen(val);

        for (i = 0; (i < (len / 2) - 1) && (i < IRC_CHANNEL_PREFIX_MAX); ++i)
            sess->usermodes[i] = val[i + 1];
    }

    return 0;
}

int sess_handle_mode_change(struct irc_session *sess,
                            const char *prefix,
                            const char *chan,
                            const char *modestr,
                            char args[][IRC_PARAM_MAX],
                            size_t argstart,
                            size_t argmax)
{
    int set = 1; /* 1 = set, 0 = unset */
    size_t i = argstart;

    struct irc_channel *t = NULL;

    const char *oldmodes = modestr; /* backup for signal */

    if (!(t = irc_channel_get(sess, chan))) {
        log_warn("Unable to look up channel '%s'", chan);
        return 1;
    }

    while (*modestr) {
        const char *arg = NULL;

        if ((*modestr == '+') || (*modestr == '-'))
            set = (*modestr++ == '+');

        if ((strchr(sess->chanmodes[MODE_LIST],   *modestr)) ||
            (strchr(sess->chanmodes[MODE_REQARG], *modestr)) ||
           ((strchr(sess->chanmodes[MODE_SETARG], *modestr) && set)) ||
            (strchr(sess->usermodes,              *modestr))) {

            if (i < argmax) {
                arg = args[i++];

                log_debug("%s: set mode %c%c with arg '%s'",
                    t->name, set ? '+' : '-', *modestr, arg);
            } else {
                log_error("too few mode parameters");
                return 1;
            }
        } else {
            /* Unknown flags, noarg flags or whensets when not set */
            arg = NULL;

            log_debug("%s: set mode %c%c", t->name, set ? '+' : '-', *modestr);
        }

        if (set) {
            irc_channel_set_mode(t, *modestr, arg);

            if (sess->cb.on_mode_set)
                sess->cb.on_mode_set(sess->cb.arg,
                    prefix, chan, *modestr, arg);
        } else {
            irc_channel_unset_mode(t, *modestr, arg);

            if (sess->cb.on_mode_unset)
                sess->cb.on_mode_unset(sess->cb.arg,
                    prefix, chan, *modestr, arg);
        }

        modestr++;
    }

    if (sess->cb.on_modes)
        sess->cb.on_modes(sess->cb.arg, prefix, chan, oldmodes);

    return 0;
}
