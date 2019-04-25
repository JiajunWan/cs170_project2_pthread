#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <time.h>

#define MAX_THREADS 128

jmp_buf buf;

static int Initialized = 0;

void signal_handler(int signum);

void signal_handler(int signum)
{
    // signal(SIGINT, signal_handler);
}

static struct sigaction siga;

struct ThreadControlBlock
{
    jmp_buf Registers;
    int *ESP;
    /* 0 for running; 1 for ready to run; 2 for exited */
    int Status;
    pthread_t ThreadID;
};

struct TCBPool
{
    struct ThreadControlBlock TCB[MAX_THREADS];
};

struct TCBPool ThreadPool;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
    static pthread_t ThreadID = (struct _opaque_pthread_t *)9999;
    /* If not initialized, initialize thread pool */
    if (!Initialized)
    {
        /* Initialize the main thread pool */
        int i = 0;
        siga.sa_handler = signal_handler;
        sigemptyset(&siga.sa_mask);
        siga.sa_flags = SA_NODEFER; 
        if(sigaction(SIGALRM, &siga, NULL) == -1) {
		perror("Unable to catch SIGALRM");
		exit(1);
	}
        for (int i = 0; i < MAX_THREADS; i++)
        {
            ThreadPool.TCB[i].Status = 2;
        }
        Initialized = 1;
    }
    else
    {

    }

    return (int)ThreadID;
}

void pthread_exit(void *value_ptr)
{
}

pthread_t pthread_self(void)
{
}