/****
 * file threads.c
 *
   A partial implementation of a user-level thread package  
   Written by Douglas Jones,                  Feb 17, 1998
   Updated with minor bug fix in thread_exit, Feb 19, 1998
   Updated so free called on original stack,  Feb 19, 2002
   Updated with defines for _longjmp,         Aug 13, 2002

   Copyright Douglas Jones, 1998.

   Permission is hereby granted to make and modify
   personal copies of this code for noncommercial or
   educational use.
                                                           *
   All other rights are reserved!                          *
                                                        ****/
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

/* just in case this isn't compiled on a UNIX system
   we back off to use the standard setjmp and longjmp.
   UNIX systems have _setjmp that just does control
   transfers, and then setjmp (no underscore) that also
   saves the signal masks.  Saving those makes sense
   for exception handlers, but not for coroutines.
*/
#ifndef _setjmp
#define _setjmp setjmp
#endif

#ifndef _longjmp
#define _longjmp longjmp
#endif

/****
 * PART I: -- Reverse Engineer the jump buffer     *
 * This is the key to user-level context switching *
                                                ****/
/****                                                         *
 *  tools used by studyit to understand jump buffer structure * 
 *                                                         ****/
static int * low_bound;		/* below probe on stack */
static int * probe_local;	/* local to probe on stack */
static int * high_bound;	/* above probe on stack */
static int * prior_local;	/* value of probe_local from earlier call */

static jmp_buf probe_env;	/* saved environment of probe */
static jmp_buf probe_sameAR;	/* second environment saved by same call */
static jmp_buf probe_samePC;	/* environment saved on previous call */

static jmp_buf * ref_probe = &probe_samePC;	/* switches between probes */

/****
 * reports from studyit giving results of study
 * -- our focus here is on learning how to make
   a jump buffer that points to newly allocated *
   activation record on the heap                *
                                             ****/
static int stack_direction;	/* +1 if stack grows up, -1 if down */
long int local_offset;		/* lowest location in AR from local vars */
jmp_buf probe_record;  		/* nonzero entries mean fields that change */

/****
 * code to explore the activation record *
 * and jump buffer structure             *       
                                      ****/
void boundhigh()
/* purpose:  set high_bound to a local variable beyond probe on stack */
{
	int c;
	high_bound = &c;
}

void probe()
/* purpose:  probe the activation record and jump buffer structure */
{
	int c;
	prior_local = probe_local;
	probe_local = &c;
	_setjmp( *ref_probe );
	ref_probe = &probe_env;
	_setjmp( probe_sameAR );
	(void) boundhigh();
}

void boundlow()
/* purpose:  set low_bound to a local variable before probe on stack */
{
	int c;
	low_bound = &c;
	(void) probe();
}

void fill()
/* purpose:  put some padding on the stack and then study the result */
{
	(void) boundlow();
}

static void studyit()
/* purpose:  study the result of probes */
{
	if (sizeof(long int) != sizeof(long int *)) {
		fprintf( stderr, "Pointer size is wrong\n" );
	}

	if (((long int)high_bound) > ((long int)low_bound)) {
		/* stack grows up */
		stack_direction =  1;
	} else {
		/* stack grows down */
		stack_direction = -1;
	}

	/* study the jump buffer */
	{
		int i;
		long int * p;
		long int * sameAR;
		long int * samePC;
		long int * probe_rec;
		long int prior_diff = (long int)probe_local
					- (long int)prior_local;
		long int min_frame = (long int)probe_local;

		/* following line views jump buffer as array of long int */
		p = (long int *)probe_env;
		sameAR = (long int *)probe_sameAR;
		samePC = (long int *)probe_samePC;
		probe_rec = (long int *)probe_record;

		for (i = 0; i < sizeof(jmp_buf); i += sizeof(long int) ) {
			*probe_rec = 0;
			if (*p != *samePC) {
				if (*p != *sameAR) {
					fprintf( stderr, "No Thread Launch\n" );
					exit(-1);
				}
				if ((*p - *samePC) == prior_diff) {
					/* record frame dependency */
					*probe_rec = 1;
					if (stack_direction == 1) { /* up */
						if (min_frame > *p) {
							min_frame = *p;
						}
					} else { /* grows down */
						if (min_frame < *p) {
							min_frame = *p;
						}
					}
				}
			}
 			
			p++, sameAR++, samePC++, probe_rec++;
		}

		if (stack_direction == 1) { /* grows up */
			local_offset
				= (long int)probe_local - min_frame;
		} else { /* stack grows down */
			local_offset
				= min_frame - (long int)probe_local;
		}
	}
}

void thread_startup_report()
/* reports on the results of the study done by above code */
{
	int i;
	long int * jb;
	printf( "Thread startup study results\n" );
	if (stack_direction == 1) {
		printf( "  Stack grows up\n" );
	} else {
		printf( "  Stack grows down\n" );
	}
	printf( "  Local variable offset from AR base = %1d\n", local_offset );
	printf( "  Jump buffer fields subject to modification:\n" );
	jb = (long int *)probe_record;
	for (i = 0; i < sizeof(jmp_buf); i += sizeof(long int) ) {
		if (*jb != 0) {
			printf( "    %1d\n", i );
		}
		jb++;
	}
	printf( "  Jump buffer size: %1d\n", sizeof(jmp_buf) );
}

/****
 * PART II: -- The Thread Manager          *
 * Based on the results reported by Part I *
                                        ****/

/****                *
 * thread data types * 
 *                ****/

struct thread {
	struct thread * next;	/* used to link threads into queues */
	int size;		/* the size of the thread */
	void (* proc)(int);	/* procedure for body of thread */
	int param;		/* parameter to base procedure */
	jmp_buf state;		/* the state of the thread */

	/* size bytes of stack follow here */

};

#define thread_null (struct thread *)0

struct thread_queue {
	struct thread * head;
	struct thread * tail;
};

/****                                    *
 * Fundamental scheduler data structures *
 *                                    ****/

static struct thread_queue readylist;  /* the list of all ready threads */
static struct thread * current;        /* the current running thread */
static void * mem_to_free;             /* anything to be passed to free() */

static char * thread_error;            /* string giving package death cause */
static jmp_buf thread_death;           /* used on thread package death */
static jmp_buf go_free_it;             /* used to call free from original stk */

/****                              *
 * code for thread_queue data type * 
 *                              ****/

static void thread_queue_init( struct thread_queue * q )
/* initialize q */
{
	q->head = thread_null;
	q->tail = thread_null;
}

static void thread_enqueue( struct thread * t, struct thread_queue * q )
/* enqueue t on q */
{
	t->next = thread_null;
	if (q->head == thread_null) {
		q->head = t;
		q->tail = t;
	} else {
		q->tail->next = t;
		q->tail = t;
	}
}

static struct thread * thread_dequeue( struct thread_queue * q )
/* dequeue and return a thread from q */
{
	if (q->head == thread_null) {
		return thread_null;
	} else {
		struct thread * t;
		t = q->head;
		q->head = t->next;
		return t;
	}
}

/****                                       *
 * user callable thread management routines * 
 *                                       ****/

void thread_manager_init()
/* call this once before launching any threads */
{
	/* first study current system and see if thread launch worth trying */
	(void) fill();		/* do a probe with filler on stack */
	(void) boundlow();	/* do a probe without filler */
	(void) studyit();	/* figure out what it all means */

	/* now build thread manager data structures */
	thread_queue_init( &readylist );
	mem_to_free = (void *)0;
	current = thread_null;
}

void thread_manager_start()
/* call this once after launching at least one thread */
/* this only returns when all threads are blocked! */
{
	current = thread_dequeue( &readylist );
	if (current == thread_null) {
		/* crisis */
		fprintf(stderr, "Thread manager start failure, no threads!\n");
		exit(-1);
	}
	if (_setjmp( thread_death )) {
		/* comes here when _longjmp( thread_death, 1 ) done */
		fprintf(stderr,
			"Thread manager terminated, %s!\n", thread_error );
		if (mem_to_free != (void *)0 ) {
			/* see comments below */
			free( mem_to_free );
		}
	} else {
		/* we will come back here whenever we need to deallocate */
		(void) _setjmp( go_free_it );

		if (mem_to_free != (void *)0 ) {
			/* it's not safe to call free() anywhere but on the
			   real stack, so once the thread manager is running,
			   this is the only safe place to call it!
		        */
			free( mem_to_free );
			mem_to_free = (void *)0;
		}

		_longjmp( current->state, 1 );
	}
}

void thread_relinquish()
/* call this within a thread to allow scheduling of a new thread */
{
	if (_setjmp( current->state ) == 0) {
		thread_enqueue( current, &readylist );
		current = thread_dequeue( &readylist );
		_longjmp( current->state, 1 );
	}
}

void thread_exit()
/* call this within a thread to terminate that thread */
{
	/* we can't deallocate, so we schedule this thread for deallocation */
	mem_to_free = (void *)current;

	/* now, get the next thread to run */
	current = thread_dequeue( &readylist );
	if (current == thread_null) {
		/* crisis */
		thread_error = "ready list empty";
		_longjmp( thread_death, 1 );
	}

	_longjmp( go_free_it, 1 );
}

void thread_launch( int size, void (* proc)(int), int param )
/* call this to launch proc(param) as a thread */
/* may be called from main or from any thread */
{
	struct thread * t;
	t = (struct thread *)malloc( sizeof(struct thread) + size );
	t->size = size;
	t->proc = proc;
	t->param = param;
	if (_setjmp( t->state )) {
		/* comes here only when new thread scheduled first time */
		(*current->proc)(current->param);
		thread_exit();
	}
	/* continue initialization */
	{
		long int * s;
		long int * probe_rec;
		long int local_base = (long int)&t; /* address of local t */
		long int new_base;
		int i;

		/* the following code copies the contents of the
		   activation record, just in case it might prove
		   useful; note that we don't know that this is
		   needed on any machine, but it might, so we do it. */

		if (stack_direction == 1) { /* grows up */
			long int * src;
			long int * dst;
			new_base = (long int)t + sizeof( struct thread )
			 	+ local_offset;
			src = (long int *) (local_base - local_offset);
			dst = (long int *) (new_base - local_offset);
			for (i = 0; i <= local_offset; i += sizeof(long int)) {
				*dst++ = *src++;
			}
		} else { /* grows down */
			long int * src;
			long int * dst;
			new_base = (long int)t + sizeof( struct thread )
			 	+ size - (local_offset + sizeof(long int));
			src = (long int *) (local_base);
			dst = (long int *) (new_base);
			for (i = 0; i <= local_offset; i += sizeof(long int)) {
				*dst++ = *src++;
			}
		}

		/* the following code adjusts the references to the
		   activation record in the saved thread state so that
		   they point to the base of the newly allocated stack */

		s = (long int *)(t->state);
		probe_rec = (long int *)probe_record;
		for (i = 0; i < sizeof(jmp_buf); i += sizeof(long int) ) {
			if (*probe_rec != 0) {
				/* adjust this field of state */
				*s += new_base - local_base;
			}
			s++;
			probe_rec++;
		}
	}
	thread_enqueue( t, &readylist );
}

void thread_free( void * ptr )
/* call this from within threads instead of free, because some
   heap managers are paranoid enough to forbid a call to free
   when the stack itself is in a block allocated in the heap */
{
	mem_to_free = ptr;
	if (_setjmp( current->state ) == 0) {
		_longjmp( go_free_it, 1 );
	}
	/* after call to free(), there will be a longjmp back to here */
}

void thread_safety_check()
/* call this to check for thread stack overflow */
{
	long int t = (long int)current + sizeof(struct thread);
	if ( ((long int)&t < t) || ((long int)&t > (t + current->size)) ) {
		/* crisis */
		thread_error = "thread stack overflow";
		_longjmp( thread_death, 1 );
	}
}

/****                                      *
 * thread manager extension for semaphores *
 *                                      ****/


/* semaphore representation known only to semaphore methods */
struct thread_semaphore {
	int count;
        struct thread_queue queue;
};

/* methods applying to semaphores */
void thread_semaphore_init( struct thread_semaphore * s, int v )
/* call this to initialize semaphore s to a count of v */
{
	s->count = v;
        thread_queue_init( &s->queue );
}

void thread_wait( struct thread_semaphore * s )
/* call this within a thread to block on a semaphore */
{
	if (s->count > 0) {
		s->count--;
        } else {
		if (_setjmp( current->state ) == 0) {
			thread_enqueue( current, &s->queue );
			current = thread_dequeue( &readylist );
			if (current == thread_null) {
				/* crisis */
				thread_error = "possible deadlock";

				_longjmp( thread_death, 1 );
			}
			_longjmp( current->state, 1 );
		}
	}
}

void thread_signal( struct thread_semaphore * s )
/* call this within a thread to signal a semaphore */
{
	struct thread * t = thread_dequeue( &s->queue );
	if (t == thread_null) {
		s->count++;
	} else {
		thread_enqueue( t, &readylist );
	}
}