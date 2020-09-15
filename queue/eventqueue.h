// an "event loop"-like stuff implementation
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// if this macro is defined, the callbackqueue for any event queue will be
// wrapped by a mutex

#ifdef EVQ_USE_THREADSAFE
    #include <pthread.h>
#endif

// deque.h
struct Deque;
typedef struct Deque Deque;

typedef enum EventQueueState {
    EVQ_STOPPED,
    EVQ_RUNNING
} EventQueueState;

// the event loop object
// behaviour is undefined if these fields are written
typedef struct EventQueue {
    EventQueueState state;
    Deque *callbackqueue;
#ifdef EVQ_USE_THREADSAFE
    pthread_mutex_t callbackqueue_mtx;
#endif
} EventQueue;

// the functions inside the event queue -> run one by one
typedef void (*EventCallback)(EventQueue *evqueue, void *arg);

// creates a new event queue in stopped state
EventQueue *eventqueue_create();

// frees the resources occupied by the event queue
// if it contains any heap userdata (void *args) unused, they are NOT freed
// the queue has to be stopped, otherwise behaviour is undefined
void eventqueue_close(EventQueue *evqueue);

// returns the number of pending jobs in the queue
size_t eventqueue_npjobs(EventQueue *evqueue);

// add a function call with these arguments to the back of the calling queue;
// nonzero value is returned if this fails
// the enqueued function is responsible for the lifetime of the *arg argument
// if it is a resource. if that function is never called due to the stopping or
// deallocation of the queue, the resource is leaked
int eventqueue_emplace(EventQueue *evqueue, EventCallback func, void *arg);

// add a stop signal to the back of the calling queue; the queue stops executing
// when it reaches this signal and sets its state to stopped
int eventqueue_emplace_stop(EventQueue *evqueue);

// starts the queue and run the functions inside until it is exhausted or stopped
// blocks this thread. has no effect if the queue is already running
// (so you can call this to ensure the loop is running after inserting a job)
// AT MOST one thread shall be running this function
void eventqueue_this_thread_run(EventQueue *evqueue);

#ifdef __cplusplus
}
#endif
