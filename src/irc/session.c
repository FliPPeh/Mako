#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/select.h>

#include "irc/irc.h"
#include "irc/util.h"
#include "irc/session.h"
#include "irc/net/socket.h"

#include "util/log.h"
#include "util/hashtable.h"
#include "util/util.h"

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
}

void sess_destroy(struct irc_session *sess)
{
    hashtable_free(sess->channels);
    hashtable_free(sess->capabilities);
}

/*
 * Util
 */
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
    char buffer[IRC_MESSAGE_MAX] = {0};
    struct irc_message msgcopy = *msg;

    if (sess->cb.on_send_message)
        if (sess->cb.on_send_message(sess->cb.arg, &msgcopy))
            return 0;

    strncat(buffer, msgcopy.command, sizeof(buffer) - 1);

    for (int i = 0; i < msgcopy.paramcount; ++i) {
        strncat(buffer, " ", sizeof(buffer) - 1);
        strncat(buffer, msgcopy.params[i], sizeof(buffer) - 1);
    }

    if (strlen(msgcopy.msg) > 0) {
        strncat(buffer, " :", sizeof(buffer) - 1);
        strncat(buffer, msgcopy.msg, sizeof(buffer) - 1);
    }

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
        /* Last sign of life, used to detect connection loss */
        time_t lsol = time(NULL);
        time_t lastidle = time(NULL);

        struct irc_message nick;
        struct irc_message user;

        if (sess_connect(sess) < 0)
            break;

        time(&sess->session_start);

        /* Login */
        irc_mkmessage(&nick, "NICK", sess->nick, NULL, NULL);
        irc_mkmessage(&user, "USER", sess->user, "*", "*", NULL,
                "%s", sess->real);

        sess_sendmsg(sess, &nick);
        sess_sendmsg(sess, &user);

        if (strcmp(sess->serverpass, "") != 0) {
            struct irc_message pass;

            irc_mkmessage(&pass, "PASS", sess->serverpass, NULL, NULL);
            sess_sendmsg(sess, &pass);
        }

        /* Inner loop, receive and handle data */
        while (!sess->kill) {
            int nfds;
            fd_set reads;

            struct timeval timeout = {
                .tv_sec = 1,
                .tv_usec = 0
            };

            FD_ZERO(&reads);
            FD_SET(sess->fd, &reads);

            nfds = select(sess->fd + 1, &reads, NULL, NULL, &timeout);

            if ((nfds > 0) && (FD_ISSET(sess->fd, &reads))) {
                char line[512] = {0};
                ssize_t data;
                struct irc_message msg;

                /* Update sign of life */
                time(&lsol);

                data = socket_recv(
                        sess->fd,
                        sess->buffer + sess->bufuse,
                        sizeof(sess->buffer) - sess->bufuse - 1);

                if (data <= 0) {
                    if (data == 0)
                        log_info("Server closed connection");
                    else
                        log_error("Couldn't receive data: %s", strerror(errno));

                    break;
                }

                sess->bufuse += (size_t)data;

                /*
                 * While the last blob of data received contains a full line,
                 * process it
                 */
                while (!sess_getln(sess, line, sizeof(line)))
                    if (!irc_parse_message(line, &msg))
                        sess_handle_message(sess, &msg);
                    else
                        log_warn("Failed to parse line '%s'", line);
            } else {
                /*
                 * If last sign of life is over the configured limit, send a
                 * ping to the server and inspect error code of send()
                 */
                if ((time(NULL) - lsol) > TIMEOUT) {
                    struct tm *tm = TIME_GETTIME(&lsol);
                    struct irc_message ping;

                    char buffer[TIMEBUF_MAX] = {0};

                    strftime(buffer, sizeof(buffer), STRFTIME_FORMAT, tm);

                    log_info("Last sign of life was %d seconds "
                             "ago (%s). Pinging server...",
                                time(NULL) - lsol, buffer);

                    irc_mkmessage(&ping, "PING", NULL, "%s", sess->hostname);

                    if (sess_sendmsg(sess, &ping) <= 0)
                        break;
                }
            }

            if ((time(NULL) - lastidle) >= IDLE_INTERVAL) {
                if (sess->cb.on_idle)
                    sess->cb.on_idle(sess->cb.arg, lastidle);

                time(&lastidle);
            }
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


/*
 * Logic
 */
int sess_handle_message(struct irc_session *sess, struct irc_message *msg)
{
    if (sess->cb.on_event)
        sess->cb.on_event(sess->cb.arg, msg);

    if (!strcmp(msg->command, "PING")) {
        struct irc_message pong;

        irc_mkmessage(&pong, "PONG", NULL, "%s", msg->msg);
        sess_sendmsg(sess, &pong);

        if (sess->cb.on_ping)
            sess->cb.on_ping(sess->cb.arg);

    } else if (!strcmp(msg->command, "001")) {
        if (sess->cb.on_connect)
            sess->cb.on_connect(sess->cb.arg);

    } else if (!strcmp(msg->command, "005")) {
        /* ISUPPORT */
        char capability[IRC_PARAM_MAX];
        char *eq = NULL;

        for (int i = 1; i < msg->paramcount; ++i) {
            memcpy(capability, msg->params[i], sizeof(capability));

            if (!(eq = strchr(msg->params[i], '='))) {
                irc_capability_set(sess->capabilities, msg->params[i], NULL);
            } else {
                *eq = '\0';
                irc_capability_set(sess->capabilities, msg->params[i], eq + 1);
            }
        }
    } else if (!strcmp(msg->command, "332")) {
        /* REPL_TOPIC */
        struct irc_channel *target = NULL;

        if (msg->paramcount < 2) {
            log_warn("Expected at least 2 arguments, got %d", msg->paramcount);
            goto exit_err;
        }

        if (!(target = irc_channel_get(sess->channels, msg->params[1]))) {
            log_warn("Unable to look up channel '%s'", msg->params[1]);
            goto exit_err;
        }

        irc_channel_set_topic(target, msg->msg);

    } else if (!strcmp(msg->command, "333")) {
        /* REPL_TOPIC_META */
        struct irc_channel *target = NULL;

        if (msg->paramcount < 4) {
            log_warn("Expected at least 4 arguments, got %d", msg->paramcount);

            goto exit_err;
        }

        if (!(target = irc_channel_get(sess->channels, msg->params[1]))) {
            log_warn("Unable to look up channel '%s'", msg->params[1]);

            goto exit_err;
        }

        irc_channel_set_topic_meta(target,
                msg->params[2], atoi(msg->params[3]));

    } else if (!strcmp(msg->command, "324")) {
        /* REPL_MODE */
        if (msg->paramcount < 3) {
            log_warn("Expected at least 3 arguments, got %d", msg->paramcount);

            goto exit_err;
        }

       sess_handle_mode_change(sess,
               msg->prefix,
               msg->params[1], /* channel */
               msg->params[2], /* modestring */
               msg->params, 3, msg->paramcount); /* param start and end */

    } else if (!strcmp(msg->command, "329")) {
        /* REPL_MODE2 */
        struct irc_channel *target = NULL;

        if (msg->paramcount < 3) {
            log_warn("Expected at least 3 arguments, got %d", msg->paramcount);

            goto exit_err;
        }

        if (!(target = irc_channel_get(sess->channels, msg->params[1]))) {
            log_warn("Unable to look up channel '%s'", msg->params[1]);

            goto exit_err;
        }

        irc_channel_set_created(target, atoi(msg->params[2]));

    } else if (!strcmp(msg->command, "352")) {
        /* REPL_WHO */
        char prefix[IRC_PREFIX_MAX] = {0};

        /* TODO: Error check */
        const char *prf = irc_capability_get(sess->capabilities, "PREFIX");

        size_t prfsep = strlen(prf) / 2;

        struct irc_channel *channel = NULL;
        struct irc_user *user = NULL;


        if (msg->paramcount < 7) {
            log_warn("Expected at least 7 arguments, got %d", msg->paramcount);

            goto exit_err;
        }

        if (!(channel = irc_channel_get(sess->channels, msg->params[1]))) {
            log_warn("Unable to look up channel '%s'", msg->params[1]);

            goto exit_err;
        }

        if ((user = irc_channel_get_user(channel, msg->params[5])))  {
            log_warn("User '%s' already in channel", msg->params[5]);

            goto exit_err;
        }


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


    } else if (!strcmp(msg->command, "376")) {
        /* MOTD_END */

    } else if (!strcmp(msg->command, "PRIVMSG")) {
        if (sess->cb.on_privmsg)
            sess->cb.on_privmsg(sess->cb.arg,
                    msg->prefix, msg->params[0], msg->msg);

    } else if (!strcmp(msg->command, "NOTICE")) {
        if (sess->cb.on_notice)
            sess->cb.on_notice(sess->cb.arg,
                    msg->prefix, msg->params[0], msg->msg);

    } else if (!strcmp(msg->command, "JOIN")) {
        if (!irc_user_cmp(msg->prefix, sess->nick)) {
            struct irc_message who;
            struct irc_message mode;

            const char *channel = msg->paramcount > 0
                ? msg->params[0]
                : msg->msg;

            irc_channel_add(sess->channels, channel);

            irc_mkmessage(&who, "WHO", channel, NULL, NULL);
            irc_mkmessage(&mode, "MODE", channel, NULL, NULL);

            sess_sendmsg(sess, &who);
            sess_sendmsg(sess, &mode);
        } else {
            irc_channel_add_user(
                    irc_channel_get(sess->channels, msg->params[0]),
                    msg->prefix);
        }

        if (sess->cb.on_join)
            sess->cb.on_join(sess->cb.arg, msg->prefix, msg->params[0]);

    } else if (!strcmp(msg->command, "PART")) {
        struct irc_channel *target = NULL;

        if (sess->cb.on_part)
            sess->cb.on_part(sess->cb.arg,
                msg->prefix, msg->params[0], msg->msg);

        if (!(target = irc_channel_get(sess->channels, msg->params[0]))) {
            log_warn("Unable to look up channel '%s'", msg->params[0]);

            goto exit_err;
        }

        if (!irc_user_cmp(msg->prefix, sess->nick)) {
            irc_channel_del(sess->channels, target);
        } else {
            struct irc_user *usr = irc_channel_get_user(target, msg->prefix);

            if (!usr)
                log_warn("Tried to remove unknown user '%s'", msg->prefix);
            else
                irc_channel_del_user(target, usr);
        }

    } else if (!strcmp(msg->command, "KICK")) {
        struct irc_channel *target = NULL;
        struct irc_user *utarget = NULL;

        if (sess->cb.on_kick)
            sess->cb.on_kick(sess->cb.arg,
                    msg->prefix, utarget->prefix, msg->params[0], msg->msg);

        if (!(target =  irc_channel_get(sess->channels, msg->params[0]))) {
            log_warn("Unable to look up channel '%s'", msg->params[0]);

            goto exit_err;
        }

        if (!(utarget = irc_channel_get_user(target, msg->params[1]))) {
            log_warn("Unable to look up channel user '%s'", msg->params[1]);

            goto exit_err;
        }

        if (!irc_user_cmp(msg->params[1], sess->nick))
            irc_channel_del(sess->channels, target);
        else
            irc_channel_del_user(target, utarget);

    } else if (!strcmp(msg->command, "QUIT")) {
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

    } else if (!strcmp(msg->command, "NICK")) {
        struct irc_user *user = NULL;
        const char *newnick = msg->paramcount > 0
            ? msg->params[0]
            : msg->msg;

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
            log_warn("Invalid prefix");
        }

    } else if (!strcmp(msg->command, "INVITE")) {
        if (sess->cb.on_invite)
            sess->cb.on_invite(
                    sess->cb.arg,
                    msg->prefix,
                    msg->paramcount > 0
                        ? msg->params[0]
                        : msg->msg);

    } else if (!strcmp(msg->command, "TOPIC")) {
        struct irc_channel *target = NULL;
        struct irc_message topic;

        if (sess->cb.on_topic)
            sess->cb.on_topic(
                    sess->cb.arg,
                    msg->prefix,
                    msg->params[0],
                    target->topic,
                    msg->msg);

        irc_mkmessage(&topic, "TOPIC", msg->params[0], NULL, NULL);
        sess_sendmsg(sess, &topic);

    } else if (!strcmp(msg->command, "MODE")) {
        if (msg->paramcount < 2) {
            log_warn("Expected at least 2 arguments, got %d", msg->paramcount);

            goto exit_err;
        }

        if (irc_user_cmp(msg->params[0], sess->nick))
            sess_handle_mode_change(sess,
                    msg->prefix,
                    msg->params[0],
                    msg->params[1],
                    msg->params, 2, msg->paramcount);
    }

    return 0;

exit_err:
    return 1;
}

/* TODO: Clean this sumbitch up */
int sess_handle_mode_change(
        struct irc_session *sess,
        const char *prefix,
        const char *chan,
        const char *modestr,
        char args[][IRC_PARAM_MAX],
        size_t argstart,
        size_t argmax)
{
    int set = 1; /* 1 = set, 0 = unset */
    size_t i = argstart;
    size_t j = 0;

    struct irc_channel *t = NULL;

    char usermodes[IRC_CHANNEL_PREFIX_MAX] = {0};
    char chanmodes_cpy[IRC_PARAM_MAX] = {0};

    const char *oldmodes = modestr; /* backup for signal */

    const char *prf = irc_capability_get(sess->capabilities, "PREFIX");
    const char *chn = irc_capability_get(sess->capabilities, "CHANMODES");
    const char *chanmodes[4] = {0};

    char *sep = chanmodes_cpy;

    if ((chn == NULL) || (prf == NULL)) {
        log_warn("No CHANMODES or PREFIX in ISUPPORT, "
                 "unable to determine modes!");
        return 0;
    }

    strncpy(chanmodes_cpy, chn, sizeof(chanmodes_cpy) - 1);

    chanmodes[0] = sep;

    while ((sep = strchr(sep, ','))) {
        chanmodes[++j] = sep + 1;
        *sep++ = '\0';
    }

    for (j = 0; (j < strlen(prf) / 2 - 1) && (j < IRC_CHANNEL_PREFIX_MAX); ++j)
        usermodes[j] = prf[j + 1];

    if (!(t = irc_channel_get(sess->channels, chan))) {
        log_warn("Unable to look up channel '%s'", chan);
        return 0;
    }

    while (*modestr) {
        const char *arg = NULL;

        if ((*modestr == '+') || (*modestr == '-'))
            set = (*modestr++ == '+') ? 1 : 0;

        if ((strchr(chanmodes[MODE_LIST],   *modestr)) ||
            (strchr(chanmodes[MODE_REQARG], *modestr)) ||
            ((strchr(chanmodes[MODE_SETARG], *modestr) && set))) {

            if (i < argmax) {
                arg = args[i++];
            } else {
                log_error("too few mode parameters");
                return 0;
            }

            if (set) {
                if (strchr(chanmodes[MODE_LIST], *modestr))
                    /* Add list entry */
                    irc_channel_add_mode(t, *modestr, arg);
                else
                    /* Set flag */
                    irc_channel_set_mode(t, *modestr, arg);
            } else {
                if (strchr(chanmodes[MODE_LIST], *modestr))
                    /* Delete list entry */
                    irc_channel_del_mode(t, *modestr, arg);
                else
                    /* Unset flag */
                    irc_channel_unset_mode(t, *modestr);
            }
        } else if (strchr(usermodes, *modestr)) {
            if (i < argmax) {
                arg = args[i++];
            } else {
                log_error("too few mode parameters");
                return 0;
            }

            struct irc_user *usr = irc_channel_get_user(t, arg);

            if (!usr)
                log_warn("unknown user '%s' for channel '%s'", arg, chan);
            else
                if (set)
                    irc_channel_user_set_mode(usr, *modestr);
                else
                    irc_channel_user_unset_mode(usr, *modestr);

        } else {
            /* Unknown flags or whensets when not set */
            if (set)
                irc_channel_set_mode(t, *modestr, NULL);
            else
                irc_channel_unset_mode(t, *modestr);
        }

        if (set) {
            if (sess->cb.on_mode_set)
                sess->cb.on_mode_set(sess->cb.arg,
                        prefix, chan, *modestr, arg);
        } else {
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
