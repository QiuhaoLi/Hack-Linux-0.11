#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include <linux/sched.h>

#define MAXSEMNAME 128
struct sem_t
{
    char m_name[MAXSEMNAME+1];
    unsigned long m_value;

	struct sem_t * m_prev;
	struct sem_t * m_next;

    struct task_struct * m_wait;
};

typedef struct sem_t sem_t;

#define SEM_FAILED ((sem_t *)0)

#endif
