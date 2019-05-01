#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <setjmp.h>
#include <time.h>

#define NUM_TESTS 8
#define PASS 1
#define FAIL 0

#define TEST_WAIT_MILI 2000 // how many miliseconds do we wait before assuming a test is hung

#define MAX_THREADS 128

void scheduler(int signum);

static long int i64_ptr_mangle(long int p);

static void WrapperFunction();

enum STATE
{
    ACTIVE,
    READY,
    EXITED
};

struct ThreadControlBlock
{
    pthread_t ThreadID;
    char *ESP;
    enum STATE Status;
    void *(*start_routine)(void *);
    void *arg;
    jmp_buf Registers;
};

struct TCBPool
{
    /* Maximum 129 concurrent running threads including main thread whose TCB Index is 0 */
    struct ThreadControlBlock TCB[MAX_THREADS + 1];
};

struct queue
{
    int front;     /* Init 0 */
    int rear;      /* Init -1 */
    int itemCount; /* Init 0 */
    int IndexQueue[MAX_THREADS + 1];
};

int peekfront(struct queue *Queue);

int size(struct queue *Queue);

void pushback(struct queue *Queue, int Index);

int popfront(struct queue *Queue);

static struct queue WrapperFunctionTCBIndexQueue = {0, -1, 0};
static struct queue SchedulerThreadPoolIndexQueue = {0, -1, 0};

struct TCBPool ThreadPool;

static struct sigaction siga;

static int Initialized = 0;

static int NextCreateTCBIndex = 1;

// static int QueuedThreadNum = 1;

static unsigned long ThreadID = 1;

static struct itimerval Timer = {0}, Zero_Timer = {0};

/* May Delete static */
static void WrapperFunction()
{
    int TCBIndex = popfront(&WrapperFunctionTCBIndexQueue);
    ThreadPool.TCB[TCBIndex].start_routine(ThreadPool.TCB[TCBIndex].arg);
    pthread_exit(0);
}

void main_thread_init()
{
    siga.sa_handler = scheduler;
    sigemptyset(&siga.sa_mask);
    siga.sa_flags = SA_NODEFER;
    if (sigaction(SIGALRM, &siga, NULL) == -1)
    {
        perror("Unable to catch SIGALRM!");
        exit(1);
    }

    Timer.it_value.tv_sec = 50 / 1000;
    Timer.it_value.tv_usec = (50 * 1000) % 1000000;
    Timer.it_interval = Timer.it_value;

    int i;
    for (i = 1; i < MAX_THREADS + 1; i++)
    {
        ThreadPool.TCB[i].Status = EXITED;
    }
    Initialized = 1;

    /* Main thread TCB Index is 0 and Main thread ID is 99999 */
    ThreadPool.TCB[0].ThreadID = 99999;
    ThreadPool.TCB[0].Status = ACTIVE;
    ThreadPool.TCB[0].ESP = NULL;
    ThreadPool.TCB[0].start_routine = NULL;
    ThreadPool.TCB[0].arg = NULL;
    setjmp(ThreadPool.TCB[0].Registers);
    pushback(&SchedulerThreadPoolIndexQueue, 0);

    /* Check if there is error in starting timer */
    if (setitimer(ITIMER_REAL, &Timer, NULL) == -1)
    {
        perror("Error Setting Timer!");
        exit(1);
    }

    /* Pause Timer */
    pause();
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
    /* If not initialized, initialize thread pool */
    if (!Initialized)
    {
        /* Initialize the main thread pool */
        main_thread_init();
    }
    else if (size(&SchedulerThreadPoolIndexQueue) <= 129)
    {

        /* Pause Timer */
        setitimer(ITIMER_REAL, &Zero_Timer, &Timer);

        /* Find the next available Thread Pool slot */
        int i;
        for (i = 1; i < MAX_THREADS + 1; i++)
        {
            if (ThreadPool.TCB[i].Status == EXITED)
            {
                NextCreateTCBIndex = i;
                break;
            }
        }

        /* Initialize for the chosen slot */
        ThreadPool.TCB[NextCreateTCBIndex].ThreadID = ThreadID++;
        ThreadPool.TCB[NextCreateTCBIndex].Status = ACTIVE;
        ThreadPool.TCB[NextCreateTCBIndex].ESP = (char *)malloc(32767);
        ThreadPool.TCB[NextCreateTCBIndex].start_routine = start_routine;
        ThreadPool.TCB[NextCreateTCBIndex].arg = arg;
        *thread = ThreadPool.TCB[NextCreateTCBIndex].ThreadID;

        /* Change data of slot 8 bytes below the top of stack to the address of pthread_exit */
        *(int *)(ThreadPool.TCB[NextCreateTCBIndex].ESP+32759) = (int)pthread_exit;

        /* Switch between setjmp and memset */
        setjmp(ThreadPool.TCB[NextCreateTCBIndex].Registers);

        // /* initialize jump buf structure to be 0, just in case there's garbage */
        // memset(&tmp_tcb.jb,0,sizeof(tmp_tcb.jb));
        // /* the jmp buffer has a stored signal mask; zero it out just in case */
        // sigemptyset(&tmp_tcb.jb->__saved_mask);

        /* Save the address of Wrapper Function to a pointer */
        void (*WrapperFunctionPointer)() = &WrapperFunction;

        /* Change External Stack Pointer in the jmp_buf */
        ThreadPool.TCB[NextCreateTCBIndex].Registers[0].__jmpbuf[6] = i64_ptr_mangle((char)(ThreadPool.TCB[NextCreateTCBIndex].ESP+32759));

        /* Change External Instruction Pointer to Wrapper Function in the jmp_buf */
        ThreadPool.TCB[NextCreateTCBIndex].Registers[0].__jmpbuf[7] = i64_ptr_mangle((char)WrapperFunctionPointer);

        /* Add the New Thread Thread Pool Index to the Queue */
        pushback(&WrapperFunctionTCBIndexQueue, NextCreateTCBIndex);
        pushback(&SchedulerThreadPoolIndexQueue, NextCreateTCBIndex);

        // /* Track the number of queued threads */
        // QueuedThreadNum++;

        /* Resume Timer */
        setitimer(ITIMER_REAL, &Timer, NULL);

        return 0;
    }
    /* Reach the Max number of concurrent threads and return -1 as error */
    else
    {
        return -1;
    }
}

pthread_t pthread_self(void)
{
    return (pthread_t)ThreadPool.TCB[peekfront(&SchedulerThreadPoolIndexQueue)].ThreadID;
}

void pthread_exit(void *value_ptr)
{
    /* If no pthread_create call, only main */
    if (Initialized == 0)
    {
        exit(0);
    }

    /* Stop Timer */
    setitimer(ITIMER_REAL, &Zero_Timer, NULL);

    /* If the current exit caller is main, then exit */
    if (ThreadPool.TCB[peekfront(&SchedulerThreadPoolIndexQueue)].ThreadID == 99999)
    {
        exit(0);
    }

    /* Clean Up */
    free((char *)ThreadPool.TCB[peekfront(&SchedulerThreadPoolIndexQueue)].ESP);
    ThreadPool.TCB[peekfront(&SchedulerThreadPoolIndexQueue)].ESP = NULL;
    ThreadPool.TCB[peekfront(&SchedulerThreadPoolIndexQueue)].start_routine = NULL;
    ThreadPool.TCB[peekfront(&SchedulerThreadPoolIndexQueue)].arg = NULL;
    ThreadPool.TCB[peekfront(&SchedulerThreadPoolIndexQueue)].Status = EXITED;
    popfront(&SchedulerThreadPoolIndexQueue);

    /* Start Timer */
    setitimer(ITIMER_REAL, &Timer, NULL);

    /* Longjmp to the front(next) thread registers */
    longjmp(ThreadPool.TCB[peekfront(&SchedulerThreadPoolIndexQueue)].Registers, 1);

    // QueuedThreadNum--;
}

void scheduler(int signum)
{
    /* If only one main thread, just return */
    if (size(&SchedulerThreadPoolIndexQueue) <= 1)
    {
        return;
    }

    if (setjmp(ThreadPool.TCB[peekfront(&SchedulerThreadPoolIndexQueue)].Registers) == 0)
    {
        /* Pushback the poped front Thread Pool Index of the saved thread to the end of queue */
        pushback(&SchedulerThreadPoolIndexQueue, popfront(&SchedulerThreadPoolIndexQueue));

        /* Longjmp to the front(next) thread registers */
        longjmp(ThreadPool.TCB[peekfront(&SchedulerThreadPoolIndexQueue)].Registers, 1);
    }

    return;
}


int peekfront(struct queue *Queue)
{
    return Queue->IndexQueue[Queue->front];
}

int size(struct queue *Queue)
{
    return Queue->itemCount;
}

void pushback(struct queue *Queue, int Index)
{

    if (Queue->rear == 128 - 1)
    {
        Queue->rear = -1;
    }

    Queue->rear += 1;
    Queue->IndexQueue[Queue->rear] = Index;
    Queue->itemCount += 1;
}

int popfront(struct queue *Queue)
{
    int Index = Queue->IndexQueue[Queue->front];
    Queue->front += 1;

    if (Queue->front == 128)
    {
        Queue->front = 0;
    }

    Queue->itemCount -= 1;
    return Index;
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


//if your code compiles you pass test 0 for free
//==============================================================================
static int test0(void){
    return PASS;
}

//basic pthread create and exit tests
//==============================================================================
static void* _thread_dummy(void* arg){
    pthread_t a = pthread_self();
    pthread_exit(0);
}

static void* _thread_dummy_loop(void* arg){
    pthread_t a = pthread_self();
    for(int i = 0; i < 100000000; i++);
    pthread_exit(0);
}

static int test1(void){
    pthread_t tid1 = 0;
    pthread_t tid2 = 0;
    
    pthread_create(&tid1, NULL,  &_thread_dummy, NULL);
    pthread_create(&tid2, NULL,  &_thread_dummy_loop, NULL);

    if(tid1 != tid2){
        return PASS;
    }else{
        return FAIL;
    }
}


//basic pthread self test
//==============================================================================
static pthread_t global_tid1 = 1; 
static void* _thread_self_test(void* arg){
    global_tid1 = pthread_self();
    pthread_exit(0);
}

static int test2(void){
    pthread_t tid1 = 0;
    
    pthread_create(&tid1, NULL,  &_thread_self_test, NULL);
    
    while((global_tid1 != tid1)); //failure occurs on a timeout
    return PASS;
}


//scheduler test 1
//==============================================================================
static int a3 = 0;
static int b3 = 0;

static void* _thread_schedule_a(void* arg){
    while(1){
        a3 = 1;
    }

}
static void* _thread_schedule_b(void* arg){
    while(1){
        if(a3){
            b3 = 1;
        }
    }
}

static int test3(void){
    pthread_t tid1; pthread_t tid2;
        
    pthread_create(&tid1, NULL,  &_thread_schedule_a, NULL);
    pthread_create(&tid2, NULL,  &_thread_schedule_b, NULL);
    
    while(b3 != 1); //just wait, failure occurs if there is a timeout
    return PASS;
        

}


//scheduler test 2
//==============================================================================
static int a4;
static void* _thread_inc4(void* arg){
    
    for(int i = 0; i < 10; i++){
        a4++;
    }
    pthread_exit(0);
    
}

static int test4(void){
    pthread_t tid1; pthread_t tid2;
        
    pthread_create(&tid1, NULL,  &_thread_inc4, NULL);
    pthread_create(&tid2, NULL,  &_thread_inc4, NULL);
    
    
    for(int i = 0; i < 10; i++){
        a4++;
    }
    
    while(a4 != 30); //just wait, failure occurs if there is a timeout
    return PASS;

}


//scheduler test 3
//==============================================================================
static int a5;
static void* _thread_inc1(void* arg){
    a5++;
    pthread_exit(0);
    
}

static int test5(void){
    pthread_t tid1; 
    
    for(int i = 0; i < 128; i++){
        pthread_create(&tid1, NULL,  &_thread_inc1, NULL);
    }
    
    while(a5 != 128); //just wait, failure occurs if there is a timeout
    return PASS;

}


//passing arguments
//==============================================================================
static int e;
static void* _thread_arg(void* arg){
    e = *(int*)arg;
    pthread_exit(0);
}

static int test6(void){
    pthread_t tid1; pthread_t tid2;

    int arg = 244567;
    pthread_create(&tid1, NULL,  &_thread_arg, &arg);
         
    while(e != arg); //just wait, failure occurs if there is a timeout
    return PASS;

}

//does your stack work?
//==============================================================================
static int _thread_fib(int a){
    if(a == 0) {
        return 0;
    }
    if(a == 1){
        return 1;
    }
    
    return _thread_fib(a-1) + _thread_fib(a-2);
}


static void* _thread_fcn(void* arg){
    int fib = *(int*)arg;
    *(int*)arg = _thread_fib(fib);
    pthread_exit(0);
}


static int test7(void){
    pthread_t tid1; pthread_t tid2;

    int arg = 12;
    pthread_create(&tid1, NULL,  &_thread_fcn, &arg);

    while(arg != 144); //just wait, failure occurs if there is a timeout
    return PASS;

}



//end of tests
//==============================================================================


/**
 *  Some implementation details: Main spawns a child process for each
 *  test, that way if test 2/20 segfaults, we can still run the remaining
 *  tests. It also hands the child a pipe to write the result of the test.
 *  the parent polls this pipe, and counts the test as a failure if there
 *  is a timeout (which would indicate the child is hung).
 */


static int (*test_arr[NUM_TESTS])(void) = {&test0, &test1, &test2, &test3, &test4, &test5, &test6, &test7};


int main(void){
    
    int status; pid_t pid;
    int pipe_fd[2]; int timeout; struct pollfd poll_fds;
    int score = 0; int total_score = 0;

    int devnull_fd = open("/dev/null", O_WRONLY);

    pipe(pipe_fd);
    poll_fds.fd = pipe_fd[0]; // only going to poll the read end of our pipe
    poll_fds.events = POLLRDNORM; //only care about normal read operations
        
    for(int i = 0; i < NUM_TESTS; i++){
        score = 0;
        pid = fork();

        //child, launches the test
        if (pid == 0){
            dup2(devnull_fd, STDOUT_FILENO); //begone debug messages
            dup2(devnull_fd, STDERR_FILENO);
            
            score = test_arr[i]();
            
            write(pipe_fd[1], &score, sizeof(score));
            exit(0); 
        }

        //parent, polls on the pipe we gave the child, kills the child,
        //keeps track of score
        else{  
            
            if(poll(&poll_fds, 1, TEST_WAIT_MILI)){
                read(pipe_fd[0], &score, sizeof(score));
            }
            
            total_score += score;
            kill(pid, SIGKILL);
            waitpid(pid,&status,0);
            
            
            if(score){
                printf("test %i : PASS\n", i);
            }
            else{
                printf("test %i : FAIL\n", i);
            }
        }
    }
    
    printf("total score was %i / %i\n", total_score, NUM_TESTS);
    return 0;
}
