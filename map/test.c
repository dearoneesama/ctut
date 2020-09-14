#include <stdio.h>
#include <stdlib.h>
#include "map.h"

typedef struct spam {
    const char *msg;
} spam;

static spam *cspam(const char *msg) {
    spam *r = malloc(sizeof(spam));
    if (!r) return NULL;
    r->msg = msg;
    return r;
}

int main(void) {
    Map *m = createHashMap(NULL, NULL);
    m->put(m, "dafwtggd", cspam("ssfabb"));
    m->put(m, "bfdr", cspam("ssfabb"));
    m->put(m, "fe kwwg s", cspam("777556"));
    m->put(m, "fa e g dsg", cspam("s538udu28xxx66b"));
    m->put(m, "fev32525", cspam("s5244"));
    m->put(m, "rw", cspam("424ttt"));
    m->put(m, "4242335244", cspam("6666666"));
    m->put(m, "4fawtwbb n", cspam(" uuxu"));
    m->put(m, "orhfasd", cspam("fa3455"));
    m->put(m, "r23iubbxx", cspam("fwrrrt"));
    free(m->get(m, "fev32525"));
    m->remove(m, "fev32525");
    m->put(m, "fev32525", cspam("s5244changed"));

    MapIterator *it = createHashMapIterator(m);
    while (it->has_next(it)) {
        it = it->next(it);
        printf("hashcode: %zu\tkey: %s\t\tvalue:%s\n", it->hashcode, (char*)it->curr->key, ((spam*)it->curr->value)->msg);
        free((spam*)it->curr->value);
    }

    it->free(it);
    m->free(m);
}
