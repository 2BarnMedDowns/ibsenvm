#ifndef __NORAVM_LIST_H__
#define __NORAVM_LIST_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>


/*
 * Linked list entry.
 */
struct noravm_list
{
    struct noravm_list* next;
    struct noravm_list* prev;
};


/*
 * Initialize list head/node.
 */
static inline void noravm_list_init(struct noravm_list* head)
{
    head->next = head;
    head->prev = head;
}


/*
 * Check if the list is empty.
 */
static inline bool noravm_list_empty(const struct noravm_list* head)
{
    return head->next == head;
}


/*
 * Count the number of entries in list.
 */
static inline size_t noravm_list_size(const struct noravm_list* head)
{
    size_t count = 0;
    const struct noravm_list* curr = head->next;

    while (curr != head) {
        ++count;
        curr = curr->next;
    }

    return count;
}


/*
 * Remove entry from list.
 */
static inline void noravm_list_remove_entry(struct noravm_list* entry)
{
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->next = entry;
    entry->prev = entry;
}


/*
 * Insert entry to back of the list.
 */
static inline void noravm_list_push_back(struct noravm_list* head, struct noravm_list* entry)
{
    struct noravm_list* tail = head->prev;
    tail->next = entry;
    entry->prev = tail;
    head->prev = entry;
    entry->next = head;
}


/*
 * Insert entry to front of the list.
 */
static inline void noravm_list_push_front(struct noravm_list* head, struct noravm_list* entry)
{
    struct noravm_list* first = head->next;
    first->prev = entry;
    entry->next = first;
    head->next = entry;
    entry->prev = head;
}


static inline struct noravm_list* noravm_list_front(const struct noravm_list* head)
{
    return head->next != head ? head->next : NULL;
}


static inline struct noravm_list* noravm_list_back(const struct noravm_list* head)
{
    return head->prev != head ? head->prev : NULL;
}


#define __entry(type, ptr) \
    ((struct noravm_list*) (((size_t) (ptr)) + offsetof(type, list)))


#define __node(type, entry) \
    ((type*) (((size_t) entry) - offsetof(type, list)))


#define noravm_list_insert(head, node) \
    noravm_list_push_back(head, __entry(typeof(*node), node))


#define noravm_list_insert_front(head, node) \
    noravm_list_push_front(head, __entry(typeof(*node), node))


#define noravm_list_remove(node) \
    noravm_list_remove_entry(__entry(typeof(*node), node))


#define noravm_list_first(type, head) \
    (noravm_list_front(head) ? __node(type, noravm_list_front(head)) : NULL)


#define noravm_list_last(type, head) \
    (noravm_list_back(head) ? __node(type, noravm_list_back(head)) : NULL)


#define noravm_list_next(head, node) \
    (__entry(typeof(*node), node)->next != (head) ? __node(typeof(*node), __entry(typeof(*node), node)->next) : NULL)

#define noravm_list_prev(head, node) \
    (__entry(typeof(*node), node)->next != (head) ? __node(typeof(*node), __entry(typeof(*node), node)->prev) : NULL)


#define noravm_list_foreach(type, name, head) \
    for (type* __it = (void*) (head)->next, *__next = (type*) ((struct noravm_list*) __it)->next, *name = __node(type, __it); \
            (void*) __it != (void*) (head); \
            __it = __next, __next = (type*) ((struct noravm_list*) __next)->next, name = __node(type, __it))


#ifdef __cplusplus
}
#endif
#endif /* __NORAVM_LIST_H__ */
