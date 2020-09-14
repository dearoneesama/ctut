#include <stdlib.h>

#include "deque.h"

#define NEW(TYPE) malloc(sizeof(TYPE))
#define DELETE(VAR) free((VAR))

// returns NULL if allocation fails
static DequeNode *get_node(void *value) {
    DequeNode *node = NEW(DequeNode);
    if (node == NULL)
        return NULL;
    node->value = value;
    node->left = node->right = NULL;
    return node;
}

Deque *deque_create() {
    Deque *deque = NEW(Deque);
    if (deque == NULL)
        return NULL;
    deque->size = 0;
    deque->leftmost = deque->rightmost = NULL;
    return deque;
}

void deque_clear(Deque *deque) {
    DequeNode *curr = deque->leftmost;
    while (curr != NULL) {
        DequeNode *tmp = curr;
        curr = curr->right;
        DELETE(tmp);
    }
    deque->size = 0;
    deque->leftmost = deque->rightmost = NULL;
}

void deque_free(Deque *deque) {
    deque_clear(deque);
    DELETE(deque);
}

bool deque_isempty(Deque *deque) {
    return deque->size == (size_t)0;
}

int deque_push_left(Deque *deque, void *value) {
    DequeNode *newnode = get_node(value);
    if (newnode == NULL)
        return 1;

    if (deque_isempty(deque)) {
        deque->leftmost = deque->rightmost = newnode;
    } else {
        newnode->right = deque->leftmost;
        deque->leftmost->left = newnode;
        deque->leftmost = newnode;
    }
    ++deque->size;

    return 0;
}

int deque_push_right(Deque *deque, void *value) {
    DequeNode *newnode = get_node(value);
    if (newnode == NULL)
        return 1;

    if (deque_isempty(deque)) {
        deque->rightmost = deque->leftmost = newnode;
    } else {
        newnode->left = deque->rightmost;
        deque->rightmost->right = newnode;
        deque->rightmost = newnode;
    }
    ++deque->size;

    return 0;
}

void *deque_pop_left(Deque *deque) {
    if (deque_isempty(deque))
        return NULL;

    DequeNode *curr = deque->leftmost;
    void *value = curr->value;
    deque->leftmost = curr->right;

    if (deque->leftmost == NULL) // curr was the only node
        deque->rightmost = NULL;
    else
        deque->leftmost->left = NULL;

    DELETE(curr);
    --deque->size;

    return value;
}

void *deque_pop_right(Deque *deque) {
    if (deque_isempty(deque))
        return NULL;

    DequeNode *curr = deque->rightmost;
    void *value = curr->value;
    deque->rightmost = curr->left;

    if (deque->rightmost == NULL) // curr was the only node
        deque->leftmost = NULL;
    else
        deque->rightmost->right = NULL;

    DELETE(curr);
    --deque->size;

    return value;
}

void *deque_left(Deque *deque) {
    return deque_isempty(deque) ? NULL : deque->leftmost->value;
}

void *deque_right(Deque *deque) {
    return deque_isempty(deque) ? NULL : deque->rightmost->value;
}
