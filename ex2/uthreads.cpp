/*
 * uthreads.cpp
 *
 *  Created on: Mar 24, 2015
 *      Author: roigreenberg
 */
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <deque>
#include <list>
#include <assert.h>
#include "uthreads.h"

#include <iostream>

using namespace std;

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
		"rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif


//macro for all the system calls that return -1 if failed.
#define SYS_ERROR(syscall, text) if ((syscall) == -1) \
	{cerr << "system error: " << text; exit(1); }

//macro for all the library functions that return -1 if failed.
#define LBR_ERROR(text) cerr << "thread library error: " << text; return -1;

typedef enum State {running, ready, blocked} State;

/*
 * struct for thread
 */
typedef struct
{
	int tid = -1;
	void* ptr_stack;
	sigjmp_buf env;
	Priority pr;
	int quantums;
	State state;
public:
	void thread_init(int tid, void* ptr_stack, \
			Priority pr, State state=ready);
}Thread;

/*
 * initiates thread's id, stack,quantums, priority and state
 */
void Thread::thread_init(int tid, void* ptr_stack, Priority pr, State state)
{
		this->tid = tid;
		this->ptr_stack = ptr_stack;
		this->pr = pr;
		this->quantums = 0;
		this->state = state;
}

int totalQuantums = 1; // counter for total quantom

//the time interval
struct itimerval tv;
int quantomsSec;
int quantomsUsec;

Thread threads[MAX_THREAD_NUM]; //array for all the threads
deque<int> priorities[3]; //the priorities queues
list<int> suspends;//list of blocked threads
int current = 0; //the current running thread, start with the 0 thread
sigset_t signal_set; // signals for blocking


/*
 * set the timer with the time interval
 * first it sets the timer to 0 then to the given time.
 */
void set_timer()
{
	//use to reset the timer
	tv.it_value.tv_sec = 0;  // first time interval, seconds part
	tv.it_value.tv_usec = 0; //first time interval, microseconds part
	tv.it_interval.tv_sec = 0; // following time intervals, seconds part
	tv.it_interval.tv_usec = 0; // following time intervals, microseconds part

	SYS_ERROR(setitimer(ITIMER_VIRTUAL, &tv, NULL), "setitimer failed\n");

	tv.it_value.tv_sec = quantomsSec;
	tv.it_value.tv_usec = quantomsUsec;
	tv.it_interval.tv_sec = quantomsSec;
	tv.it_interval.tv_usec = quantomsUsec;

	SYS_ERROR(setitimer(ITIMER_VIRTUAL, &tv, NULL), "setitimer failed\n");
}



/*
 * return the first unused id or -1 if reaches to maximum threads
 */
int find_first_free_id(Thread threads[])
{
	for (int i = 1; i < MAX_THREAD_NUM; i++)
	{
		if (threads[i].tid == -1)
		{
			return i;
		}
	}
	return -1;
}

/*
 * return the next-to-run thread
 * (if only main is left, returns 0)
 */
int next_thread()
{
	int next = 0;
	for (int i = 0; i < 3; i++)
	{
		if (!priorities[i].empty())
		{
			next = priorities[i].front();
			priorities[i].pop_front();
			break;
		}

	}
	return next;
}

/*
 * push the thread to the ready queue
 */
void push_to_ready (int tid)
{
	threads[tid].state = ready;
	priorities[threads[tid].pr].push_back(tid);
}


/*
 * removes the thread from the ready queue
 */
void remove_from_ready(int tid)
{
	SYS_ERROR(sigprocmask(SIG_BLOCK, &signal_set, NULL), \
				"blocking signal failed\n");

	deque<int> &current_deque = priorities[threads[tid].pr];

	for (deque<int>::iterator it = current_deque.begin(); \
		it != current_deque.end(); ++it)
	{
		if (*it == tid)
			{
				current_deque.erase(it);
				break;
			}
	}

	SYS_ERROR(sigprocmask(SIG_UNBLOCK, &signal_set, NULL), \
				"unblocking signal failed\n");
}

/*
 * switches between threads
 */
void switchThreads(int sig)
{
	totalQuantums++;

	int ret_val = sigsetjmp(threads[current].env,1);

	if (ret_val == 1)
	{
		return;
	}

	int prev = current;
	current = next_thread();
	threads[current].state = running;

	//push the previous thread to ready queue if it's not blocked
	// and not the only active thread
	if (threads[prev].state != blocked && prev != current)
	{
		push_to_ready(prev);
	}

	//removing signal from pending
	sigset_t pending;
	SYS_ERROR(sigemptyset(&pending), "sigemptyset failed\n");
	SYS_ERROR(sigpending(&pending), "sigpending failed\n");
	int waitSig = 1;//only to be used as parameter to syscalls. no real meaning

	int sigMember = sigismember(&pending, SIGVTALRM);

	SYS_ERROR(sigMember, "sigismember failed\n");

	if (sigMember == 1)
	{
		if(sigwait(&pending, &waitSig) > 0)
			{
				cerr << "system error: sigwait failed\n";
				exit(1);
			}
	}


	threads[current].quantums++;

	set_timer();

	siglongjmp(threads[current].env,1);

}



/* Initialize the thread library */
int uthread_init(int quantum_usecs)
{

	if (quantum_usecs <= 0)
	{
		LBR_ERROR( "non-positive quantum usecs\n");
	}

	if(signal(SIGVTALRM, switchThreads) == SIG_ERR)
	{
		cerr << "system error: signal syscall failed\n";
		exit(1);
	}

	SYS_ERROR(sigemptyset(&signal_set), "sigemptyset failed\n");
	SYS_ERROR(sigaddset(&signal_set, SIGVTALRM), "sigaddset failed\n");

	//divides the given time to seconds and microsecs.
	quantomsSec = quantum_usecs / 1000000;
	quantomsUsec = quantum_usecs % 1000000;

	//"spawn" the 0 thread(the main)
	threads[0].thread_init(0, NULL, ORANGE, running);
	threads[0].quantums = 1;

	set_timer();

	sigsetjmp(threads[0].env, 1);

	return 0;
}

/* Create a new thread whose entry point is f */
int uthread_spawn(void (*f)(void), Priority pr)
{
	SYS_ERROR(sigprocmask(SIG_BLOCK, &signal_set, NULL), \
				"blocking signal failed\n");

	address_t sp, pc;
	int tid;

	if((tid = find_first_free_id(threads)) == -1)
	{
		LBR_ERROR( "maximum threads\n");
	}


	void* ptr_stack = malloc(STACK_SIZE);
	if (ptr_stack == NULL)
	{
		cerr << "system error: malloc failed\n";
		exit(1);
	}

	sp = (address_t)ptr_stack + STACK_SIZE - sizeof(address_t);
	pc = (address_t)f;

	threads[tid].thread_init(tid, ptr_stack, pr);
	threads[tid].state=ready;
	push_to_ready(threads[tid].tid);

	sigsetjmp(threads[tid].env, 1);
	(threads[tid].env->__jmpbuf)[JB_SP] = translate_address(sp);
	(threads[tid].env->__jmpbuf)[JB_PC] = translate_address(pc);

	SYS_ERROR(sigemptyset(&threads[tid].env->__saved_mask),\
					     "sigemptyset failed\n");


	SYS_ERROR(sigprocmask(SIG_UNBLOCK, &signal_set, NULL), \
				"unblocking signal failed\n");

	return tid;
}

/* Terminate a thread */
int uthread_terminate(int tid)
{
	SYS_ERROR(sigprocmask(SIG_BLOCK, &signal_set, NULL), \
				"blocking signal failed\n");

	if ((tid < 0) || (threads[tid].tid < 0))
	{
		SYS_ERROR(sigprocmask(SIG_UNBLOCK, &signal_set, NULL), \
						"unblocking signal failed\n");
		LBR_ERROR("no such thread\n");
	}

	if (tid)
	{
		free(threads[tid].ptr_stack);
		threads[tid].tid = -1;
		threads[tid].quantums = 0;
		if (threads[tid].state == running)
		{
			threads[tid].state = blocked; //use to prevent switch to push it back to queue
			switchThreads(0);

			SYS_ERROR(sigprocmask(SIG_UNBLOCK, &signal_set, NULL), \
							"unblocking signal failed\n");

			return 0;
		}
		else if (threads[tid].state == ready)
		{
			remove_from_ready(tid);
		}
		else
		{
			suspends.remove(tid);
		}
	}
	else
	{
		for (int i = 1; i < MAX_THREAD_NUM; i++)
		{
			if (threads[i].tid != -1)
			{
				free(threads[i].ptr_stack);
				threads[i].tid = -1;
			}
		}
		exit(0);
	}

	SYS_ERROR(sigprocmask(SIG_UNBLOCK, &signal_set, NULL), \
				"unblocking signal failed\n");

	return 0;
}

/* Suspend a thread */
int uthread_suspend(int tid)
{
	SYS_ERROR(sigprocmask(SIG_BLOCK, &signal_set, NULL), \
				"blocking signal failed\n");

	if ((tid <= 0) || (threads[tid].tid < 0))
	{
		SYS_ERROR(sigprocmask(SIG_UNBLOCK, &signal_set, NULL), \
						"unblocking signal failed\n");
		if (tid == 0)
		{
			LBR_ERROR("cannot suspend main thread\n");
		}
		else
		{
			LBR_ERROR("no such thread\n");
		}
	}

	if (threads[tid].state != blocked)
	{
		suspends.push_front(tid);
		if (threads[tid].state == ready)
		{
			remove_from_ready(tid);
			threads[tid].state = blocked;
		}
		else
		{
			threads[tid].state = blocked;
			switchThreads(0);
		}
	}

	SYS_ERROR(sigprocmask(SIG_UNBLOCK, &signal_set, NULL), \
				"unblocking signal failed\n");

	return 0;
}

/* Resume a thread */
int uthread_resume(int tid)
{
	SYS_ERROR(sigprocmask(SIG_BLOCK, &signal_set, NULL), \
				"blocking signal failed\n");

	if ((tid < 0) || (threads[tid].tid < 0))
	{
		SYS_ERROR(sigprocmask(SIG_UNBLOCK, &signal_set, NULL), \
						"unblocking signal failed\n");
		LBR_ERROR("no such thread\n");

	}

	if (threads[tid].state == blocked)
	{
		suspends.remove(tid);
		threads[tid].state = ready;
		push_to_ready(tid);
	}

	SYS_ERROR(sigprocmask(SIG_UNBLOCK, &signal_set, NULL), \
				"unblocking signal failed\n");

	return 0;
}


/* Get the id of the calling thread */
int uthread_get_tid()
{
	return current;
}

/* Get the total number of library quantums */
int uthread_get_total_quantums()
{
	return totalQuantums;
}

/* Get the number of thread quantums */
int uthread_get_quantums(int tid)
{
	if ((tid < 0) || (threads[tid].tid < 0))
		{
			LBR_ERROR( "no such thread\n");
		}
	return threads[tid].quantums;
}
