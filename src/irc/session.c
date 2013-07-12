#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "irc/irc.h"
#include "irc/util.h"
#include "irc/session.h"
#include "net/socket.h"

#include "util/log.h"
#include "util/list.h"
#include "util/util.h"

/*
 * Util
 */
int sess_getln(struct irc_session *sess, char *linebuf, size_t linebufsiz)
{
    char *crlf = NULL;
    struct log_context log;

    log_derive(&log, NULL, __func__);

    if ((crlf = strstr(sess->buffer, "\r\n"))) {
        size_t len = crlf - sess->buffer;

        /* bufuse = buffer usage after line has been taken out */
        sess->bufuse -= len + 2;

        /* Copy line over to line buffer */
        memset(linebuf, 0, linebufsiz);
        strncpy(linebuf, sess->buffer, MIN(len, linebufsiz - 1));

        /* Shift buffer forward, starting from the first byte after \r\n to the
         * end */
        memmove(sess->buffer, crlf + 2, sizeof(sess->buffer) - len + 2);

        log_debug(&log, "<< %s", linebuf);
        log_destroy(&log);
        return 0;
    }

    log_destroy(&log);
    return 1;
}

int sess_sendmsg(struct irc_session *sess, const struct irc_message *msg)
{
    int i;
    char buffer[IRC_MESSAGE_MAX] = {0};
    struct irc_message msgcopy = *msg;
    struct log_context log;

    log_derive(&log, NULL, __func__);

    if (sess->cb.on_send_message)
        if (sess->cb.on_send_message(sess->cb.arg, &msgcopy))
            return 0;

    strncat(buffer, msgcopy.command, sizeof(buffer) - 1);

    for (i = 0; i < msgcopy.paramcount; ++i) {
        strncat(buffer, " ", sizeof(buffer) - 1);
        strncat(buffer, msgcopy.params[i], sizeof(buffer) - 1);
    }

    if (strlen(msgcopy.msg) > 0) {
        strncat(buffer, " :", sizeof(buffer) - 1);
        strncat(buffer, msgcopy.msg, sizeof(buffer) - 1);
    }

    log_debug(&log, ">> %s", buffer);

    return socket_sendfln(sess->fd, "%s", buffer);
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
    log_derive(&sess->log, NULL, "session");

    /* Jump into mainloop */
    while (!sess->kill) {
        /* Last sign of life, used to detect connection loss */
        time_t lsol = time(NULL);
        time_t lastidle = time(NULL);

        struct irc_message nick;
        struct irc_message user;

        if (sess_connect(sess) < 0)
            break;

        /* Login */
        irc_mkmessage(&nick, "NICK", sess->nick, NULL, NULL);
        irc_mkmessage(&user, "USER", sess->user, "*", "*", NULL,
                "%s", sess->real);

        sess_sendmsg(sess, &nick);
        sess_sendmsg(sess, &user);

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
                        log_info(&sess->log, "Server closed connection");
                    else
                        log_error(&sess->log,
                                "Couldn't receive data: %s", strerror(errno));

                    break;
                }

                sess->bufuse += data;

                /*
                 * While the last blob of data received contains a full line,
                 * process it
                 */
                while (!sess_getln(sess, line, sizeof(line)))
                    if (!irc_parse_message(line, &msg))
                        sess_handle_message(sess, &msg);
                    else
                        log_warn(&sess->log, "Failed to parse line '%s'", line);
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

                    log_info(&sess->log, "Last sign of life was %d seconds "
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
        list_free_all(sess->channels, irc_channel_free);
        list_free_all(sess->capabilities, free);

        sess->channels = NULL;
        sess->capabilities = NULL;

        sess_disconnect(sess);
    }

    log_destroy(&sess->log);

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
        int i;
        char capability[IRC_PARAM_MAX];
        char *eq = NULL;

        for (i = 1; i < msg->paramcount; ++i) {
            memcpy(capability, msg->params[i], sizeof(capability));
            eq = strchr(msg->params[i], '=');

            if (!eq) {
                irc_capability_set(&sess->capabilities, msg->params[i], NULL);
            } else {
                *eq = '\0';
                irc_capability_set(&sess->capabilities, msg->params[i], eq + 1);
            }
        }
    } else if (!strcmp(msg->command, "332")) {
        /* REPL_TOPIC */
        struct irc_channel *target = NULL;

        if (msg->paramcount < 2)
            return 0;

        if ((target = irc_channel_get(sess->channels, msg->params[1])))
            strncpy(target->topic, msg->msg, sizeof(target->topic) - 1);

    } else if (!strcmp(msg->command, "333")) {
        /* REPL_TOPIC_META */
        struct irc_channel *target = NULL;

        if (msg->paramcount < 4)
            return 0;

        if ((target = irc_channel_get(sess->channels, msg->params[1]))) {
            strncpy(target->topic_setter, msg->params[2],
                    sizeof(target->topic_setter) - 1);

            target->topic_set = atoi(msg->params[3]);
        }
    } else if (!strcmp(msg->command, "324")) {
        /* REPL_MODE */
        if (msg->paramcount < 3)
            return 0;

        sess_handle_mode_change(sess,
                msg->prefix,
                msg->params[1], /* channel */
                msg->params[2], /* modestring */
                msg->params, 3); /* param start */
    } else if (!strcmp(msg->command, "329")) {
        /* REPL_MODE2 */
        struct irc_channel *target = NULL;

        if (msg->paramcount < 3)
            return 0;

        if ((target = irc_channel_get(sess->channels, msg->params[1])))
            target->created = atoi(msg->params[2]);
    } else if (!strcmp(msg->command, "352")) {
        /* REPL_WHO */
        int j;
        int i;
        char prefix[IRC_PREFIX_MAX] = {0};
        const char *prf = irc_capability_get(sess->capabilities, "PREFIX");

        size_t prfsep = strlen(prf) / 2;

        struct irc_channel *channel = irc_channel_get(
                sess->channels, msg->params[1]);

        struct irc_user *user = NULL;

        if (msg->paramcount < 7)
            return 0;

        snprintf(prefix, sizeof(prefix), "%s!%s@%s",
                msg->params[5],
                msg->params[2],
                msg->params[3]);

        irc_channel_add_user(channel, prefix);
        user = irc_channel_get_user(channel, msg->params[5]);

        /*
         * Go over every flag within the users mode string and match them
         * against the server's PREFIX to get the actual mode.
         */
        for (i = 0; i < strlen(msg->params[6]); ++i)
            for (j = 0; j < prfsep - 1; ++j)
                if (msg->params[6][i] == prf[prfsep + j + 1])
                    irc_channel_user_set_mode(user, prf[j + 1]);

    } else if (!strcmp(msg->command, "376")) {
        /* MOTD_END */

    } else if (!strcmp(msg->command, "PRIVMSG")) {
        if (sess->cb.on_privmsg)
            sess->cb.on_privmsg(
                    sess->cb.arg,
                    msg->prefix,
                    msg->params[0],
                    msg->msg);

    } else if (!strcmp(msg->command, "NOTICE")) {
        if (sess->cb.on_notice)
            sess->cb.on_notice(
                    sess->cb.arg,
                    msg->prefix,
                    msg->params[0],
                    msg->msg);

    } else if (!strcmp(msg->command, "JOIN")) {
        if (!irc_user_cmp(msg->prefix, sess->nick)) {
            struct irc_message who;
            struct irc_message mode;

            irc_channel_add(&sess->channels, msg->params[0]);

            irc_mkmessage(&who, "WHO", msg->params[0], NULL, NULL);
            irc_mkmessage(&mode, "MODE", msg->params[0], NULL, NULL);

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
        struct irc_channel *target = irc_channel_get(
                sess->channels,
                msg->params[0]);

        if (sess->cb.on_part)
            sess->cb.on_part(
                sess->cb.arg,
                msg->prefix,
                msg->params[0],
                msg->msg);

        if (!irc_user_cmp(msg->prefix, sess->nick))
            irc_channel_del(&sess->channels, target);
        else
            irc_channel_del_user(target,
                    irc_channel_get_user(target, msg->prefix));

    } else if (!strcmp(msg->command, "KICK")) {
        struct irc_channel *target = irc_channel_get(
                sess->channels,
                msg->params[0]);

        if (sess->cb.on_kick)
            sess->cb.on_kick(
                    sess->cb.arg,
                    msg->prefix,
                    msg->params[1], /* TODO: lookup for full prefix */
                    msg->params[0],
                    msg->msg);

        if (!irc_user_cmp(msg->params[1], sess->nick))
            irc_channel_del(&sess->channels, target);
        else
            irc_channel_del_user(target,
                    irc_channel_get_user(target, msg->params[1]));

    } else if (!strcmp(msg->command, "QUIT")) {
        struct list *pos = NULL;

        if (sess->cb.on_quit)
            sess->cb.on_quit(sess->cb.arg, msg->prefix, msg->msg);

        LIST_FOREACH(sess->channels, pos)
            irc_channel_del_user(
                    pos->data,
                    irc_channel_get_user(pos->data, msg->prefix));

    } else if (!strcmp(msg->command, "NICK")) {
        struct list *pos = NULL;
        struct irc_user *user = NULL;
        const char *newnick = msg->paramcount > 0
            ? msg->params[0]
            : msg->msg;

        char *oldpostfix = NULL;
        char newprefix[IRC_PREFIX_MAX] = {0};
        char oldprefix[IRC_PREFIX_MAX] = {0};

        oldpostfix = strchr(msg->prefix, '!');

        strncpy(oldprefix, msg->prefix, sizeof(oldprefix) - 1);
        strncat(newprefix, newnick, sizeof(newprefix) - 1);
        strncat(newprefix, oldpostfix, sizeof(newprefix) - 1);

        if (!irc_user_cmp(msg->prefix, sess->nick)) {
            memset(sess->nick, 0, sizeof(sess->nick));
            strncpy(sess->nick, newnick, sizeof(sess->nick) - 1);
        }

        LIST_FOREACH(sess->channels, pos)
            if ((user = irc_channel_get_user(pos->data, msg->prefix)))
                strncpy(user->prefix, newprefix, IRC_PREFIX_MAX - 1);

        if (sess->cb.on_nick)
            sess->cb.on_nick(sess->cb.arg, oldprefix, newprefix);

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

        if ((target = irc_channel_get(sess->channels, msg->params[1]))) {
            if (sess->cb.on_topic)
                sess->cb.on_topic(
                        sess->cb.arg,
                        msg->prefix,
                        msg->params[0],
                        target->topic,
                        msg->msg);

            irc_mkmessage(&topic, "TOPIC", msg->params[0], NULL, NULL);
            sess_sendmsg(sess, &topic);
        }

    } else if (!strcmp(msg->command, "MODE")) {
        if (msg->paramcount < 2)
            return 0;

        if (irc_user_cmp(msg->params[0], sess->nick))
            return sess_handle_mode_change(sess,
                    msg->prefix,
                    msg->params[0],
                    msg->params[1],
                    msg->params, 2);
    }

    return 1;
}

int sess_handle_mode_change(
        struct irc_session *sess,
        const char *prefix,
        const char *chan,
        const char *modestr,
        char args[][IRC_PARAM_MAX],
        size_t argstart)
{
    int set = 1; /* 1 = set, 0 = unset */
    int i = argstart;
    int j = 0;

    char usermodes[IRC_CHANNEL_PREFIX_MAX] = {0};
    char chanmodes_cpy[IRC_PARAM_MAX] = {0};

    const char *oldmodes = modestr; /* backup for signal */

    const char *prf = irc_capability_get(sess->capabilities, "PREFIX");
    const char *chn = irc_capability_get(sess->capabilities, "CHANMODES");
    const char *chanmodes[4] = {0};

    char *sep = chanmodes_cpy;

    memcpy(chanmodes_cpy, chn, sizeof(chanmodes_cpy));

    chanmodes[0] = sep;

    while ((sep = strstr(sep, ","))) {
        chanmodes[j] = sep + 1;
        *sep++ = '\0';
    }

    for (j = 0; (j < strlen(prf) / 2 - 1) && (j < IRC_CHANNEL_PREFIX_MAX); ++j)
        usermodes[j] = prf[j + 1];

    struct irc_channel *t = irc_channel_get(sess->channels, chan);
    if (!t)
        return 0;

    while (*modestr) {
        const char *arg = NULL;

        if ((*modestr == '+') || (*modestr == '-'))
            set = (*modestr++ == '+') ? 1 : 0;

        if ((strchr(chanmodes[MODE_LIST],    *modestr)) ||
            (strchr(chanmodes[MODE_REQARG],  *modestr)) ||
            (strchr(chanmodes[MODE_SETARG], *modestr) && !set)) {

            arg = args[i++];

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
            arg = args[i++];

            if (set)
                irc_channel_user_set_mode(irc_channel_get_user(t, arg),
                        *modestr);
            else
                irc_channel_user_unset_mode(irc_channel_get_user(t, arg),
                        *modestr);

        } else {
            /* Unknown flags or whensets when not set */
            if (set)
                irc_channel_set_mode(t, *modestr, NULL);
            else
                irc_channel_unset_mode(t, *modestr);
        }

        if (set) {
            if (sess->cb.on_mode_set)
                sess->cb.on_mode_set(
                        sess->cb.arg,
                        prefix,
                        chan,
                        *modestr,
                        arg);
        } else {
            if (sess->cb.on_mode_unset)
                sess->cb.on_mode_unset(
                        sess->cb.arg,
                        prefix,
                        chan,
                        *modestr,
                        arg);
        }

        modestr++;
    }

    if (sess->cb.on_modes)
        sess->cb.on_modes(sess->cb.arg, prefix, chan, oldmodes);

    return 1;
}
