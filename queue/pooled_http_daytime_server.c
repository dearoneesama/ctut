// threaded day time server
// open browser and open localhost:<your port>
// to see current day time

#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define EVQ_USE_THREADSAFE
#include "eventqueue.h"

#define ERRSTR strerror(errno)

#define MAX_WORKERS 4
#define LISTENQ 1024
#define MAX_BUFF_SIZE 512


// one main thread for delegating work to each MAX_WORKERS work thread.
// each worker has its own function queue to query jobs
// except the main thread, each worker only accesses to one item of the array
static struct ThreadEquipment {
    int id;
    EventQueue *cbqueue;
    pthread_t thread;
} thread_equipments[MAX_WORKERS];

// argument passed to each callback due to a connection accept
struct ThreadConnArg {
    struct ThreadEquipment *eqp;
    int conn_fd;
    struct sockaddr_in client;
    socklen_t client_len;
};

static void *wrap_evqueue_start(void *arg) {
    eventqueue_this_thread_run((EventQueue *)arg);
    return NULL;
}

// pretend processing takes time
/*static void dosleep() {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = (long)1e9;
    nanosleep(&ts, NULL);
}*/

// prepare and send daytime response
static void do_daytime(struct ThreadConnArg *cona) {
    char buf[MAX_BUFF_SIZE];
    char *body = buf;
    char *header = buf + (MAX_BUFF_SIZE/2)*(sizeof(char));
    time_t tick = time(NULL);

    snprintf(body, MAX_BUFF_SIZE, 
        "Hi!\r\nCurrent server time: %.24s\r\nThis is sent from worker %d",
        ctime(&tick), cona->eqp->id
    );
    size_t bodylen = strlen(body);
    snprintf(header, MAX_BUFF_SIZE / 2,
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=UTF-8\r\nContent-Length: %ld\r\nConnection: Closed\r\n\r\n",
        bodylen
    );
    size_t headerlen = strlen(header);

    if (write(cona->conn_fd, header, headerlen) != headerlen ||
        write(cona->conn_fd, body, bodylen) != bodylen)
        fprintf(stderr, "tid %d: cannot write response: %s\n", cona->eqp->id, ERRSTR);

    close(cona->conn_fd);
}

static void do_others(struct ThreadConnArg *cona) {
    const char *header = "HTTP/1.1 404 Not Found\r\nConnection: Closed\r\n\r\n";
    const size_t header_len = 46;

    if (write(cona->conn_fd, header, header_len) != header_len)
        fprintf(stderr, "tid %d: cannot write response: %s\n", cona->eqp->id, ERRSTR);

    close(cona->conn_fd);
}

static void handle_request(EventQueue *evq, void *arg) {
    (void)evq;
    struct ThreadConnArg *cona = (struct ThreadConnArg *)arg;

    // read first 6 bytes, which is the min number to determine the request is GET and to root
    char headerbuf[6];
    if (read(cona->conn_fd, headerbuf, 6) > 0 
        && memcmp((void *)"GET / ", (void *)headerbuf, 6) == 0)
        do_daytime(cona);
    else
        do_others(cona);

    printf("tid: %d: processed connection from %s\n", cona->eqp->id, inet_ntoa(cona->client.sin_addr));
    free(cona);
}

static void usage(char *fn) {
    fprintf(stderr, "usage: %s <port>\n", fn);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc != 2)
        usage(argv[0]);

    int port = atoi(argv[1]);
    if (port <= 0)
        errx(1, "atoi failed");

    // if client suddenly closes connection; ignore the signal
    signal(SIGPIPE, SIG_IGN);

    // init some fields
    for (size_t i = 0; i < MAX_WORKERS; ++i) {
        thread_equipments[i].id = i;
        if ((thread_equipments[i].cbqueue = eventqueue_create()) == NULL)
            errx(1, "cannot create evqueue");
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
        err(1, "cannot create socket");

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof server_addr) == -1)
        err(1, "cannot bind");

    if (listen(listen_fd, LISTENQ) == -1)
        err(1, "cannot listen");

    printf("listening on port %d!\n", port);

    for (;;) {
        int free_id = -1;
        int should_rerun_thread = false;
        // find a queue that is not running
        for (size_t i = 0; i < MAX_WORKERS; ++i) {
            if (thread_equipments[i].cbqueue->state == EVQ_STOPPED) {
                free_id = i;
                should_rerun_thread = true;
                break;
            }
        }
        // if all threads are running, find the one with the smallest load
        // since there are multiple threads, this might not be most accurate
        if (!should_rerun_thread) {
            size_t currmin = (size_t)-1;
            for (size_t i = 0; i < MAX_WORKERS; ++i) {
                if (eventqueue_npjobs(thread_equipments[i].cbqueue) < currmin)
                    free_id = i;
            }
        }

        // create a set of args, accept connection using this arg
        // this blocks...
        struct ThreadConnArg *cona = (struct ThreadConnArg *)malloc(sizeof(struct ThreadConnArg));
        cona->eqp = &thread_equipments[free_id];
        cona->client_len = sizeof cona->client;
    retry_accept:
        if ((cona->conn_fd = accept(listen_fd, (struct sockaddr *)&cona->client, &cona->client_len)) == -1) {
            if (errno == EPROTO || errno == ECONNABORTED)
                goto retry_accept;
            fprintf(stderr, "cannot accept connection: %s", ERRSTR);
            free(cona);
            continue;
        }

        // delegate work to that thread
        eventqueue_emplace(cona->eqp->cbqueue, handle_request, (void *)cona);
        // if that thread used to be running, at this point it might have stopped
        if (cona->eqp->cbqueue->state == EVQ_STOPPED) {
            // starting an event queue with no callbacks has no effect
            if (pthread_create(&cona->eqp->thread, NULL, wrap_evqueue_start, (void *)cona->eqp->cbqueue) != 0) {
                fprintf(stderr, "cannot create thread: %s", ERRSTR);
                continue;
            }
            pthread_detach(cona->eqp->thread);
        }
    }

    // never happens
    // close(listen_fd);
    // for (size_t i = 0; i < MAX_WORKERS; ++i)
    //    eventqueue_close(thread_equipments[i].cbqueue);

    return 0;
}
