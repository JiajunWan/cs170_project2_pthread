#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <setjmp.h>

#define MAX_THREADS = 128

jmp_buf buf;

struct ThreadControlBlock
{
    int Registers;
    int *ESP;
    int status;

};

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{

    return 0;
}

void pthread_exit(void *value_ptr)
{
    
}

pthread_t pthread_self(void)
{

}