#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "eventqueue.h"

#define CAST_LONG(X) (void *)(long)(X)

static sig_atomic_t stop = 0;
static void handle_interrupt(int sig) { (void)sig; stop = 1; }

static void counter1(EventQueue *evq, void *arg) {
    long num = (long)arg;
    printf("+ This is counter 1: %ld\n", num);
    sleep(1);
    // recursively insert itself in to the queue
    if (!stop)
        eventqueue_emplace(evq, counter1, CAST_LONG(num + 1));
}

static void counter2(EventQueue *evq, void *arg) {
    long num = (long)arg;
    printf("* This is counter 2: %ld\n", num);
    sleep(1);
    // recursively insert itself in to the queue
    if (!stop)
        eventqueue_emplace(evq, counter2, CAST_LONG(num * 2));
}

int main() {
    signal(SIGINT, handle_interrupt);
    EventQueue *eq = eventqueue_create();
    eventqueue_emplace(eq, counter1, CAST_LONG(1));
    eventqueue_emplace(eq, counter2, CAST_LONG(1));
    // eventqueue_emplace_stop(eq); will only print two lines if this is present
    // prints two counters interactively
    // press ctrl+C to stop
    printf("loop starts running\n");
    // will run counter1 and counter2 alternatively
    eventqueue_this_thread_run(eq);
    printf("loop finishes running\n");
    eventqueue_close(eq);
}
