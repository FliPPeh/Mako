#include "hashtable.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct hashtable_entry *_hashtable_entry_new(void *key, void *value)
{
    struct hashtable_entry *e = malloc(sizeof(*e));

    memset(e, 0, sizeof(*e));

    e->key = key;
    e->value = value;

    return e;
}

struct hashtable *hashtable_new(hashtable_hash_func hsh,
                                hashtable_equality_func eq)
{
    return hashtable_new_with_free(hsh, eq, NULL, NULL);
}

struct hashtable *hashtable_new_with_free(hashtable_hash_func hsh,
                                          hashtable_equality_func eq,
                                          hashtable_delete_func fkey,
                                          hashtable_delete_func fvalue)
{
    return hashtable_new_real(HASHTABLE_INIT_SIZE, hsh, eq, fkey, fvalue);
}

struct hashtable *hashtable_new_real(size_t buckets,
                                     hashtable_hash_func hsh,
                                     hashtable_equality_func eq,
                                     hashtable_delete_func fkey,
                                     hashtable_delete_func fvalue)
{
    assert(hsh != NULL);
    assert(eq != NULL);

    struct hashtable *tab = malloc(sizeof(*tab));

    memset(tab, 0, sizeof(*tab));

    tab->bucket_count = buckets;
    tab->buckets = malloc(sizeof(struct list *) * tab->bucket_count);
    tab->key_hash = hsh;
    tab->key_equal = eq;
    tab->free_key = fkey;
    tab->free_value = fvalue;

    memset(tab->buckets, 0, sizeof(struct list *) * tab->bucket_count);

    return tab;
}

struct hashtable *hashtable_new_from(struct hashtable *orig)
{
    assert(orig != NULL);

    return hashtable_new_real(
            HASHTABLE_INIT_SIZE,
            orig->key_hash,
            orig->key_equal,
            orig->free_key,
            orig->free_value);
}

void hashtable_free(struct hashtable *table)
{
    assert(table != NULL);

    hashtable_clear(table);

    free(table->buckets);
    free(table);
}

void hashtable_insert(struct hashtable *table, void *key, void *value)
{
    assert(table != NULL);
    assert(key != NULL);

    size_t idx = table->key_hash(key) % table->bucket_count;

    if (table->buckets[idx] == NULL) {
        // Bucket empty => new singleton list.
        table->buckets[idx] = list_new_with_data(
            _hashtable_entry_new(key, value));
    } else {
        // Bucket not empty => scan for eventually existing key to replace
        for (struct list *n = table->buckets[idx]; n != NULL; n = n->next) {
            struct hashtable_entry *e = list_data(n, struct hashtable_entry *);

            if (table->key_equal(key, e->key) == 0) {
                // replace!
                if (table->free_value)
                    table->free_value(e->value);

                // Also free the new key, we're using the old one
                if (table->free_key)
                    table->free_key(key);

                e->value = value;
                return; // Return early because the entries count is unchanged
                        // and no rehash is necessary.
            }
        }

        // Not found in list, append!
        list_append(table->buckets[idx], _hashtable_entry_new(key, value));
    }

    table->entries++;

#ifdef AUTOREHASH
    if (hashtable_load_factor(table) > HASHTABLE_MAXLOAD) {
        hashtable_rehash(table);
    }
#endif
}

void _hashtable_remove_internal(struct hashtable *table,
                                const void *key,
                                list_delete_func deleter)
{
    assert(table != NULL);
    assert(key != NULL);

    size_t idx = table->key_hash(key) % table->bucket_count;

    struct list *lst = list_find_custom(
        table->buckets[idx], key, _hashtable_find, table);

    if (!lst)
        return;

    table->buckets[idx] = list_remove_link(
        table->buckets[idx], lst, deleter, table);

    table->entries--;

#ifdef AUTOREHASH
    if (hashtable_load_factor(table) < HASHTABLE_MINLOAD) {
        hashtable_rehash(table);
    }
#endif
}

void hashtable_remove(struct hashtable *table, const void *key)
{
    _hashtable_remove_internal(table, key, _hashtable_free_element);
}

void hashtable_remove_shallow(struct hashtable *table, const void *key)
{
    _hashtable_remove_internal(table, key, _hashtable_free_element_shallow);
}

void _hashtable_clear_internal(struct hashtable *table,
                               list_delete_func deleter)
{
    for (size_t i = 0; i < table->bucket_count; ++i) {
        list_free_all(table->buckets[i], deleter, table);
        table->buckets[i] = NULL;
    }
}

void hashtable_clear(struct hashtable *table)
{
    _hashtable_clear_internal(table, _hashtable_free_element);
}

void hashtable_clear_shallow(struct hashtable *table)
{
    _hashtable_clear_internal(table, _hashtable_free_element_shallow);
}

int _hashtable_find(const void *listdata, const void *key, void *ud)
{
    const struct hashtable_entry *e = listdata;
    struct hashtable *table = ud;

    return table->key_equal(e->key, key);
}

void *hashtable_lookup(const struct hashtable *table, const void *key)
{
    assert(table != NULL);
    assert(key != NULL);

    size_t hash = table->key_hash(key);
    size_t idx = hash % table->bucket_count;

    if (table->buckets[idx] == NULL)
        return NULL;

    struct list *n = list_find_custom(
        table->buckets[idx], key, _hashtable_find, (void *)table);

    if (n != NULL)
        return list_data(n, struct hashtable_entry *)->value;

    return NULL;
}

bool hashtable_contains(const struct hashtable *table, const void *key)
{
    return hashtable_lookup(table, key) != NULL;
}

size_t hashtable_size(const struct hashtable *table)
{
    assert(table != NULL);

    return table->entries;
}

double hashtable_load_factor(const struct hashtable *table)
{
    assert(table != NULL);

    if (table->bucket_count > 0)
        return (double)(table->entries) / (double)(table->bucket_count);

    return 0;
}

void hashtable_rehash(struct hashtable *table)
{
    assert(table != NULL);

    size_t newcount = hashtable_load_factor(table) > HASHTABLE_MAXLOAD
        ? table->bucket_count * 2
        : table->bucket_count / 2;

    struct hashtable *tmp = hashtable_new_real(
        newcount,
        table->key_hash,
        table->key_equal,
        NULL,
        NULL);

    // Move elements to temporary table
    hashtable_union(tmp, table);

    // Free the buckets of the old table
    hashtable_clear_shallow(table);
    free(table->buckets);

    // Move over the new buckets and size
    table->buckets      = tmp->buckets;
    table->bucket_count = tmp->bucket_count;

    // Free the old container.
    free(tmp);
}

struct list *hashtable_keys(const struct hashtable *table)
{
    assert(table != NULL);

    struct list *lst = NULL;

    for (size_t i = 0; i < table->bucket_count; ++i)
        for (struct list *p = table->buckets[i]; p != NULL; p = p->next)
            lst = list_append(lst, list_data(p, struct hashtable_entry *)->key);

    return lst;
}

struct list *hashtable_values(const struct hashtable *table)
{
    assert(table != NULL);

    struct list *lst = NULL;

    for (size_t i = 0; i < table->bucket_count; ++i)
        for (struct list *p = table->buckets[i]; p != NULL; p = p->next)
            lst = list_append(
                lst, list_data(p, struct hashtable_entry *)->value);

    return lst;
}

void hashtable_union(struct hashtable *a, struct hashtable *b)
{
    struct hashtable_iterator iter;

    void *key;
    void *value;

    hashtable_iterator_init(&iter, b);
    while (hashtable_iterator_next(&iter, &key, &value)) {
        hashtable_insert(a, key, value);
    }
}

void hashtable_complement(struct hashtable *a, struct hashtable *b)
{
    struct hashtable_iterator iter;

    void *key;
    void *value;

    hashtable_iterator_init(&iter, b);
    while (hashtable_iterator_next(&iter, &key, &value)) {
        hashtable_remove(a, key);
    }

}

void _hashtable_free_element_shallow(void *elem, void *ud)
{
    free(elem);
}

void _hashtable_free_element(void *elem, void *userdata)
{
    struct hashtable *table = userdata;
    struct hashtable_entry *e = elem;

    if (table->free_key)
        table->free_key(e->key);

    if (table->free_value)
        table->free_value(e->value);

    free(e);
}

void hashtable_iterator_init(struct hashtable_iterator *iter,
                             const struct hashtable *t)
{
    assert(iter != NULL);
    assert(t != NULL);

    memset(iter, 0, sizeof(*iter));

    iter->table = t;
    iter->bucket = 0;
    iter->lst = t->buckets[0];
}

/*
 * Do NOT add or remove entries while iterating. Consequences range from skipped
 * items, doubled items to segmentation faults! Build another hashtable
 * while iterating and use hashtable_union() and hashtable_complement() to merge
 * the changes.
 *
 * Modifications that don't change the size are okay.
 */
bool hashtable_iterator_next(struct hashtable_iterator *iter,
                             void **tkey,
                             void **tval)
{
    assert(iter != NULL);

    while (iter->lst == NULL) {
        if (iter->bucket + 1 < iter->table->bucket_count)
            iter->lst = iter->table->buckets[++iter->bucket];
        else
            return false;
    }

    *tkey = list_data(iter->lst, struct hashtable_entry *)->key;
    *tval = list_data(iter->lst, struct hashtable_entry *)->value;

    iter->lst = iter->lst->next;
    return true;
}

size_t str_hash(const void *k)
{
    size_t hash = 5381;
    char c;

    while ((c = *(const char *)k++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

int str_equal(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

/* Modified DJB algorithm that hashes the characters as lowercase */
size_t ascii_hash(const void *str)
{
    size_t hash = 5381;
    char c;

    while ((c = *(const char *)str++)) {
        hash = ((hash << 5) + hash) + (c | (1 << 5));
    }

    return hash;
}

int ascii_equal(const void *a, const void *b)
{
    const char *sa = a;
    const char *sb = b;

#define ASCIILOWER(a) ((a) | (1 << 5))

    while (*sa && (ASCIILOWER(*sb) == ASCIILOWER(*sa)))
        sa++, sb++;

    return ASCIILOWER(*(const unsigned char *)sa)
        -  ASCIILOWER(*(const unsigned char *)sb);

#undef ASCIILOWER
}

