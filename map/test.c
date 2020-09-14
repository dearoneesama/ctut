#include <stdio.h>
#include <stdlib.h>
#include "map.h"

typedef struct spam {
    const char *msg;
} spam;

static spam *cspam(const char *msg) {
    spam *r = (spam *)malloc(sizeof(spam));
    if (!r) return NULL;
    r->msg = msg;
    return r;
}

int main(void) {
    Map *m = createHashMap(NULL, NULL);
    m->put(m, (void *)"dafwtggd",   (void *)cspam("ssfabb"));
    m->put(m, (void *)"bfdr",       (void *)cspam("ssfabb"));
    m->put(m, (void *)"fe kwwg s",  (void *)cspam("777556"));
    m->put(m, (void *)"fa e g dsg", (void *)cspam("s538udu28xxx66b"));
    m->put(m, (void *)"fev32525",   (void *)cspam("s5244"));
    m->put(m, (void *)"rw",         (void *)cspam("424ttt"));
    m->put(m, (void *)"4242335244", (void *)cspam("6666666"));
    m->put(m, (void *)"4fawtwbb n", (void *)cspam(" uuxu"));
    m->put(m, (void *)"orhfasd",    (void *)cspam("fa3455"));
    m->put(m, (void *)"r23iubbxx",  (void *)cspam("fwrrrt"));
    free(m->get(m, (void *)"fev32525"));
    m->remove(m, (void *)"fev32525");
    m->put(m, (void *)"fev32525",   (void *)cspam("s5244changed"));

    MapIterator *it = createHashMapIterator(m);
    while (it->has_next(it)) {
        it = it->next(it);
        printf("hashcode: %zu\tkey: %s\t\tvalue:%s\n", it->hashcode, (char*)it->curr->key, ((spam*)it->curr->value)->msg);
        free((spam*)it->curr->value);
    }

    it->free(it);
    m->free(m);
}
