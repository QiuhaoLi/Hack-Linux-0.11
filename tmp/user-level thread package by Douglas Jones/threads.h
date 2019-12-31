/****
 * file threads.h
 *
   Header files for the user-level thread package  
   Written by Douglas Jones,    Feb 18, 1998
   Added thread_free interface, Feb 19, 2002

   Copyright Douglas Jones, 1998.

   Permission is hereby granted to make and modify
   personal copies of this code for noncommercial or
   educational use.
                                                     *
   All other rights are reserved!                    *
                                                  ****/


/****                      *
 * Initialization routines *
 *                      ****/

void thread_manager_init();
/* call this once before launching any threads */

void thread_launch( int size, void (* proc)(int), int param );
/* call this to launch proc(param) as a thread */
/* may be called from main or from any thread */

void thread_manager_start();
/* call this once after launching at least one thread */
/* this only returns when all threads are blocked! */



/****                                    *
 * Routines to call from within a thread *
 *                                    ****/

void thread_relinquish();
/* call this within a thread to allow scheduling of a new thread */

void thread_exit();
/* call this within a thread to terminate that thread */

void thread_free( void * ptr );
/* call this within a thread instead of calling free() */

/* note, thread_launch() may also be called within a thread */

/****                             *
 * Utility and debugging routines *
 *                             ****/

void thread_startup_report();
/* reports on the results of the study done in the startup */

void thread_safety_check();
/* call this to check for thread stack overflow */


/****                                    *
 * Semaphore extension to thread package *
 *                                    ****/

/* the declaration of the semaphore type given here is a lie! */
struct thread_semaphore {
	void * unused_fields[3];
};

void thread_semaphore_init( struct thread_semaphore * s, int v );
/* call this to initialize a semaphore */

void thread_wait( struct thread_semaphore * s );
/* call this within a thread to block on a semaphore */

void thread_signal( struct thread_semaphore * s );
/* call this within a thread to signal a semaphore */