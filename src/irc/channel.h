#ifndef IRC_CHANNEL_H
#define IRC_CHANNEL_H

#include "irc/irc.h"

#include <libutil/container/hashtable.h>

enum irc_mode_type
{
    IRC_MODE_SIMPLE,  /* +c, +t, ... => argument        */
    IRC_MODE_SINGLE,  /* +k, +l, ... => arg1, arg2, ... */
    IRC_MODE_LIST,    /* +b, ...     => no argument     */
    IRC_MODE_CHANUSER /* +o, +v, ... => nick argument   */
};

struct irc_mode
{
    char mode;

    enum irc_mode_type type;

    union irc_mode_argument
    {
        char arg[IRC_PARAM_MAX];
        struct list *args;
    } value;
};

struct irc_user
{
    struct irc_channel *channel;

    char prefix[IRC_PREFIX_MAX];
    char modes[IRC_FLAGS_MAX];
};

struct irc_channel
{
    struct irc_session *session;

    char name[IRC_CHANNEL_MAX];
    time_t created;

    char topic[IRC_TOPIC_MAX];
    char topic_setter[IRC_PREFIX_MAX];
    time_t topic_set;

    struct hashtable *users;
    struct hashtable *modes;
};

/* Hashtable management */
void irc_channel_free(void *data);
void irc_mode_free(void *data);

/* Channel management */
int irc_channel_add(struct irc_session *sess, const char *chan);
int irc_channel_del(struct irc_session *sess, struct irc_channel *chan);
struct irc_channel *irc_channel_get(struct irc_session *sess, const char *chan);

int irc_channel_set_created(struct irc_channel *chan, time_t created);
int irc_channel_set_topic(struct irc_channel *chan, const char *topic);
int irc_channel_set_topic_meta(
        struct irc_channel *chan, const char *setter, time_t set);

/* Channel user management */
int irc_channel_add_user(struct irc_channel *chan, const char *prefix);
int irc_channel_del_user(struct irc_channel *chan, struct irc_user *user);
struct irc_user *irc_channel_get_user(struct irc_channel *chn, const char *usr);

struct irc_user *irc_channel_get_user_by_nick(struct irc_channel *chan,
                                              const char *nick);

struct irc_user *irc_channel_get_user_by_mask(struct irc_channel *chan,
                                              const char *mask);

int irc_channel_rename_user(
        struct irc_channel *chan, struct irc_user *user, const char *pref);

/*
 * Modes
 */

/* Channel flags with or without arguments */
int irc_channel_set_mode(struct irc_channel *c, char mode, const char *arg);
int irc_channel_unset_mode(struct irc_channel *c, char mode, const char *arg);

enum irc_mode_type _irc_channel_mode_type(struct irc_session *sess, char mode);
int _irc_channel_mode_strcmp(const void *list, const void *search, void *ud);

/* User flags */
int irc_channel_user_set_mode(struct irc_user *u, char mode);
int irc_channel_user_unset_mode(struct irc_user *u, char mode);

/* Utility functions */
int _irc_channel_find_by_name(const void *list, const void *search, void *ud);
int _irc_channel_user_find_by_prefix(
        const void *list, const void *search, void *ud);

int _irc_mode_find_by_flag(const void *list, const void *data, void *ud);

struct irc_user *_irc_user_new(const char *pref, struct irc_channel *c);
struct irc_channel *_irc_channel_new(const char *name, struct irc_session *s);
struct irc_mode *_irc_mode_new(char mode, enum irc_mode_type type);

#endif /* defined IRC_CHANNEL_H */
