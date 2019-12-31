// issa rantisi 208633255

#include "threadPool.h"
#define FAILURE -1
#define STDERR 2
#define Error "Error in system call\n"
/**
 * function to counter the length of the string
 * @param string string of the Error.
 * @return size of string
 */
size_t stringLength(char *string){
    size_t i = 0;
    while (*string != '\0'){
        string++;
        i++;
    }
    return i;
}
/**
 * void function that use to print to the user an error if there fail in allocation.
 */
void writeError(){
    write(STDERR, Error, stringLength(Error));
    _exit(FAILURE);
}
void taskFree(Task* task){
    (task->compFunc)(task->params);
    free(task);
}
/**
 * make the task run
 * @param threadPool
 * @return
 */
void *Execute(void *threadPool) {
    // make cast for the threadPool
    ThreadPool* threadPool1 = (ThreadPool*)threadPool;
    // make an task type on the stack
    Task* task;
    // loop until that we can run the tasks for the threadPool
    while(threadPool1->availableForRun == canRun) {
        int mutexLOckFail = pthread_mutex_lock(&(threadPool1->lock));
        if (mutexLOckFail != 0) { writeError(); }
        if (osIsQueueEmpty(threadPool1->queue) && threadPool1->notAvailableForRun == NotRun) {
            int waitFailure =pthread_cond_wait(&(threadPool1->cond), &(threadPool1->lock));
            if( waitFailure != 0){writeError();}
        }
        if (osIsQueueEmpty(threadPool1->queue) && threadPool1->notAvailableForRun == canRun  && threadPool1->availableForRun == canRun) {
            break;
        }
        // get the task out.
        task = (Task*)osDequeue(threadPool1->queue);
        pthread_mutex_unlock(&(threadPool1->lock));
        if (task != NULL) {
            taskFree(task);
        }
    }
    int mutexUnlock = pthread_mutex_unlock(&(threadPool1->lock));
    if(mutexUnlock != 0){writeError();}
    return NULL;
}
/**
 * function that used to destroy the threadPool
 * @param threadPool a pointer to threadPool.
 * @param shouldWaitForTasks
 */
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
    int i;
    // case we got a NULL in threadPool
    if (threadPool == NULL) { return; }
    threadPool->availableForRun = shouldWaitForTasks;
    if (threadPool->notAvailableForRun == NotRun) {
        pthread_mutex_lock(&threadPool->lock);
        threadPool->notAvailableForRun = canRun;
        int conddBroadCast = pthread_cond_broadcast(&(threadPool->cond));
        int mutexxFailure = pthread_mutex_unlock(&(threadPool->lock));
        if ((conddBroadCast != 0) || mutexxFailure != 0) {
            // free all what allocate.
            while(!(osIsQueueEmpty(threadPool->queue))) { free(osDequeue(threadPool->queue)); }
            osDestroyQueue(threadPool->queue);
            if (threadPool->threads != NULL) { free(threadPool->threads); }
            pthread_mutex_destroy(&(threadPool->lock));
            pthread_cond_destroy(&(threadPool->cond));
            free(threadPool);
            return;
        }
        if (threadPool->threads != NULL) {
            for (i = 0; i < threadPool->numOfThreads; i++) {
                if (pthread_join(threadPool->threads[i], NULL) != 0) { writeError(); }
            }
        }
    }
    // free all the task the the threadPool.
    while(!(osIsQueueEmpty(threadPool->queue))) { free(osDequeue(threadPool->queue)); }
    osDestroyQueue(threadPool->queue);
    if (threadPool->threads != NULL) { free(threadPool->threads); }
    pthread_mutex_destroy(&(threadPool->lock));
    pthread_cond_destroy(&(threadPool->cond));
    free(threadPool);
}
int tpInsertTask(ThreadPool* threadPool, void (*compFunc) (void *), void* param) {
    // case of error that threadPool is NULL
    if(threadPool == NULL) { return FAILURE; }
    // case that we can't run the threadPool (mean threads).
    if(threadPool->notAvailableForRun == canRun) { return FAILURE; }
    // case that error in function.
    if(compFunc == NULL){ return FAILURE;}
    // allocate task
    Task* task = (Task*)malloc(sizeof(Task));
    // case of failure of the allocation in task.
    if(task == NULL){free(threadPool); writeError();}
    // init the params of the task.. func.
    task->compFunc = compFunc;
    // init the prarm..
    task->params = param;
    // lock the mutex to prevent intersection.
    int mutexLockFailure = pthread_mutex_lock(&(threadPool->lock));
    // case of error.
    if( mutexLockFailure != 0) { return FAILURE; }
    // put the task in the queue.
    osEnqueue(threadPool->queue, (void*)task);
    // init the signal that takes one.
    int condFailure = pthread_cond_signal(&(threadPool->cond));
    // case of failure.
    if (condFailure != 0) { return FAILURE; }
    // unlock the mutex.
    if(pthread_mutex_unlock(&threadPool->lock) != 0) { return FAILURE; }
    // return 0 as indication for success.
    return 0;
}
ThreadPool* tpCreate(int numOfThreads) {
    // case we got an negative numOfThreads.
    if (numOfThreads <= 0) { return NULL; }
    // allocation for the threadPool
    ThreadPool* threadPool = (ThreadPool*)malloc(sizeof(ThreadPool));
    // case that the allocation failed.
    if (threadPool == NULL) { return NULL; }
    // make an array of threads according the number of thread we got.
    threadPool->threads = (pthread_t*)malloc(sizeof(pthread_t) * numOfThreads);
    // init for the mutex
    int mutexInitFailure = pthread_mutex_init(&(threadPool->lock), NULL);
    // case we got an error we print an error.
    if (mutexInitFailure != 0) { writeError(); }
    // make the queue according the function that we got.
    threadPool->queue = osCreateQueue();
    // make a boolean that indicate we can run the threads.
    threadPool->availableForRun = canRun;
    // make a boolean that indicate that we cannot run the threads.
    threadPool->notAvailableForRun = NotRun;
    // init the cond.
    int condFailure = pthread_cond_init(&(threadPool->cond), NULL);
    // case that failed we print an error to the user.
    if (condFailure!= 0) { writeError(); }
    // initialize the num of threads that we got from the user.
    threadPool->numOfThreads = numOfThreads;
    // counter
    int i;
    // loop to initialize the threads.
    for (i = 0; i < numOfThreads; i++) {
        // create the threads and print an error in case of fail.
        if (pthread_create(&(threadPool->threads[i]), NULL, Execute, (void *) threadPool) != 0) {
            writeError();
        }
    }
    // return a pointer.
    return threadPool;
}