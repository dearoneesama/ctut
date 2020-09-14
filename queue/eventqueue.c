#include <stdlib.h>

#include "deque.h"
#include "eventqueue.h"

#define NEW(TYPE) malloc(sizeof(TYPE))
#define DELETE(VAR) free((VAR))

// entry of the deque
typedef struct QueuedJob {
    EventCallback func;
    void *arg;
} QueuedJob;

// closely related to eventqueue_send_stop()
static QueuedJob queued_stop_sig;

// the deque: push job to left, get job from right

EventQueue *eventqueue_create() {
    EventQueue *evqueue = NEW(EventQueue);
    if (evqueue == NULL)
        return NULL;
    Deque *callbackqueue = deque_create();
    if (callbackqueue == NULL) {
        DELETE(evqueue);
        return NULL;
    }
    evqueue->callbackqueue = callbackqueue;
    evqueue->state = EVQ_STOPPED;
    return evqueue;
}

void eventqueue_close(EventQueue *evqueue) {
    // get rid of all the remaining jobs if there is any
    while (!deque_isempty(evqueue->callbackqueue)) {
        QueuedJob *curr = deque_pop_right(evqueue->callbackqueue);
        DELETE(curr);
    }
    deque_free(evqueue->callbackqueue);
    DELETE(evqueue);
}

size_t eventqueue_npjobs(EventQueue *evqueue) {
    return evqueue->callbackqueue->size;
}

int eventqueue_emplace(EventQueue *evqueue, EventCallback func, void *arg) {
    QueuedJob *newjob = NEW(QueuedJob);
    if (newjob == NULL)
        return 1;
    newjob->func = func;
    newjob->arg = arg;
    deque_push_left(evqueue->callbackqueue, newjob);
    return 0;
}

int eventqueue_emplace_stop(EventQueue *evqueue) {
    deque_push_left(evqueue->callbackqueue, &queued_stop_sig);
    return 0;
}

void eventqueue_runloop(EventQueue *evqueue) {
    if (evqueue->state == EVQ_RUNNING)
        return;
    evqueue->state = EVQ_RUNNING;

    for (;;) {
        if (deque_isempty(evqueue->callbackqueue))
            break;
        QueuedJob *job = deque_pop_right(evqueue->callbackqueue);
        if (job == &queued_stop_sig)
            break;
        EventCallback func = job->func;
        void *arg = job->arg;
        DELETE(job);
        // run func
        func(evqueue, arg);
    }

    // queue is empty or signaled, stop
    evqueue->state = EVQ_STOPPED;
}
