#include <semaphore.h>
#include <errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>  
#include <asm/segment.h>  
#include <asm/system.h>
#include <string.h>

/*
 * Due to compatibility issues with inline assembly, some calls like
 * strcmp may modify the parameters value. So I just implement a new
 * strCmp in string_fix.h.
 */
#include <string_fix.h>


// Data structure optimization is possible here
sem_t _semHead={.m_name = "_semHead", .m_value = 0, .m_prev = NULL,\
                 .m_next = NULL, .m_wait = NULL};

sem_t *find_sem(const char* name)
{
    sem_t *tmpSemP = &_semHead;
    while (tmpSemP->m_next != NULL)
    {
        if (strCmp((tmpSemP->m_name), name) == 0)
        {
            return tmpSemP;
        }
        tmpSemP = tmpSemP->m_next;
    }
    return tmpSemP;
}

// The input should be checked for security and I did not implement it

sem_t *sys_sem_open(const char* name,unsigned int value)
{
	char tmpName[MAXSEMNAME+1] = {0};

    /* read name from user space */
    char c;
    int i;
    for (i = 0; i < MAXSEMNAME+1; i++)
    {
        c = get_fs_byte(name + i);
        if (c == '\0')
        {
            break;
        }
        tmpName[i] = c;
    }

    /* detect error for overlong name */
    if (c != '\0')
    {
		printk("Name length exceeds limit %u\n", MAXSEMNAME);
        return (sem_t *)-EINVAL;
    }

    if (strCmp("_semHead", tmpName) == 0)
    {
        printk("Can't create _semHead\n");
        return (sem_t *)-EINVAL;
    }

    sem_t *tmpSemP = find_sem(tmpName);
    if (strCmp(tmpSemP->m_name, tmpName) == 0)
    {
        // already exists
        return tmpSemP;
    }

    sem_t *newSemP = NULL;
    if ((newSemP = malloc(sizeof(sem_t))) == NULL)
    {
        printk("Failed to malloc memory space for new semaphore\n");
        return (sem_t *)-ENOSPC;
    }
    strncpy(newSemP->m_name, tmpName, strlen(tmpName));
    newSemP->m_value = value;
    newSemP->m_prev = tmpSemP;
    newSemP->m_next = NULL;
    newSemP->m_wait = NULL;

    tmpSemP->m_next = newSemP;
    return newSemP;
}  

int sys_sem_wait(sem_t *sem)
{
	cli();
	while((sem->m_value) == 0)
	{
		sleep_on(&sem->m_wait);
	}
	(sem->m_value)--;
	sti();
	return 0;
}

int sys_sem_post(sem_t *sem)
{
	cli();
	(sem->m_value)++;
	wake_up(&sem->m_wait);
	sti();
	return 0;
}

int sys_sem_unlink(const char *name)  
{

	char tmpName[MAXSEMNAME+1] = {0};

    /* read name from user space */
    char c;
    int i;
    for (i = 0; i < MAXSEMNAME+1; i++)
    {
        c = get_fs_byte(name + i);
        if (c == '\0')
        {
            break;
        }
        tmpName[i] = c;
    }

    /* detect error for overlong name */
    if (c != '\0')
    {
		printk("Name length exceeds limit %u\n", MAXSEMNAME);
        return -EINVAL;
    }

    if (strCmp("_semHead", tmpName) == 0)
    {
        printk("Can't realse _semHead\n");
        return -EINVAL;
    }
    sem_t *tmpSemP = find_sem(tmpName);
    
    if (strCmp(tmpSemP->m_name, tmpName) != 0)
    {
        // doesn't exist
        printk("This semaphore doesn't exist\n");
        return -EINVAL;
    }

    // delete semaphore from linked list
    (tmpSemP->m_prev)->m_next = tmpSemP->m_next;
    if (tmpSemP->m_next != NULL)
    {
        (tmpSemP->m_next)->m_prev = tmpSemP->m_prev;
    }

    // free semaphore struct
    free_s(tmpSemP, sizeof(sem_t));
    return 0;  
}  
