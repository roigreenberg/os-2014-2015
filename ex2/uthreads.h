#ifndef _UTHREADS_H
#define _UTHREADS_H

/*
 * User-Level Threads Library (uthreads)
 * Author: OS, huji.os.2015@gmail.com
 */

#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define STACK_SIZE 4096 /* stack size per thread (in bytes) */

/* External interface */
typedef enum Priority { RED, ORANGE, GREEN } Priority;

/* Initialize the thread library */
int uthread_init(int quantum_usecs);

/* Create a new thread whose entry point is f */
int uthread_spawn(void (*f)(void), Priority pr);

/* Terminate a thread */
int uthread_terminate(int tid); 

/* Suspend a thread */
int uthread_suspend(int tid);

/* Resume a thread */
int uthread_resume(int tid);


/* Get the id of the calling thread */
int uthread_get_tid();

/* Get the total number of library quantums */
int uthread_get_total_quantums();

/* Get the number of thread quantums */
int uthread_get_quantums(int tid);

#endif

