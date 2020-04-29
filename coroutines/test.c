#include <stdio.h>
#include <stdlib.h>

#include "coroutines.h"

static void send(scheduler *sch, void *args) {
    (void)args;
    for (;;) {
        char *buff = malloc(256);
        if (fgets(buff, 256, stdin) == NULL || buff[0] == '\n') {
            free(buff);
            break;
        }
        // toss the ownership of the string to client
        coroutine_yield(sch, buff, NULL);
    }
}

struct receive_args {
    coroid_t sender;
};
static void receive(scheduler *sch, void *args) {
    coroid_t sid = ((struct receive_args *)args)->sender;
    char *recvd;
    const char * const myname;
    coroutine_yield(sch, NULL, (void**)&myname);
    while (coroutine_resume(sch, sid, NULL, (void**)&recvd) >= 0
        && coroutine_status(sch, sid) != CO_STATUS_COMPLETED) {
        printf("%s, your input is: %s\n", myname, recvd);
        free(recvd);
    }
    printf("bye!\n");
}

int main(void) {
    scheduler *s = scheduler_open(0);
    struct receive_args rc_a = { coroutine_new(s, send, NULL) };
    coroid_t rc = coroutine_new(s, receive, &rc_a);
    // kick out the receive coroutine
    coroutine_resume(s, rc, NULL, NULL);
    // resume line 26
    coroutine_resume(s, rc, "oneesama", NULL);
    scheduler_close(s);
}
