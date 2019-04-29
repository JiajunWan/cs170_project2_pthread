//Anzhe Ye, Jiacheng Liu

#include <assert.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define STATUS int
#define EMPTY ((STATUS)0)
#define READY ((STATUS)1)
#define RUNNING ((STATUS)2)
#define EXITED ((STATUS)3)

#define JB_BX 0
#define JB_SI 1
#define JB_DI 2
#define JB_BP 3
#define JB_SP 4
#define JB_PC 5

#define CREATE_FAIL ((int)-1)
#define CREATE_SUCCESS ((int)0)

#define MAX_THREADS 128
#define STACK_SIZE 32767
#define TIMER_PERIOD 50000

struct TCB {
  jmp_buf env;
  void *stack;
  STATUS status;
  void *exit_value;
};

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);

void pthread_exit(void *value_ptr);

pthread_t pthread_self(void);

static int ptr_mangle(int p);

void scheduler();

/**
 * Customer variables
 */
struct TCB tcbs[MAX_THREADS];
int current_thread = 0;
struct sigaction sig;
struct itimerval timer;
bool installed = false;

/**
 * initialize this module in first running
 */
void try_install() {
  // avoid installing duplicatedly
  if (installed)
    return;
  installed = true;

  // allocate main process as index 0
  struct TCB *tcb = &tcbs[0];
  tcb->stack = NULL;
  tcb->status = RUNNING;
  tcb->exit_value = NULL;
  setjmp(tcb->env);

  // init sigaction
  memset(&sig, 0x00, sizeof sig);
  sig.sa_handler = scheduler;
  sig.sa_flags = SA_NODEFER;
  // wait for sigaction
  int sig_status = sigaction(SIGALRM, &sig, NULL);
  assert(sig_status == 0);

  // init timer
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = TIMER_PERIOD;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = TIMER_PERIOD;
  // set timer
  int timer_status = setitimer(ITIMER_REAL, &timer, NULL);
  assert(timer_status == 0);
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg) {
  // TODO
  // sometimes, I think, need lock?

  // allocate a new unused index
  int new_thread = -1;
  for (int i = 1; i < MAX_THREADS; i++)
    if (tcbs[i].status == EMPTY) {
      new_thread = i;
      break;
    }
  if (new_thread == -1)
    return CREATE_FAIL;

  *thread = (pthread_t)new_thread;

  // init tcb
  struct TCB *tcb = &tcbs[new_thread];
  tcb->status = READY;
  tcb->exit_value = NULL;
  tcb->stack = malloc(STACK_SIZE);
  // init env of tcb
  void *esp = tcb->stack + STACK_SIZE;
  esp -= sizeof(arg);
  *((typeof(arg) *)esp) = arg;
  esp -= sizeof(&pthread_exit);
  *(typeof(&pthread_exit) *)esp = &pthread_exit;
#if defined(__APPLE__) || defined(__APPLE_CC__) || defined(__OSX__)
  tcb->env[JB_SP] = ptr_mangle((int)esp);
  tcb->env[JB_PC] = ptr_mangle((int)start_routine);
#else
  tcb->env[0].__jmpbuf[JB_SP] = ptr_mangle((long)esp);
  tcb->env[0].__jmpbuf[JB_PC] = ptr_mangle((long)start_routine);
#endif

  // install this module
  try_install();

  return CREATE_SUCCESS;
}

void pthread_exit(void *value_ptr) {
  struct TCB *tcb = &tcbs[current_thread];
  tcb->status = EXITED;
  tcb->exit_value = value_ptr;
  free(tcb->stack);

  // run next available thread
  scheduler();

  // shouldn't return to break down module
  while (1)
    ;
}

pthread_t pthread_self(void) { return current_thread; }

void scheduler() {
  // stop current thread
  if (tcbs[current_thread].status == RUNNING)
    if (setjmp(tcbs[current_thread].env) == 0)
      tcbs[current_thread].status = READY;
    else
      return;

  // select next thread
  int next_thread = current_thread;
  do {
    next_thread++;
    if (next_thread == MAX_THREADS)
      next_thread = 0;
    if (next_thread == current_thread && tcbs[current_thread].status == EXITED)
      exit(EXIT_SUCCESS);
  } while (tcbs[next_thread].status != READY);

  // run next thread
  current_thread = next_thread;
  tcbs[current_thread].status = RUNNING;
  longjmp(tcbs[current_thread].env, 1);
}

int32_t ptr_mangle(int32_t p) {
  int32_t ret;
  asm(" movl %1, %%eax; "
      " xorl %%gs:0x18, %%eax; "
      " roll $0x9, %%eax; "
      " movl %%eax, %0; "
      : "=r"(ret)
      : "r"(p)
      : "%eax");
  return ret;
}
