#ifndef __IBSEN_LIST_H__
#define __IBSEN_LIST_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>


/* Hack borrowed from the Linux kernel */
#define container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) );})


/*
 * Linked list node.
 * Use container of to retrieve element.
 */
struct ibsen_list
{
    struct ibsen_list*  next;
    struct ibsen_list*  prev;
};


void ibsen_list_init(struct ibsen_list* head);


void ibsen_list_insert_back(struct ibsen_list* node, struct ibsen_list* head);


void ibsen_list_insert_front(struct ibsen_list* node, struct ibsen_list* head);


void ibsen_list_remove(struct ibsen_list* node);


size_t ibsen_list_count(const struct ibsen_list* head);


bool ibsen_list_empty(const struct ibsen_list* head);


struct ibsen_list* ibsen_list_remove_first(struct ibsen_list* head);


struct ibsen_list* ibsen_list_remove_last(struct ibsen_list* head);


struct ibsen_list* ibsen_list_peek_first(struct ibsen_list* head);


struct ibsen_list* ibsen_list_peek_last(struct ibsen_list* head);


#define ibsen_list_first(head, type, member) \
    (ibsen_list_peek_first(head) ? container_of(ibsen_list_peek_first(head), type, member) : NULL)

#define ibsen_list_last(head, type, member) \
    (ibsen_list_peek_last(head) ? container_of(ibsen_list_peek_last(head), type, member) : NULL)

#define ibsen_list_for_each(node, head)   \
    for (struct ibsen_list *node = (head)->next, *next = node->next; \
            node != (head); \
            node = next, next = next->next)




#ifdef __cplusplus
}
#endif
#endif /* __IBSEN_LIST_H__ */
