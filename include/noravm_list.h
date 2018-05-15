#ifndef __NORAVM_LIST_H__
#define __NORAVM_LIST_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>


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
#define noravm_list_init(head) \
    do { \
        (head)->next = (head); \
        (head)->prev = (head); \
    } while (0) 


/*
 * Check if the list is empty.
 */
#define noravm_list_empty(head) \
    ((head)->next == (head))



/*
 * Remove entry from list.
 */
#define noravm_list_remove_entry(entry) \
    do { \
        (entry)->prev->next = (entry)->next; \
        (entry)->next->prev = (entry)->prev; \
        (entry)->next = (entry); \
        (entry)->prev = (entry); \
    } while (0)


/*
 * Insert entry to back of the list.
 */
#define noravm_list_push_back(head, entry) \
    do { \
        struct noravm_list* tail = (head)->prev; \
        tail->next = (entry); \
        (entry)->prev = tail; \
        (head)->prev = (entry); \
        (entry)->next = (head); \
    } while (0)


/*
 * Insert entry to front of the list.
 */
#define noravm_list_push_front(head, entry) \
    do { \
        struct noravm_list* first = (head)->next; \
        first->prev = (entry); \
        (entry)->next = first; \
        (head)->next = (entry); \
        (entry)->prev = (head); \
    } while (0)


/*
 * Get first element of list.
 */
#define noravm_list_front(head) \
    ( (head)->next != (head) ? (head)->next : NULL )


/*
 * Get last element of list.
 */
#define noravm_list_back(head) \
    ( (head)->prev != (head) ? (head)->prev : NULL )


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
