#include <threading.h>

#ifdef _WIN32
static thread_fn global_start_fn = NULL;
static void* global_start_arg = NULL;

DWORD WINAPI win32_thread_wrapper(LPVOID lpParam) {
    (void)lpParam;
    return (DWORD)(uintptr_t)global_start_fn(global_start_arg);
}

int thread_create(thread_t* thread, thread_fn fn, void* arg) {
    global_start_fn = fn;
    global_start_arg = arg;
    *thread = CreateThread(NULL, 0, win32_thread_wrapper, NULL, 0, NULL);
    return *thread != NULL;
}

void thread_join(thread_t thread) {
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

void mutex_init(mutex_t* mtx) {
    InitializeCriticalSection(mtx);
}

void mutex_lock(mutex_t* mtx) {
    EnterCriticalSection(mtx);
}

void mutex_unlock(mutex_t* mtx) {
    LeaveCriticalSection(mtx);
}

void mutex_destroy(mutex_t* mtx) {
    DeleteCriticalSection(mtx);
}

#else

int thread_create(thread_t* thread, thread_fn fn, void* arg) {
    return pthread_create(thread, NULL, fn, arg) == 0;
}

void thread_join(thread_t thread) {
    pthread_join(thread, NULL);
}

void mutex_init(mutex_t* mtx) {
    pthread_mutex_init(mtx, NULL);
}

void mutex_lock(mutex_t* mtx) {
    pthread_mutex_lock(mtx);
}

void mutex_unlock(mutex_t* mtx) {
    pthread_mutex_unlock(mtx);
}

void mutex_destroy(mutex_t* mtx) {
    pthread_mutex_destroy(mtx);
}
#endif
