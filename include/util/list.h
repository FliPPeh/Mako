#ifndef _LIST_H_
#define _LIST_H_

struct list
{
    void *data;

    struct list *next;
    struct list *prev;
};

#define list_next(list) ((list)->next)
#define list_data(list, type) ((type)((list)->data))

typedef int (*list_compare_func)(const void *a, const void *b);
typedef void (*list_data_func)(void *data, void *userdata);
typedef void (*list_delete_func)(void *data);
typedef void *(*list_copy_func)(const void *data, void *userdata);


struct list *list_new(void);
struct list *list_new_with_data(void *data);

void list_free(struct list *link, list_delete_func fn);

struct list *list_append(struct list *list, void *data);
struct list *list_prepend(struct list *list, void *data);
struct list *list_insert(struct list *list, void *data, int position);

struct list *list_insert_before(
        struct list *list, struct list *pos, void *data);

struct list *list_insert_sorted(
        struct list *list, void *data, list_compare_func fn);

struct list *list_remove(
        struct list *list, const void *data, list_delete_func fn);

struct list *list_remove_all(
        struct list *list, const void *data, list_delete_func fn);

struct list *list_remove_link(
        struct list *list, struct list *link, list_delete_func fn);

void list_free_all(struct list *list, list_delete_func fn);

size_t list_length(struct list *list);
void list_foreach(struct list *list, list_data_func fn, void *ud);
void list_foreach_reverse(struct list *list, list_data_func fn, void *ud);

struct list *list_copy(struct list *list);
struct list *list_copy_deep(struct list *list, list_copy_func fn, void *ud);

struct list *list_reverse(struct list *list);
struct list *list_sort(struct list *list, list_compare_func fn);

struct list *list_concat(struct list *list1, struct list *list2);

struct list *list_last(struct list *list);
struct list *list_nth(struct list *list, unsigned int n);

struct list *list_find(struct list *list, const void *data);
struct list *list_find_custom(
        struct list *list, const void *data, list_compare_func fn);

int list_position(struct list *list, struct list *link);
int list_index(struct list *list, const void *data);

void *_list_copy_data(const void *data, void *userdata);
int _list_data_equal(const void *a, const void *b);

struct list *_list_link_before(
        struct list *list, struct list *link, struct list *newlink);

struct list *_list_link_after(
        struct list *list, struct list *link, struct list *newlink);

struct list *_list_unlink(struct list *list, struct list *link);

#define LIST_FOREACH(list, ptr) \
    for (ptr = list; ptr != NULL; ptr = ptr->next)

#endif /* defined _LIST_H_ */
