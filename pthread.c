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

void scheduler(int signum);

static long int i64_ptr_mangle(long int p);

static void WrapperFunction();

struct ThreadControlBlock
{
    jmp_buf Registers;
    int *ESP;
    /* 0 for running; 1 for ready to run; 2 for exited */
    int Status;
    pthread_t ThreadID;
    void *(*start_routine)(void *);
    void *arg;
};

struct TCBPool
{
    struct ThreadControlBlock TCB[MAX_THREADS];
};

struct queue
{
    int front;     /* Init 0 */
    int rear;      /* Init -1 */
    int itemCount; /* Init 0 */
    int IndexQueue[MAX_THREADS];
};

int peekfront(struct queue *Queue);

int size(struct queue *Queue);

void pushback(struct queue *Queue, int data);

int popfront(struct queue *Queue);

static struct queue WrapperFunctionTCBIndexQueue = {0, -1, 0};
static struct queue SchedulerTCBIndexQueue = {0, -1, 0};

struct TCBPool ThreadPool;

static struct sigaction siga;

static int NextCreateTCBIndex = 0;

static void WrapperFunction()
{
    int TCBIndex = popfront(&WrapperFunctionTCBIndexQueue);
    ThreadPool.TCB[TCBIndex].start_routine(ThreadPool.TCB[TCBIndex].arg);
    pthread_exit(0);
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
    static pthread_t ThreadID = (struct _opaque_pthread_t *)9999;
    /* If not initialized, initialize thread pool */
    if (!Initialized)
    {
        /* Initialize the main thread pool */

        /* Specify sigaction parameters */
        siga.sa_handler = scheduler;
        sigemptyset(&siga.sa_mask);
        siga.sa_flags = SA_NODEFER;
        if (sigaction(SIGALRM, &siga, NULL) == -1)
        {
            perror("Unable to catch SIGALRM");
            exit(1);
        }

        int i;
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

pthread_t pthread_self(void)
{
}

void pthread_exit(void *value_ptr)
{
}

void scheduler(int signum)
{
}

static long int i64_ptr_mangle(long int p)
{
    long int ret;
    asm(" mov %1, %%rax;\n"
        " xor %%fs:0x30, %%rax;"
        " rol $0x11, %%rax;"
        " mov %%rax, %0;"
        : "=r"(ret)
        : "r"(p)
        : "%rax");
    return ret;
}

int peekfront(struct queue *Queue)
{
    return Queue->IndexQueue[Queue->front];
}

int size(struct queue *Queue)
{
    return Queue->itemCount;
}

void pushback(struct queue *Queue, int data)
{

    if (Queue->rear == 128 - 1)
    {
        Queue->rear = -1;
    }

    Queue->rear += 1;
    Queue->IndexQueue[Queue->rear] = data;
    Queue->itemCount += 1;
}

int popfront(struct queue *Queue)
{
    int data = Queue->IndexQueue[Queue->front];
    Queue->front += 1;

    if (Queue->front == 128)
    {
        Queue->front = 0;
    }

    Queue->itemCount -= 1;
    return data;
}

// int peekfront(struct queue Queue)
// {
//     return Queue.IndexQueue[Queue.front];
// }

// int size(struct queue Queue)
// {
//     return Queue.itemCount;
// }

// void pushback(struct queue Queue, int data)
// {

//     if (Queue.rear == MAX_THREADS - 1)
//     {
//         Queue.rear = -1;
//     }

//     Queue.IndexQueue[++Queue.rear] = data;
//     Queue.itemCount += 1;
// }

// int popfront(struct queue Queue)
// {
//     int data = Queue.IndexQueue[Queue.front++];

//     if (Queue.front == MAX_THREADS)
//     {
//         Queue.front = 0;
//     }

//     Queue.itemCount -= 1;
//     return data;
// }
