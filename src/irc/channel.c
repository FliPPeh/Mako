#include "irc/session.h"
#include "irc/channel.h"
#include "util/log.h"
#include "util/util.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>


/* List management */
void irc_channel_free(void *data)
{
    struct irc_channel *channel = (struct irc_channel *)data;

    hashtable_free(channel->users);
    hashtable_free(channel->modes);

    free(channel);
}

void irc_mode_free(void *data)
{
    struct irc_mode *mode = (struct irc_mode *)data;

    if (mode->type == IRC_MODE_LIST)
        list_free_all(mode->value.args, list_free_wrapper, NULL);

    free(mode);
}

/* Channel management */
int irc_channel_add(struct irc_session *sess, const char *chan)
{
    hashtable_insert(sess->channels,
            strdup(chan), _irc_channel_new(chan, sess));

    return 0;
}

int irc_channel_del(struct irc_session *sess, struct irc_channel *chan)
{
    hashtable_remove(sess->channels, chan->name);

    return 0;
}

struct irc_channel *irc_channel_get(struct irc_session *sess, const char *chan)
{
    return hashtable_lookup(sess->channels, chan);
}

int irc_channel_set_topic(struct irc_channel *chan, const char *topic)
{
    strncpy(chan->topic, topic, sizeof(chan->topic) - 1);

    return 0;
}

int irc_channel_set_created(struct irc_channel *chan, time_t created)
{
    chan->created = created;

    return 0;
}

int irc_channel_set_topic_meta(
        struct irc_channel *chan, const char *setter, time_t set)
{
    strncpy(chan->topic_setter, setter, sizeof(chan->topic_setter) - 1);
    chan->topic_set = set;

    return 0;
}


/* Channel user management */
int irc_channel_add_user(struct irc_channel *chan, const char *prefix)
{
    hashtable_insert(chan->users, strdup(prefix), _irc_user_new(prefix, chan));

    return 0;
}

int irc_channel_del_user(struct irc_channel *chan, struct irc_user *user)
{
    hashtable_remove(chan->users, user->prefix);

    return 0;
}

struct irc_user *irc_channel_get_user(struct irc_channel *chn, const char *usr)
{
    /* No '!' probably means no prefix, so we'll have to search */
    if (!strchr(usr, '!'))
        return irc_channel_get_user_by_nick(chn, usr);
    else
        return hashtable_lookup(chn->users, usr);
}

struct irc_user *irc_channel_get_user_by_nick(
        struct irc_channel *chan, const char *nick)
{
    struct hashtable_iterator iter;

    void *prefix;
    void *user;

    hashtable_iterator_init(&iter, chan->users);
    while (hashtable_iterator_next(&iter, &prefix, &user))
        if (!irc_user_cmp(prefix, nick))
            return user;

    return NULL;
}


int irc_channel_rename_user(
        struct irc_channel *chan, struct irc_user *user, const char *pref)
{
    struct irc_user *newuser = _irc_user_new(pref, chan);

    /* Copy modes from old to new */
    strncpy(newuser->modes, user->modes, sizeof(newuser->modes) - 1);

    /* Add new user and remove old user */
    hashtable_insert(chan->users, strdup(pref), newuser);
    hashtable_remove(chan->users, user->prefix);

    return 0;
}

/*
 * Modes
 */
int irc_channel_set_mode(struct irc_channel *c, char mode, const char *arg)
{
    enum irc_mode_type type = _irc_channel_mode_type(c->session, mode);
    if (type != IRC_MODE_CHANUSER) {
        struct irc_mode *m = hashtable_lookup(c->modes, &mode);

        if (m == NULL) {
            m = _irc_mode_new(mode, type);
            hashtable_insert(c->modes, chrdup(mode), m);
        }

        if (type == IRC_MODE_LIST) {
            /* Add to list or create list */
            m->value.args = list_append(m->value.args, strdup(arg));
        } else if (type == IRC_MODE_SINGLE) {
            /* Replace or place */
            strncpy(m->value.arg, arg, sizeof(m->value.arg) - 1);
        }
    } else {
        /* Find channel user and apply mode to them */
        struct irc_user *usr = irc_channel_get_user(c, arg);

        if (!usr) {
            log_warn("%s: unknown user '%s'", c->name, arg);
            return 1;
        }

        log_debug("%s: set user mode +%c for '%s'", c->name, mode, usr->prefix);
        irc_channel_user_set_mode(usr, mode);
    }

    return 0;
}

int irc_channel_unset_mode(struct irc_channel *c, char mode, const char *arg)
{
    enum irc_mode_type type = _irc_channel_mode_type(c->session, mode);

    if (type != IRC_MODE_CHANUSER) {
        struct irc_mode *m = hashtable_lookup(c->modes, &mode);

        if (m == NULL) {
            /* Mode not set, can not unset */
            log_warn("%s: mode +%c not set, can not unset!", c->name, mode);
            return 1;
        }

        if (type == IRC_MODE_LIST) {
            /* Try to locate mode in list */
            struct list *pos = NULL;

            if ((pos = list_find_custom(m->value.args, arg,
                                        _irc_channel_mode_strcmp, NULL)))

                m->value.args = list_remove_link(m->value.args, pos,
                                                 list_free_wrapper, NULL);

            if (!list_length(m->value.args))
                hashtable_remove(c->modes, &mode);

        } else {
            /* Remove the whole thing */
            hashtable_remove(c->modes, &mode);
        }

    } else {
        /* Find channel user and apply mode to them */
        struct irc_user *usr = irc_channel_get_user(c, arg);

        if (!usr) {
            log_warn("%s: unknown user '%s'", c->name, arg);
            return 1;
        }

        log_debug("%s: set user mode +%c for '%s'", c->name, mode, usr->prefix);
        irc_channel_user_unset_mode(usr, mode);
    }

    return 0;

}

enum irc_mode_type _irc_channel_mode_type(struct irc_session *sess, char mode)
{
    if (strchr(sess->chanmodes[MODE_LIST], mode))
        return IRC_MODE_LIST;
    else if (strchr(sess->chanmodes[MODE_REQARG], mode)
          || strchr(sess->chanmodes[MODE_SETARG], mode))
        return IRC_MODE_SINGLE;
    else if (strchr(sess->usermodes, mode))
        return IRC_MODE_CHANUSER;
    else
        return IRC_MODE_SIMPLE;
}

int _irc_channel_mode_strcmp(const void *list, const void *search, void *ud)
{
    (void)ud;

    return strcmp(list, search);
}

int irc_channel_user_set_mode(struct irc_user *u, char mode)
{
    if (!strchr(u->modes, mode) && (strlen(u->modes) < IRC_FLAGS_MAX - 1))
        u->modes[strlen(u->modes)] = mode;

    return 0;
}

int irc_channel_user_unset_mode(struct irc_user *u, char mode)
{
    char *pos = NULL;

    if ((pos = strchr(u->modes, mode))) {
        memmove(pos, pos + 1, strlen(u->modes) - (size_t)(pos - u->modes));

        return 0;
    }

    return 1;
}

/* Utility functions */
int _irc_channel_find_by_name(const void *list, const void *search, void *ud)
{
    return strcasecmp(((struct irc_channel *)list)->name, search);
}

int _irc_channel_user_find_by_prefix(
        const void *list, const void *search, void *ud)
{
    return irc_user_cmp(((struct irc_user *)list)->prefix, search);
}

int _irc_mode_find_by_flag(const void *list, const void *data, void *ud)
{
    return ((struct irc_mode *)list)->mode != *((char *)data);
}


struct irc_user *_irc_user_new(const char *pref, struct irc_channel *c)
{
    struct irc_user *usr = malloc(sizeof(*usr));

    memset(usr, 0, sizeof(*usr));
    strncpy(usr->prefix, pref, sizeof(usr->prefix) - 1);
    usr->channel = c;

    return usr;
}

struct irc_channel *_irc_channel_new(const char *name, struct irc_session *s)
{
    struct irc_channel *chn = malloc(sizeof(*chn));

    memset(chn, 0, sizeof(*chn));
    strncpy(chn->name, name, sizeof(chn->name) - 1);

    chn->users = hashtable_new_with_free(ascii_hash, ascii_equal, free, free);
    chn->modes = hashtable_new_with_free(char_hash, char_equal,
                                         free,      irc_mode_free);
    chn->session = s;

    return chn;
}

struct irc_mode *_irc_mode_new(char mode, enum irc_mode_type type)
{
    struct irc_mode *mde = malloc(sizeof(*mde));

    memset(mde, 0, sizeof(*mde));
    mde->mode = mode;
    mde->type = type;

    return mde;
}
