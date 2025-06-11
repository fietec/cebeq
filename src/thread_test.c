#include <cebeq.h>
#include <threading.h>
#include <message_queue.h>
#include <worker.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
#endif

static thread_t worker_thread;

int main() {
    msgq_init();
    thread_create(&worker_thread, NULL, NULL);

    for (int i = 0; i < 10; ++i) {
        char msg[256];
        while (msgq_pop(msg, sizeof(msg))) {
            printf("[LOG] %s\n", msg);
        }
        sleep_ms(500);
    }

    thread_join(worker_thread);
    msgq_destroy();
    return 0;
}