#include <iostream>

#include "../queue/eventqueue.h"

#include "l2cf.hh"

using namespace oneesama;

int main() {
    {
        auto yolo = l2cf{[i = 0] (bool &, int start) mutable {
            return start + i++;
        }};
        // get pointers
        auto (*cb)(int, void *) -> int = yolo.get_cfnptr();
        void *ctx = yolo.get_ctx();
        for (int i = 0; i < 5; ++i)
            std::cout << cb(10, ctx) << std::endl;
    }

    const char *strings[] = {"hehe", "yolo", "hm"};
    // C api
    EventQueue *eq = eventqueue_create();
    {
        auto [cb, ctx, wr] = make_quick_cf_pair([&strings] (bool &free_me, EventQueue *) {
            free_me = true;
            std::cout << strings[1] << std::endl;
        });
        wr.clean_up_on_destruct(false);
        eventqueue_emplace(eq, cb, ctx);
    }
    eventqueue_this_thread_run(eq);
    eventqueue_close(eq);
    // no memory leak
}
