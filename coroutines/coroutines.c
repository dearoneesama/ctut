#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include "coroutines.h"

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

struct coroutine;
typedef struct coroutine coroutine;

struct scheduler {
    coroid_t running; // id of running coroutine
    size_t stsize;    // stack size
    size_t nco;       // number of coroutines
    size_t cap;       // max number of coroutines
    coroutine **co;   // array of coroutines, the index is coroutine id (malloc)
    void *yd;         // yield x. yielded result from callee to caller
    void *sd;         // sent result from the caller to callee
};

struct coroutine {
    char *stack;      // stack (malloc)
    scheduler *sch;   // scheduler that runs this
    yieldable func;   // function
    void *args;       // userdata
    ucontext_t ctx;   // context of coroutine
    co_status status; // status
    coroid_t prevco;  // previous coroutine id that resumes the current coroutine
};

scheduler *scheduler_open(size_t stsize) {
    scheduler *sch = malloc(sizeof(scheduler));
    if (sch == NULL)
        return NULL;
    sch->stsize = MIN(SCHEDULER_MAX_ST_SIZE, MAX(stsize, SCHEDULER_MIN_ST_SIZE));
    sch->nco = 0;
    sch->cap = SCHEDULER_CO_SIZE;
    sch->co = calloc(sch->cap, sizeof(coroutine *));
    if (sch->co == NULL) {
        free(sch);
        return NULL;
    }
    sch->yd = NULL;
    sch->sd = NULL;

    // spawn main coroutine
    coroid_t mid = coroutine_new(sch, NULL, NULL);
    if (mid != MAIN_CO_ID)
        errx(1, "invalid scheduler creation");
    // set main coroutine to running
    sch->co[mid]->status = CO_STATUS_RUNNING;
    sch->running = mid;
    return sch;
}

void scheduler_close(scheduler *sch) {
    if (sch->running != MAIN_CO_ID)
        errx(1, "invalid scheduler close");

    for (int i = 0; i < sch->cap; ++i) {
        coroutine *co = sch->co[i];
        if (co != NULL) {
            free(co->stack);
            free(co);
        }
    }
    free(sch->co);
    free(sch);
}

static void cofunc(scheduler *sch) {
    coroutine *co = sch->co[sch->running];
    // run coroutine
    co->func(sch, co->args);
    // coroutine finished
    co->status = CO_STATUS_COMPLETED;
    --sch->nco;
    // restore precious coroutine
    coroutine *prevco = sch->co[co->prevco];
    prevco->status = CO_STATUS_RUNNING;
    sch->running = co->prevco;
    ucontext_t _;
    swapcontext(&_, &prevco->ctx);
}

coroid_t coroutine_new(scheduler *sch, yieldable f, void *args) {
    coroid_t id = -1;
    if (sch->nco >= sch->cap) {
        // max size reached, double
        coroutine **newco = realloc(sch->co, sch->cap * 2 * sizeof(coroutine *));
        if (newco == NULL)
            return -1;
        sch->co = newco;
        memset(sch->co + sch->cap, 0, sch->cap * sizeof(coroutine *));
        sch->cap *= 2;
        id = sch->cap;
    } else {
        // find empty slot
        for (coroid_t i = 0; i < sch->cap; ++i) {
            if (sch->co[i] == NULL ||
                sch->co[i]->status == CO_STATUS_COMPLETED) {
                id = i;
                break;
            }
        }
    }
    if (id < 0)
        return id;

    // create? coroutine
    coroutine *co;
    if (sch->co[id]) {
        // reuse dead coroutine
        co = sch->co[id];
    } else {
        // malloc new coroutine
        co = malloc(sizeof(coroutine));
        if (co == NULL)
            return -1;
        if (id == MAIN_CO_ID) {
            co->stack = NULL;
        } else {
            co->stack = malloc(sch->stsize);
            if (co->stack == NULL) {
                free(co);
                return -1;
            }
        }
        co->prevco = MAIN_CO_ID;
        sch->co[id] = co;
    }

    ++sch->nco;

    co->func = f;
    co->args = args;
    co->sch = sch;
    co->status = CO_STATUS_PENDING;

    if (f != NULL) {
        coroutine *curco = sch->co[sch->running];
        if (curco == NULL)
            errx(1, "scheduler not valid");
        getcontext(&co->ctx);
        co->ctx.uc_stack.ss_sp = co->stack;
        co->ctx.uc_stack.ss_size = sch->stsize;
        co->ctx.uc_link = &curco->ctx;
        makecontext(&co->ctx, (void(*)(void))cofunc, 1, sch);
    }

    return id;
}

int coroutine_resume(scheduler *sch, coroid_t cid, void *send, void **yielded_r) {
    if (cid < 0 || cid > sch->cap)
        errx(1, "bad coroutine id");
    coroutine *co = sch->co[cid],
              *curco = sch->co[sch->running];
    if (co == NULL || curco == NULL)
        return -1;
    switch (co->status) {
        case CO_STATUS_PENDING:
            curco->status = CO_STATUS_NORMAL;
            co->prevco = sch->running;
            co->status = CO_STATUS_RUNNING;
            sch->running = cid;

            // send something to callee
            sch->sd = send;
            swapcontext(&curco->ctx, &co->ctx);
            // obtain the yielded xxx from switched-out coroutine
            if (yielded_r != NULL)
                *yielded_r = sch->yd;
            sch->yd = NULL;

            return 0;
        default:
            return -1;
    }
}

int coroutine_yield(scheduler *sch, void *result, void **received_r) {
    coroid_t id = sch->running;
    // main coroutine cannot yield
    if (id == MAIN_CO_ID)
        return -1;
    if (id < 0)
        errx(1, "bad coroutine id");

    coroutine *co = sch->co[id],
              *prevco = sch->co[co->prevco];
    co->status = CO_STATUS_PENDING;
    prevco->status = CO_STATUS_RUNNING;
    sch->running = co->prevco;

    // yield something to caller
    sch->yd = result;
    swapcontext(&co->ctx, &prevco->ctx);
    // obtain the sent result from the caller coroutine
    if (received_r != NULL)
        *received_r = sch->sd;
    sch->sd = NULL;

    return 0;
}

coroid_t coroutine_running(scheduler *sch) {
    return sch->running;
}

co_status coroutine_status(scheduler *sch, coroid_t cid) {
    if (cid < 0 || cid > sch->cap)
        errx(1, "bad coroutine id");
    if (sch->co[cid] == NULL)
        return CO_STATUS_NEXIST;
    return sch->co[cid]->status;
}
