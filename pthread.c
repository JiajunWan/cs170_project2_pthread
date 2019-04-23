#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <setjmp.h>

#define MAX_THREADS 128

jmp_buf buf;

struct ThreadControlBlock
{
    jmp_buf Registers;
    int *ESP;
    /* 0 for running; 1 for ready to run; 2 for exited */
    int Status;
};

struct TCBPool
{
    struct ThreadControlBlock ThreadPool[MAX_THREADS];
};

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
    static int Initialized = 0;
    pthread_t ThreadID;
    /* If not initialized, initialize thread pool */
    if (!Initialized)
    {
        /* Initialize the main thread pool */
        
    }
    else
    {
        
    }
    
    return 
}

void pthread_exit(void *value_ptr)
{
    
}

pthread_t pthread_self(void)
{

}