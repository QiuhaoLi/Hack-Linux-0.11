#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <stdarg.h>

#define PSINFO 0
#define HDINFO 1

static char psinfoBuf[1024] = {'\0'};
static char hdinfoBuf[1024] = {'\0'};
static long psinfoPid = -1;
static long hdinfoPid = -1;

extern int vsprintf();
int sprintf(char *buf, const char *fmt, ...)
{
    va_list args; int i;
    va_start(args, fmt);
    i=vsprintf(buf, fmt, args);
    va_end(args);
    return i;
}

int refresh_psinfo(void)
{
    unsigned int write = sprintf(psinfoBuf, "%s", \
    "pid\tstate\tfather\tcounter\tstart_time\n");

    struct task_struct ** p;
    for(p = &FIRST_TASK+1 ; p <= &LAST_TASK ; ++p)
        if (*p)
            write += sprintf(psinfoBuf+write, "%ld\t%ld\t%ld\t%ld\t%ld\n", \
            (*p)->pid, (*p)->state, (*p)->father, (*p)->counter, (*p)->start_time);
    psinfoBuf[write] = '\0';
    return write;
}

/* From linux/fs/super.c, though copying code is evil :)*/
/* set_bit uses setb, as gas doesn't recognize setc */
#define set_bit(bitnr,addr) ({ \
register int __res ; \
__asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
__res; })

int refresh_hdinfo(void)
{
    unsigned int write = 0;
    int i, freeBlocks = 0, freeInodes = 0;
    struct super_block *p = NULL;
    if ((p = get_super(ROOT_DEV)) != NULL)
    {
        for (i=p->s_nzones-1; i >= 0; --i)
            if (!set_bit(i&8191,p->s_zmap[i>>13]->b_data))
			    freeBlocks++;
		for (i=p->s_ninodes; i >= 0; --i)
            if (!set_bit(i&8191,p->s_imap[i>>13]->b_data))
			    freeInodes++;
        write += sprintf(hdinfoBuf+write, \
        "Free blocks: %d/%d\nFree inodes: %d/%d\n", freeBlocks, p->s_nzones, \
        freeInodes, p->s_ninodes);
    }
    hdinfoBuf[write] = '\0';
    return write;
}

int proc_read(int dev, unsigned long * pos, char * buf, int count)
{
	unsigned int read = 0;

    if (dev == PSINFO)
    {
        /* Wait until psinfoPid finish reading */
        /* It's better to use lock or semaphores with timeout */
        if ((psinfoPid != -1) && (current->pid != psinfoPid))
            return -EINVAL;
        if (*pos == 0)
            refresh_psinfo();
        for (; count > 0; --count, ++read, ++*pos)
        {
            if (psinfoBuf[*pos])
                put_fs_byte(psinfoBuf[*pos], (buf + read));
            else
            {
                psinfoPid = -1;
                break;
            }
        }
    }
    else if (dev == HDINFO)
    {
        if ((hdinfoPid != -1) && (current->pid != hdinfoPid))
            return -EINVAL;
        if (*pos == 0)
            refresh_hdinfo();
        for (; count > 0; --count, ++read, ++*pos)
        {
            if (hdinfoBuf[*pos])
                put_fs_byte(hdinfoBuf[*pos], (buf + read));
            else
            {
                hdinfoPid = -1;
                break;
            }
        }
    }
    
	return read;
}
