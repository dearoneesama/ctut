#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum co_status {
    CO_STATUS_DEAD,     // completed
    CO_STATUS_PENDING,  // justed created without resuming, or yielded
    CO_STATUS_RUNNING,  // running
    CO_STATUS_NORMAL    // current coroutine resumes another coroutine
} co_status;

struct scheduler;
typedef struct scheduler scheduler;

typedef void (*yieldable)(scheduler *sch, void *args);

typedef int coroid_t;

// create a scheduler per thread, with context's stack size. 0 default
scheduler *scheduler_open(size_t stsize);

// close the scheduler for the thread
void scheduler_close(scheduler *sch);

coroid_t coroutine_new(scheduler *sch, yieldable f, void *args);

int coroutine_resume(scheduler *sch, coroid_t cid);

int coroutine_yield(scheduler *sch);

coroid_t coroutine_running(scheduler *sch);

co_status coroutine_status(scheduler *sch, coroid_t cid);

#ifdef __cplusplus
}
#endif
