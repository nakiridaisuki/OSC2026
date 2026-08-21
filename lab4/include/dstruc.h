#ifndef _DSTRUC_H_
#define _DSTRUC_H_

#include <stddef.h>
#include <stdio.h>

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

typedef struct {
    char *buf;
    size_t buf_size;
    size_t head;
    size_t tail;
} RingBuffer;

inline static void ring_buf_init(char *buf, size_t size, RingBuffer *rb) {
    rb->buf      = buf;
    rb->buf_size = size;
    rb->head = rb->tail = 0;
}
inline static void ring_buf_push(char c, RingBuffer *rb) {
    size_t next = (rb->head + 1) % rb->buf_size;
    if (next != rb->tail) {
        rb->buf[rb->head] = c;
        rb->head          = next;
    }
}
inline static int ring_buf_pop(char *c, RingBuffer *rb) {
    if (rb->head == rb->tail)
        return 0;
    *c       = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) % rb->buf_size;
    return 1;
}
inline static int ring_buf_full(RingBuffer *rb) {
    return ((rb->head + 1) % rb->buf_size) == rb->tail;
}
inline static int ring_buf_empty(RingBuffer *rb) { return rb->head == rb->tail; }

#endif /* ifndef _DSTRUC_H_ */
