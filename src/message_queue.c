#include <stdlib.h>
#include <string.h>

#include <message_queue.h>
#include <threading.h>

static char queue[MAX_QUEUE][MAX_MSG_LEN];
static int head = 0, tail = 0;
static mutex_t lock;

void msgq_init() {
    mutex_init(&lock);
}

void msgq_destroy() {
    mutex_destroy(&lock);
}

void msgq_push(const char* msg) {
    mutex_lock(&lock);
    strncpy(queue[tail], msg, MAX_MSG_LEN - 1);
    queue[tail][MAX_MSG_LEN - 1] = '\0';
    tail = (tail + 1) % MAX_QUEUE;
    if (tail == head) head = (head + 1) % MAX_QUEUE;
    mutex_unlock(&lock);
}

int msgq_pop(char* out, int max_len) {
    mutex_lock(&lock);
    if (head == tail) {
        mutex_unlock(&lock);
        return 0;
    }
    strncpy(out, queue[head], max_len - 1);
    out[max_len - 1] = '\0';
    head = (head + 1) % MAX_QUEUE;
    mutex_unlock(&lock);
    return 1;
}