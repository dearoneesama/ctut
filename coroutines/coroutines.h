#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCHEDULER_MIN_ST_SIZE 128*1024
#define SCHEDULER_MAX_ST_SIZE 1024*1024
// default number of coroutines per scheduler before reallocating
#define SCHEDULER_CO_SIZE 128
// coroutine id of the main thread
#define MAIN_CO_ID 0

typedef enum co_status {
    CO_STATUS_NEXIST,    // nonexistent
    CO_STATUS_COMPLETED, // completed
    CO_STATUS_PENDING,   // justed created without resuming, or yielded
    CO_STATUS_RUNNING,   // running
    CO_STATUS_NORMAL     // current coroutine resumes another coroutine
} co_status;

struct scheduler;
typedef struct scheduler scheduler;

// valid function signature to pass as a coroutine
typedef void (*yieldable)(scheduler *sch, void *args);

// identify coroutines
typedef int coroid_t;

// create a scheduler per thread, with context's stack size stsize. 0 means use default
// NULL is returned if this fails
scheduler *scheduler_open(size_t stsize);

// close the scheduler for the thread
void scheduler_close(scheduler *sch);

// negative code is returned if this fails
coroid_t coroutine_new(scheduler *sch, yieldable f, void *args);

// cid: coroutine id to resume
// send: send this value to the point of yielding of the caller, or NULL
// yielded_r: the value yielded from the resumed coroutine will be stored here, or NULL means
// to discard the value
// negative code is returned if this fails
int coroutine_resume(scheduler *sch, coroid_t cid, void *send, void **yielded_r);

// result: the value to be passed to the coroutine that resumed this coroutine, or NULL
// received_r: the value sent from the caller will be stored here, or NULL means to discard it
// negative code is returned if this fails
int coroutine_yield(scheduler *sch, void *result, void **received_r);

coroid_t coroutine_running(scheduler *sch);

co_status coroutine_status(scheduler *sch, coroid_t cid);

#ifdef __cplusplus
}
#endif
