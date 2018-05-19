#ifndef __IBSENVM_LIST_H__
#define __IBSENVM_LIST_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>


/*
 * Linked list entry.
 * The struct member should be named ``list''.
 */
struct ivm_list
{
    struct ivm_list* next;
    struct ivm_list* prev;
};


/*
 * Secret macro magic to get the offset to list entry from a node (offsetof)
 */
#define _ivm_list_entry(type, ptr) \
    ((struct ivm_list*) (((size_t) (ptr)) + offsetof(type, list)))


/*
 * Macro magic to get pointer to the node from list (container_of)
 */
#define _ivm_list_node(type, entry) \
    ((type*) (((size_t) entry) - offsetof(type, list)))



/*
 * Initialize list head/node.
 */
#define ivm_list_init(head) \
    do { \
        (head)->next = (head); \
        (head)->prev = (head); \
    } while (0) 


/*
 * Check if the list is empty.
 */
#define ivm_list_empty(head) \
    ((head)->next == (head))



/*
 * Remove entry from list.
 */
#define ivm_list_remove_entry(entry) \
    do { \
        (entry)->prev->next = (entry)->next; \
        (entry)->next->prev = (entry)->prev; \
        (entry)->next = (entry); \
        (entry)->prev = (entry); \
    } while (0)


/*
 * Insert entry to back of the list.
 */
#define ivm_list_push_back(head, entry) \
    do { \
        struct ivm_list* tail = (head)->prev; \
        tail->next = (entry); \
        (entry)->prev = tail; \
        (head)->prev = (entry); \
        (entry)->next = (head); \
    } while (0)


/*
 * Insert entry to front of the list.
 */
#define ivm_list_push_front(head, entry) \
    do { \
        struct ivm_list* first = (head)->next; \
        first->prev = (entry); \
        (entry)->next = first; \
        (head)->next = (entry); \
        (entry)->prev = (head); \
    } while (0)


/*
 * Get first entry in list.
 */
#define ivm_list_front(head) \
    ( (head)->next != (head) ? (head)->next : NULL )


/*
 * Get last entry in list.
 */
#define ivm_list_back(head) \
    ( (head)->prev != (head) ? (head)->prev : NULL )


/*
 * Insert element at end of list.
 */
#define ivm_list_insert(head, node) \
    ivm_list_push_back(head, _ivm_list_entry(typeof(*node), node))


/*
 * Insert element at front of list.
 */
#define ivm_list_insert_front(head, node) \
    ivm_list_push_front(head, _ivm_list_entry(typeof(*node), node))


/*
 * Remove element from list.
 */
#define ivm_list_remove(node) \
    ivm_list_remove_entry(_ivm_list_entry(typeof(*node), node))


/*
 * Get first element in list.
 */
#define ivm_list_first(type, head) \
    (ivm_list_front(head) ? _ivm_list_node(type, ivm_list_front(head)) : NULL)


/*
 * Get last element in list.
 */
#define ivm_list_last(type, head) \
    (ivm_list_back(head) ? _ivm_list_node(type, ivm_list_back(head)) : NULL)


/*
 * Get next element in list.
 */
#define ivm_list_next(head, node) \
    (_ivm_list_entry(typeof(*node), node)->next != (head) ? _ivm_list_node(typeof(*node), _ivm_list_entry(typeof(*node), node)->next) : NULL)


/*
 * Get previous element in list.
 */
#define ivm_list_prev(head, node) \
    (_ivm_list_entry(typeof(*node), node)->next != (head) ? _ivm_list_node(typeof(*node), _ivm_list_entry(typeof(*node), node)->prev) : NULL)


/*
 * Iterate list.
 */
#define ivm_list_foreach(type, name, head) \
    for (type* __it = (void*) (head)->next, *__next = (type*) ((struct ivm_list*) __it)->next, *name = _ivm_list_node(type, __it); \
            (void*) __it != (void*) (head); \
            __it = __next, __next = (type*) ((struct ivm_list*) __next)->next, name = _ivm_list_node(type, __it))


#ifdef __cplusplus
}
#endif
#endif /* __NORAVM_LIST_H__ */
