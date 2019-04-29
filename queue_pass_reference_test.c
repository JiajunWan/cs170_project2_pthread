#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>

static void* thread_dummy()
{
    printf("Test\n");
    pthread_exit(0);
}

struct queue
{
    int front;     /* Init 0 */
    int rear;      /* Init -1 */
    int itemCount; /* Init 0 */
    int IndexQueue[128];
};

int peekfront(struct queue *Queue);

int size(struct queue *Queue);

void pushback(struct queue *Queue, int data);

int popfront(struct queue *Queue);

static struct queue WrapperFunctionTCBIndexQueue = {0, -1, 0};
static struct queue SchedulerTCBIndexQueue = {0, -1, 0};

int main()
{
    // pthread_t tid1 = 0;
    
    // pthread_create(&tid1, NULL,  &thread_dummy, NULL);
    int i;
    pushback(&WrapperFunctionTCBIndexQueue, 0);
    i = popfront(&WrapperFunctionTCBIndexQueue);
    printf("Number: %d\n", i);
    pushback(&WrapperFunctionTCBIndexQueue, i);
    pushback(&WrapperFunctionTCBIndexQueue, 1);
    pushback(&WrapperFunctionTCBIndexQueue, 2);
    i = popfront(&WrapperFunctionTCBIndexQueue);
    printf("Number: %d\n", i);
    pushback(&WrapperFunctionTCBIndexQueue, i);
    pushback(&WrapperFunctionTCBIndexQueue, 3);
    pushback(&WrapperFunctionTCBIndexQueue, 4);
    i = popfront(&WrapperFunctionTCBIndexQueue);
    printf("Number: %d\n", i);
    pushback(&WrapperFunctionTCBIndexQueue, i);
    printf("Number: %d\n", popfront(&WrapperFunctionTCBIndexQueue));
    printf("Number: %d\n", popfront(&WrapperFunctionTCBIndexQueue));
    printf("Number: %d\n", popfront(&WrapperFunctionTCBIndexQueue));
    printf("Number: %d\n", popfront(&WrapperFunctionTCBIndexQueue));
    printf("Number: %d\n", popfront(&WrapperFunctionTCBIndexQueue));
    return 0;
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
