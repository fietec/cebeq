#ifndef _CBQTHREADING_H
#define _CBQTHREADING_H

#include <cebeq.h>

#ifdef _WIN32
#include <windows.h>
typedef HANDLE thread_t;
typedef CRITICAL_SECTION mutex_t;
#else
#include <pthread.h>
typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;
#endif

typedef void* (*thread_fn)(void* arg);

CBQLIB int thread_create(thread_t* thread, thread_fn fn, void* arg);
CBQLIB void thread_join(thread_t thread);
CBQLIB void mutex_init(mutex_t* mtx);
CBQLIB void mutex_lock(mutex_t* mtx);
CBQLIB void mutex_unlock(mutex_t* mtx);
CBQLIB void mutex_destroy(mutex_t* mtx);

#endif //_CBQTHREADING_H