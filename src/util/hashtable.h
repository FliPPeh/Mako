#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include "list.h"

#include <stdlib.h>
#include <stdbool.h>

#define HASHTABLE_INIT_SIZE 4
#define AUTOREHASH // undefine to never automatically rehash tables.
#define HASHTABLE_MINLOAD 0.12 // minimum load factor for rehash on delete
#define HASHTABLE_MAXLOAD 0.75 // maximum load factor for rehash on insert

typedef size_t (*hashtable_hash_func)(const void *key);
typedef int (*hashtable_equality_func)(const void *ka, const void *kb);
typedef void (*hashtable_delete_func)(void *data);

struct hashtable
{
    size_t bucket_count;
    size_t entries;
    struct list **buckets;

    hashtable_hash_func key_hash;
    hashtable_equality_func key_equal;

    hashtable_delete_func free_key;
    hashtable_delete_func free_value;
};

// For list items
struct hashtable_entry
{
    void *key;
    void *value;
};

struct hashtable_iterator
{
    const struct hashtable *table;

    size_t bucket;
    struct list *lst;
};

struct hashtable_entry *_hashtable_entry_new(void *key, void *value);

struct hashtable *hashtable_new(hashtable_hash_func hsh,
                                hashtable_equality_func eq);

struct hashtable *hashtable_new_with_free(hashtable_hash_func hsh,
                                          hashtable_equality_func eq,
                                          hashtable_delete_func fkey,
                                          hashtable_delete_func fvalue);

struct hashtable *hashtable_new_real(size_t buckets,
                                     hashtable_hash_func hsh,
                                     hashtable_equality_func eq,
                                     hashtable_delete_func fkey,
                                     hashtable_delete_func fvalue);

struct hashtable *hashtable_new_from(struct hashtable *orig);

void hashtable_free(struct hashtable *table);

/*
 * Inserts an item into the hashmap. Should key already exist, it replaces its
 * value with the new value, freeing the old one if a deleter function for the
 * value was given.
 */
void hashtable_insert(struct hashtable *table, void *key, void *value);

/*
 * Removing and clearing. Remove removes the given key from the hashtable (if
 * it exists), clear removes all keys from the hashtable. Their _shallow
 * variants also remove the key/value pairs from the hashtable, but do not call
 * the release functions for either (in other words, they only remove the
 * entries from the hashtable, but don't free their memory).
 */
void hashtable_remove(struct hashtable *table, const void *key);
void hashtable_remove_shallow(struct hashtable *table, const void *key);

void hashtable_clear(struct hashtable *table);
void hashtable_clear_shallow(struct hashtable *table);

void _hashtable_clear_internal(struct hashtable *table,
                               list_delete_func deleter);

void _hashtable_remove_internal(struct hashtable *table,
                                const void *key,
                                list_delete_func deleter);


int _hashtable_find(const void *listdata, const void *key, void *ud);

void *hashtable_lookup(const struct hashtable *table, const void *key);
bool hashtable_contains(const struct hashtable *table, const void *key);
size_t hashtable_size(const struct hashtable *table);

double hashtable_load_factor(const struct hashtable *table);

/*
 * Rehash the hashtable by (depending on the load factor, see HASHTABLE_MAXLOAD,
 * HASHTABLE_MINLOAD) increasing or decreasing the number of buckets to reduce
 * seek time (for many elements) and reduce memory usage (for few elements).
 * The order of the contained entries may change, but the amount will not.
 */
void hashtable_rehash(struct hashtable *table);

struct list *hashtable_keys(const struct hashtable *table);
struct list *hashtable_values(const struct hashtable *table);

/*
 * Set operations on hashtables.
 *
 * hashtable_union() merges all key/value pairs from B into A.
 * hashtable_complement() removes all keys not in B from A.
 *
 * These functions are intended for merging changes that could not be done in
 * place while iterating A. Hashtable A takes ownership of added key/value paris
 * so B must not free them. If B was created using hashtable_new_from(), perform
 * a shallow clear on B before destroying it.
 */
void hashtable_union(struct hashtable *a, struct hashtable *b);
void hashtable_complement(struct hashtable *a, struct hashtable *b);

void _hashtable_free_element(void *elem, void *userdata);
void _hashtable_free_element_shallow(void *elem, void *ud);

void hashtable_iterator_init(struct hashtable_iterator *iter,
                             const struct hashtable *t);

bool hashtable_iterator_next(struct hashtable_iterator *iter,
                             void **tkey,
                             void **tval);

#define HASHTABLE_NEW(T) hashtable_new(HASHFUNC_FOR(T), EQUALFUNC_FOR(T))

#define HASHTABLE_NEW_WRITH_FREE(T, fkey, fval) \
    hashtable_new_with_free(HASHFUNC_FOR(T),    \
                            EQUALFUNC_FOR(T), (fkey), (fval))

// TODO: More functions
#define HASHFUNC_FOR(T) _Generic((T)NULL, \
        const char *: str_hash)

#define EQUALFUNC_FOR(T) _Generic((T)NULL, \
        const char *: str_equal)

size_t ascii_hash(const void *str);
int ascii_equal(const void *a, const void *b);

size_t str_hash(const void *str);
int str_equal(const void *a, const void *b);

#endif /* defined _HASHTABLE_H */
