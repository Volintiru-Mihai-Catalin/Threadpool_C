#include "os_threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


os_task_queue_t *search_last(os_task_queue_t *task_queue) {
    os_task_queue_t *tp = task_queue;
    if (tp == NULL) {
        return NULL;
    }

    while (tp->next != NULL) {
        tp = tp->next;
    }

    return tp;
}

/* === TASK === */

/* Creates a task that thread must execute */
os_task_t *task_create(void *arg, void (*f)(void *))
{
    os_task_t *new_task = (os_task_t *)malloc(sizeof(os_task_t));
    new_task->argument = arg;
    new_task->task = f;

    return new_task;
}

/* Add a new task to threadpool task queue */
void add_task_in_queue(os_threadpool_t *tp, os_task_t *t)
{
    pthread_mutex_lock(&tp->taskLock);
    
    os_task_queue_t *last = search_last(tp->tasks);
    os_task_queue_t *new_task = (os_task_queue_t *)malloc(sizeof(os_task_queue_t));
    new_task->next = NULL;
    new_task->task = t;
    if (last != NULL) {
        last->next = new_task;
    } else {
        tp->tasks = new_task;
    }

    pthread_mutex_unlock(&tp->taskLock);
}

/* Get the head of task queue from threadpool */
os_task_t *get_task(os_threadpool_t *tp)
{
    pthread_mutex_lock(&tp->taskLock);
    if (tp->tasks == NULL) {
        pthread_mutex_unlock(&tp->taskLock);
        return NULL;
    }

    os_task_queue_t *head = tp->tasks;
    
    tp->tasks = tp->tasks->next;
    os_task_t *first_task = head->task;
    
    pthread_mutex_unlock(&tp->taskLock);
    
    return first_task;
}

/* === THREAD POOL === */

/* Initialize the new threadpool */
os_threadpool_t *threadpool_create(unsigned int nTasks, unsigned int nThreads)
{
    os_threadpool_t *th = (os_threadpool_t *)malloc(sizeof(os_threadpool_t));
    
    th->should_stop = 0;
    th->num_threads = nThreads;
    th->tasks = NULL;
    pthread_mutex_init(&th->taskLock, NULL);

    pthread_t *threads = (pthread_t *)malloc(nThreads * sizeof(pthread_t));
    th->threads = threads;

    for (int index = 0; index < nThreads; index++) {
        pthread_create(&threads[index], NULL, thread_loop_function, (void *) th);
    }

    return th; 
}

/* Loop function for threads */
void *thread_loop_function(void *args)
{
    os_threadpool_t *tp = (os_threadpool_t *) args;
    while(!tp->should_stop) {
        os_task_t *task = get_task(tp);
        if (task != NULL) {
            (*task->task)(task->argument); 
        }
    }
    return NULL;
}

/* Stop the thread pool once a condition is met */
void threadpool_stop(os_threadpool_t *tp, int (*processingIsDone)(os_threadpool_t *))
{
    while (!(*processingIsDone)(tp)) {}

    tp->should_stop = 1;
    for (int index = 0; index < tp->num_threads; index++) {
        pthread_join(tp->threads[index], NULL);
    }
}
