## Objectives

**In this lab, you need to implement a new system call which outputs the paging status, like content of page directory and page tables.**

*Intel describes paging in the [Intel Manual Volume 3 System Programming Guide - 325384-056US September 2015](https://web.archive.org/web/20151025081259/http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-system-programming-manual-325384.pdf) Chapter 4 "Paging".*

<br />

For example, you can create a system call named `dabug_paging` :

```c
int debug_paging(pid_t pid, void *address, const char *logPath);
```

If the `logPath` argument is *NULL*, `debug_paging` will output to `stdout`. Otherwise, it will output to the file pointed by `logPath`. If the `address` argument is *NULL*, `debug_paging` will output all page tables’ addresses and page frames’ addresses used by the process (Present bit = 1). Otherwise it will only output the paging process of address.

On success, 0 is returned.  On error, -1 is returned,  and `errno` is set to indicate the error.

<br />

**Notice:**

To make it easier to write the C string pointed by format to a terminal or file in the kernel, you can implement your own `fprintk` in `/kernel/printk.c` : 

```c
static char logbuf[1024];
int fprintk(int fd, const char *fmt, ...)
{
    va_list args;
    int count;
    struct file * file;
    struct m_inode * inode;

    va_start(args, fmt);
    count=vsprintf(logbuf, fmt, args);
    va_end(args);

    if (fd < 3)
    {
        __asm__("push %%fs\n\t"
            "push %%ds\n\t"
            "pop %%fs\n\t"
            "pushl %0\n\t"
            "pushl $logbuf\n\t"
            "pushl %1\n\t"
            "call sys_write\n\t"
            "addl $8,%%esp\n\t"
            "popl %0\n\t"
            "pop %%fs"
            ::"r" (count),"r" (fd):"ax","cx","dx");
    }
    else
    {
        if (!(file=current->filp[fd]))
            return 0;
        inode=file->f_inode;

        __asm__("push %%fs\n\t"
            "push %%ds\n\t"
            "pop %%fs\n\t"
            "pushl %0\n\t"
            "pushl $logbuf\n\t"
            "pushl %1\n\t"
            "pushl %2\n\t"
            "call file_write\n\t"
            "addl $12,%%esp\n\t"
            "popl %0\n\t"
            "pop %%fs"
            ::"r" (count),"r" (file),"r" (inode):"ax","cx","dx");
    }
    return count;
}

```

And don’t forget to add a statement in `/include/linux/kernel.h` :

```c
int fprintk(int fd, const char *fmt, ...);
```

## Demo

If we run following  `test.c` to test our implementation of `debug_paging` :

```c
/* test.c */
#define   __LIBRARY__
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

_syscall3(int, debug_paging, pid_t, pid, void *, address, const char *, logPath);

int i = 0x12345678;

int main(void)
{
    printf("The logical/virtual address of i is 0x%08x\n", &i);
    fflush(stdout);
    debug_paging(getpid(), &i, NULL);
    debug_paging(getpid(), NULL, "/usr/var/paging.log");
    return 0;
}

```

Then the output may be like:

![demo1](./img/demo1.gif)

<br />

If we write following code after `p = shmat(shmid)` in `producer.c` :

```c
debug_paging(getpid(), p, "/usr/var/paging.log");
debug_paging(getpid(), NULL, "/usr/var/paging.log");
```

and following code after `p = shmat(shmid)` in `consumer.c` :

```c
debug_paging(getpid(), NULL, "/usr/var/paging.log");
```

to observe the shared memory page of consumer and producer, then the output will be like:

![demo1](./img/demo2.gif)