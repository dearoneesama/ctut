// a deque implementation
#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// the node in the linked list
// behaviour is undefined if these fields are written
typedef struct DequeNode {
    void *value;
    struct DequeNode *left;
    struct DequeNode *right;
} DequeNode;

// the deque object to operate on
// behaviour is undefined if these fields are written
typedef struct Deque {
    size_t size; // number of values stored in the deque
    DequeNode *leftmost;
    DequeNode *rightmost;
} Deque;

// creates a new deque with size 0; NULL is returned if this fails
Deque *deque_create();

// clears all the value nodes from the deque (llist) and resets size to be 0
// NOT the void* values stored in each node if they are heap objects
// (then why is this function useful? prolly in cases you store words in this struct)
void deque_clear(Deque *deque);

// deallocate the deque itself and its nodes; this function also calls deque_clear()
void deque_free(Deque *deque);

// returns whether deque is empty
bool deque_isempty(Deque *deque);

// add a value to the leftmost position and returns 0; nonzero value is returned
// if this fails
int deque_push_left(Deque *deque, void *value);

// add a value to the rightmost position and returns 0; nonzero value is returned
// if this fails
int deque_push_right(Deque *deque, void *value);

// removes the leftmost value node and returns its contained value
// NULL is returned if deque is empty
void *deque_pop_left(Deque *deque);

// removes the rightmost value node and returns its contained value
// NULL is returned if deque is empty
void *deque_pop_right(Deque *deque);

// returns the contained value of the leftmost node
// NULL is returned if deque is empty
void *deque_left(Deque *deque);

// returns the contained value of the rightmost node
// NULL is returned if deque is empty
void *deque_right(Deque *deque);

#ifdef __cplusplus
}
#endif
