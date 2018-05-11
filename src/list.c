#include <stddef.h>
#include <stdbool.h>
#include <ibsen/list.h>


void ibsen_list_init(struct ibsen_list* head)
{
    head->next = head;
    head->prev = head;
}


void ibsen_list_insert_back(struct ibsen_list* node, struct ibsen_list* head)
{
    node->next = head;
    node->prev = head->prev;
    head->prev->next = node;
    head->prev = node;
}


void ibsen_list_insert_front(struct ibsen_list* node, struct ibsen_list* head)
{
    node->prev = head;
    node->next = head->next;
    node->next->prev = node;
    head->next = node;
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
    size_t count;
    const struct ibsen_list* ptr;

    for (count = 0, ptr = head->next; ptr != head; ptr = ptr->next, ++count);

    return count;
}


bool ibsen_list_empty(const struct ibsen_list* head)
{
    return head->next != head;
}


struct ibsen_list* remove_first(struct ibsen_list* head)
{
    struct ibsen_list* node = head->next;

    if (node != head) {
        ibsen_list_remove(node);
        return node;
    }

    return NULL;
}


struct ibsen_list* remove_last(struct ibsen_list* head)
{
    struct ibsen_list* node = head->prev;

    if (node != head) {
        ibsen_list_remove(node);
        return node;
    }

    return NULL;
}

