#include <utils/log.h>
#include <utils/common.h>
#include <utils/runnable/default_runnable.h>

#define LOG_TAG "test_default_runnable"

static void loop1(struct pthread_wrapper* thread, void* param) {
    LOGI("loop1, param=%ld\n", (long)param);
}

static void loop2(struct pthread_wrapper* thread, void* param) {
    LOGI("loop2, param=%ld\n", (long)param);
}

int main(int argc, char *argv[]) {
    struct default_runnable* runnable1 = _new(struct default_runnable,
            default_runnable);

    struct default_runnable* runnable2 = _new(struct default_runnable,
            default_runnable);

    runnable1->set_thread_count(runnable1, 10);
    runnable1->runnable.run = loop1;
    runnable1->start(runnable1, NULL);

    runnable2->set_thread_count(runnable2, 5);
    runnable2->runnable.run = loop2;
    runnable2->start(runnable2, NULL);

    runnable1->wait(runnable1);
    runnable2->wait(runnable2);

    _delete(runnable1);
    _delete(runnable2);

    LOGI("test done\n");

    return 0;
}
