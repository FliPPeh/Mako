#include "list.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void list_free_wrapper(void *data, void *ud)
{
    free(data);
}

struct list *list_new()
{
    struct list *list = malloc(sizeof(*list));

    memset(list, 0, sizeof(*list));

    return list;
}

struct list *list_new_with_data(void *data)
{
    struct list *list = list_new();

    list->data = data;
    return list;
}

void list_free(struct list *link, list_delete_func fn, void *ud)
{
    if (fn)
        fn(link->data, ud);

    free(link);
}

struct list *list_append(struct list *list, void *data)
{
    return list_insert(list, data, -1);
}

struct list *list_prepend(struct list *list, void *data)
{
    return list_insert(list, data, 0);
}

struct list *list_insert(struct list *list, void *data, int position)
{
    if (!list) {
        return list_new_with_data(data);
    } else {
        struct list *newitem = list_new_with_data(data);

        if (!position) {
            return _list_link_before(list, list, newitem);
        } else {
            struct list *ptr = list;

            while (--position && ptr->next)
                ptr = ptr->next;

            return _list_link_after(list, ptr, newitem);
        }
    }
}

struct list *list_insert_before(struct list *list,
                                struct list *pos,
                                void *data)
{
    struct list *newitem = list_new_with_data(data);

    return _list_link_before(list, pos, newitem);
}

struct list *list_insert_sorted(struct list *list,
                                void *data,
                                list_compare_func fn,
                                void *userdata)
{
    if (!list) {
        list = list_new_with_data(data);
    } else {
        struct list *newitem = list_new_with_data(data);

        newitem->data = data;

        for (struct list *ptr = list; ptr != NULL; ptr = ptr->next) {
            if (fn(ptr->data, data, userdata) < 0)
                return _list_link_before(list, ptr, newitem);

            if (ptr->next == NULL)
                return _list_link_after(list, ptr, newitem);
        }
    }

    return list;
}

struct list *list_remove(struct list *list,
                         const void *data,
                         list_delete_func fn,
                         void *ud)
{
    for (struct list *ptr = list; ptr != NULL; ptr = ptr->next) {
        if (ptr->data == data) {
            struct list *newlist = _list_unlink(list, ptr);

            list_free(ptr, fn, ud);

            return newlist;
        }
    }

    return list;
}

struct list *list_remove_all(struct list *list,
                             const void *data,
                             list_delete_func fn,
                             void *ud)
{
    struct list *ptr = list;

    while (ptr) {
        if (ptr->data == data) {
            struct list *next = ptr->next;

            list = _list_unlink(list, ptr);
            list_free(ptr, fn, ud);

            ptr = next;

            continue;
        }

        ptr = ptr->next;
    }

    return list;
}

struct list *list_remove_link(struct list *list,
                              struct list *link,
                              list_delete_func fn,
                              void *ud)
{
    for (struct list *ptr = list; ptr != NULL; ptr = ptr->next) {
        if (ptr == link) {
            struct list *newlist = _list_unlink(list, ptr);

            list_free(ptr, fn, ud);

            return newlist;
        }
    }

    return list;
}

void list_free_all(struct list *list, list_delete_func fn, void *ud)
{
    struct list *ptr = list;

    while (ptr != NULL) {
        struct list *next = ptr->next;

        list_free(ptr, fn, ud);

        ptr = next;
    }
}

size_t list_length(struct list *list)
{
    size_t n = 0;

    if (!list)
        return 0;

    while ((list = list->next))
        n++;

    return n;
}

void list_foreach(struct list *list, list_data_func fn, void *ud)
{
    for (struct list *ptr = list; ptr != NULL; ptr = ptr->next)
        fn(ptr->data, ud);
}

void list_foreach_reverse(struct list *list, list_data_func fn, void *ud)
{
    struct list *ptr = NULL;

    for (struct list *ptr = list; ptr->next != NULL; ptr = ptr->next);

    for (; ptr != NULL; ptr = ptr->prev)
        fn(ptr->data, ud);
}

struct list *list_copy(struct list *list)
{
    return list_copy_deep(list, _list_copy_data, NULL);
}

struct list *list_copy_deep(struct list *list, list_copy_func fn, void *ud)
{
    struct list *copy = NULL;

    for (struct list *ptr = list; ptr != NULL; ptr = ptr->next)
        list_prepend(copy, fn(ptr->data, ud));

    return list_reverse(copy);
}

struct list *list_reverse(struct list *list)
{
    struct list *ptr = list;
    struct list *next = NULL;
    struct list *prev = NULL;

    while (ptr) {
        next = ptr->next;
        ptr->next = prev;
        prev = ptr;
        ptr = next;
    }

    return prev;
}

struct list *list_sort(struct list *list, list_compare_func fn, void *userdata)
{
    struct list *sorted = NULL;

    for (struct list *ptr = list; ptr != NULL; ptr = ptr->next)
        sorted = list_insert_sorted(sorted, ptr->data, fn, userdata);

    return sorted;
}

struct list *list_concat(struct list *list1, struct list *list2)
{
    struct list *ptr = NULL;

    if (!list1)
        return list2;

    for (ptr = list1; ptr->next != NULL; ptr = ptr->next);

    ptr->next = list2;
    list2->prev = ptr;

    return list1;
}

struct list *list_last(struct list *list)
{
    struct list *ptr = NULL;

    if (!list)
        return NULL;

    for (ptr = list; ptr->next != NULL; ptr = ptr->next);

    return ptr;
}

struct list *list_nth(struct list *list, unsigned int n)
{
    struct list *ptr = NULL;

    for (ptr = list; ptr != NULL && n--; ptr = ptr->next);

    return ptr;
}

struct list *list_find(struct list *list, const void *data)
{
    return list_find_custom(list, data, _list_data_equal, NULL);
}

struct list *list_find_custom(
        struct list *list,
        const void *data,
        list_compare_func fn,
        void *userdata)
{
    for (struct list *ptr = list; ptr != NULL; ptr = ptr->next)
        if (!fn(ptr->data, data, userdata))
            return ptr;

    return NULL;
}

int list_position(struct list *list, struct list *link)
{
    int pos = 0;

    for (struct list *ptr = list; ptr != NULL; ptr = ptr->next, pos++)
        if (ptr == link)
            return pos;

    return -1;
}

int list_index(struct list *list, const void *data)
{
    int pos = 0;

    for (struct list *ptr = list; ptr != NULL; ptr = ptr->next, pos++)
        if (ptr->data == data)
            return pos;

    return -1;
}

void *_list_copy_data(const void *data, void *userdata)
{
    return (void *)data;
}

int _list_data_equal(const void *a, const void *b, void *ud)
{
    return !(a == b);
}

struct list *_list_link_before(struct list *list,
                               struct list *link,
                               struct list *newlink)
{
    if (!list)
        return newlink;

    newlink->prev = link->prev;
    newlink->next = link;

    if (link->prev)
        link->prev->next = newlink;

    link->prev = newlink;

    return (link == list) ? newlink : list;
}

struct list *_list_link_after(struct list *list,
                              struct list *link,
                              struct list *newlink)
{
    if (!list)
        return newlink;

    newlink->prev = link;
    newlink->next = link->next;

    if (link->next)
        link->next->prev = newlink;

    link->next = newlink;

    return list;
}

struct list *_list_unlink(struct list *list, struct list *link)
{
    struct list *next = link->next;

    if (link->prev) {
        link->prev->next = link->next;
    }

    if (link->next) {
        link->next->prev = link->prev;
    }

    return (link == list) ? next : list;
}
