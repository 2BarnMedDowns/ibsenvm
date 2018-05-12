#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <ibsen/list.h>


void ibsen_list_init(struct ibsen_list* head)
{
    head->next = head;
    head->prev = head;
}


void ibsen_list_insert_back(struct ibsen_list* node, struct ibsen_list* head)
{
    struct ibsen_list* tail = head->prev;
    tail->next = node;
    node->prev = tail;
    head->prev = node;
    node->next = head;
}


void ibsen_list_insert_front(struct ibsen_list* node, struct ibsen_list* head)
{
    struct ibsen_list* hhead = head->next;
    hhead->prev = node;
    node->next = hhead;
    head->next = node;
    node->prev = head;
}


void ibsen_list_remove(struct ibsen_list* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = node;
    node->prev = node;
}


size_t ibsen_list_count(const struct ibsen_list* head)
{
    size_t count = 0;
    const struct ibsen_list* ptr = head;

    for (; ptr->next != head; ptr = ptr->next, ++count);

    return count;
}


bool ibsen_list_empty(const struct ibsen_list* head)
{
    return head->next != head;
}


struct ibsen_list* ibsen_list_peek_first(struct ibsen_list* head)
{
    struct ibsen_list* node = head->next;

    if (node != head) {
        return node;
    }

    return NULL;
}


struct ibsen_list* ibsen_list_peek_last(struct ibsen_list* head)
{
    struct ibsen_list* node = head->prev;
    
    if (node != head) {
        return node;
    }

    return NULL;
}


struct ibsen_list* ibsen_list_remove_first(struct ibsen_list* head)
{
    struct ibsen_list* node = head->next;

    if (node != head) {
        ibsen_list_remove(node);
        return node;
    }

    return NULL;
}


struct ibsen_list* ibsen_list_remove_last(struct ibsen_list* head)
{
    struct ibsen_list* node = head->prev;

    if (node != head) {
        ibsen_list_remove(node);
        return node;
    }

    return NULL;
}

