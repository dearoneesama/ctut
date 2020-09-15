#include <stdlib.h>

#include "deque.h"
#include "eventqueue.h"

#define NEW(TYPE) (TYPE *)malloc(sizeof(TYPE))
#define DELETE(VAR) free((VAR))

#ifdef EVQ_USE_THREADSAFE
    #define SHOULD_LOCK(PLOCK) pthread_mutex_lock(PLOCK)
    #define SHOULD_UNLOCK(PLOCK) pthread_mutex_unlock(PLOCK)
#else
    #define SHOULD_LOCK(PLOCK)
    #define SHOULD_UNLOCK(PLOCK)
#endif

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
#ifdef EVQ_USE_THREADSAFE
    if (pthread_mutex_init(&evqueue->callbackqueue_mtx, NULL) != 0) {
        deque_free(evqueue->callbackqueue);
        DELETE(evqueue);
        return NULL;
    }
#endif
    return evqueue;
}

void eventqueue_close(EventQueue *evqueue) {
    // get rid of all the remaining jobs if there is any
    while (!deque_isempty(evqueue->callbackqueue)) {
        QueuedJob *curr = (QueuedJob *)deque_pop_right(evqueue->callbackqueue);
        DELETE(curr);
    }
    deque_free(evqueue->callbackqueue);
#ifdef EVQ_USE_THREADSAFE
    pthread_mutex_destroy(&evqueue->callbackqueue_mtx);
#endif
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
    SHOULD_LOCK(&evqueue->callbackqueue_mtx);
    deque_push_left(evqueue->callbackqueue, newjob);
    SHOULD_UNLOCK(&evqueue->callbackqueue_mtx);
    return 0;
}

int eventqueue_emplace_stop(EventQueue *evqueue) {
    SHOULD_LOCK(&evqueue->callbackqueue_mtx);
    deque_push_left(evqueue->callbackqueue, &queued_stop_sig);
    SHOULD_UNLOCK(&evqueue->callbackqueue_mtx);
    return 0;
}

void eventqueue_this_thread_run(EventQueue *evqueue) {
    if (evqueue->state == EVQ_RUNNING)
        return;
    evqueue->state = EVQ_RUNNING;

    for (;;) {
        SHOULD_LOCK(&evqueue->callbackqueue_mtx);
        if (deque_isempty(evqueue->callbackqueue)) {
            SHOULD_UNLOCK(&evqueue->callbackqueue_mtx);
            break;
        }
        QueuedJob *job = (QueuedJob *)deque_pop_right(evqueue->callbackqueue);
        SHOULD_UNLOCK(&evqueue->callbackqueue_mtx);
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
