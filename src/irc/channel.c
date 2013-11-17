#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "irc/channel.h"
#include "util/log.h"
#include "util/util.h"


/* List management */
void irc_channel_free(void *data)
{
    struct irc_channel *channel = (struct irc_channel *)data;

    hashtable_free(channel->users);
    list_free_all(channel->modes, &list_free_wrapper, NULL);

    free(channel);
}

/* Channel management */
int irc_channel_add(struct hashtable *chans, const char *chan)
{
    hashtable_insert(chans, strdup(chan), _irc_channel_new(chan));

    return 0;
}

int irc_channel_del(struct hashtable *chans, struct irc_channel *chan)
{
    hashtable_remove(chans, chan->name);

    return 0;
}

struct irc_channel *irc_channel_get(struct hashtable *chans, const char *chan)
{
    return hashtable_lookup(chans, chan);
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
    hashtable_insert(chan->users, strdup(prefix), _irc_user_new(prefix));

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
    struct irc_user *newuser = _irc_user_new(pref);

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

/* Mode lists (+b, +e, ...) */
int irc_channel_add_mode(struct irc_channel *c, char mode, const char *arg)
{
    c->modes = list_append(c->modes, _irc_mode_new(mode, arg));

    return 0;
}

int irc_channel_del_mode(struct irc_channel *c, char mode, const char *arg)
{
    struct list *pos = NULL;

again:
    LIST_FOREACH(c->modes, pos) {
        /* If arg != NULL, delete mode with that arg, otherwise remove all
         * modes with that flag */
        struct irc_mode *ptr = pos->data;

        if ((ptr->mode == mode) && (!arg || !strcasecmp(ptr->arg, arg))) {
            c->modes = list_remove_link(c->modes, pos, list_free_wrapper, NULL);
            goto again;
        }
    }

    return 0;
}

/* Single flags with or without arguments */
int irc_channel_set_mode(struct irc_channel *c, char mode, const char *arg)
{
    struct list *pos = NULL;
    struct irc_mode *m = NULL;

    /* Try to find mode in list first */
    if ((pos = list_find_custom(
                    c->modes, &mode, _irc_mode_find_by_flag, NULL))) {
        m = list_data(pos, struct irc_mode *);

        /* update */
        m->mode = mode;

        if (arg)
            strncpy(m->arg, arg, sizeof(m->arg) - 1);
        else
            memset(m->arg, 0, sizeof(m->arg));
    } else {
        c->modes = list_append(c->modes, _irc_mode_new(mode, arg));
    }

    return 0;
}

int irc_channel_unset_mode(struct irc_channel *c, char mode)
{
    struct list *pos = NULL;

    if ((pos = list_find_custom(
                    c->modes, &mode, _irc_mode_find_by_flag, NULL)))
        c->modes = list_remove_link(c->modes, pos, list_free_wrapper, NULL);

    return 0;
}

int irc_channel_user_set_mode(struct irc_user *u, char mode)
{
    if (!strchr(u->modes, mode) && strlen(u->modes) < IRC_FLAGS_MAX - 1)
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


struct irc_user *_irc_user_new(const char *pref)
{
    struct irc_user *usr = malloc(sizeof(*usr));

    memset(usr, 0, sizeof(*usr));
    strncpy(usr->prefix, pref, sizeof(usr->prefix) - 1);

    return usr;
}

struct irc_channel *_irc_channel_new(const char *name)
{
    struct irc_channel *chn = malloc(sizeof(*chn));

    memset(chn, 0, sizeof(*chn));
    strncpy(chn->name, name, sizeof(chn->name) - 1);

    chn->users = hashtable_new_with_free(ascii_hash, ascii_equal, free, free);

    return chn;
}

struct irc_mode *_irc_mode_new(char mode, const char *arg)
{
    struct irc_mode *mod  = malloc(sizeof(*mod));

    memset(mod, 0, sizeof(*mod));

    mod->mode = mode;
    strncpy(mod->arg, arg ? arg : "", sizeof(mod->arg) - 1);

    return mod;
}
