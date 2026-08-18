#ifndef _DSTRUC_H_
#define _DSTRUC_H_

#include <stddef.h>

#define container_of(nodeptr, type, member) ((type *)((char *)nodeptr - offsetof(type, member)))

typedef struct _linked_list_node LinkedListNode;
struct _linked_list_node {
    LinkedListNode *prev, *next;
};

inline static int lln_init(LinkedListNode *node) {
    if (node == NULL)
        return 1;
    node->prev = node->next = node;
    return 0;
}
inline static int lln_add(LinkedListNode *head, LinkedListNode *node) {
    if (node == NULL || head == NULL)
        return 1;
    node->next       = head->next;
    node->prev       = head;
    head->next->prev = node;
    head->next       = node;
    return 0;
}
inline static int lln_remove(LinkedListNode *node) {
    if (node == NULL)
        return 1;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node->next = node;
    return 0;
}

#endif /* ifndef _DSTRUC_H_ */
