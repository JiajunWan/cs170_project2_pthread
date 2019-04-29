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

int main()
{
    pthread_t tid1 = 0;
    
    pthread_create(&tid1, NULL,  &thread_dummy, NULL);

    return 0;
}