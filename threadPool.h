// issa rantisi 208633255

#ifndef __THREAD_POOL__
#define __THREAD_POOL__
#include <pthread.h>
#include <malloc.h>
#include <unistd.h>
#include "osqueue.h"

typedef enum { NotRun = 0,canRun = 1} state;
typedef struct thread_pool
{
    int numOfThreads;
    pthread_t* threads;
    pthread_mutex_t lock;
    state notAvailableForRun;
    pthread_cond_t cond;
    state availableForRun;
    OSQueue* queue;

}ThreadPool;
typedef struct {
    void (*compFunc)(void *);
    void *params;
} Task;
ThreadPool* tpCreate(int numOfThreads);
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);
int tpInsertTask(ThreadPool* threadPool, void (*compFunc) (void *), void* param);
#endif