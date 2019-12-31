## Objectives

**Implement semaphores on Linux 0.11 with the following (system) calls:**

```c
sem_t *sem_open(const char *name, unsigned int value);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_unlink(const char *name);
```

`sem_t` is the semaphore’s type which should be defined in `<semaphore.h>` .

`sem_open` is used to create a semaphore or open an existing semaphore. Different processes can share the same semaphore by providing the same name. Value is the initial value of the semaphore, which is valid only when a new semaphore is created. On success, `sem_open` returns a unique identifier for the semaphore, or -1 if an error occurred (in which case, `errno` is set appropriately). 

`sem_wait` is the semaphore's `P` atom operation. It decrements (locks) the semaphore pointed to by `sem`.  If the semaphore's value is greater than zero, then the decrement proceeds, and the function returns, immediately.  If  the semaphore currently has  the  value  zero,  then  the  call blocks until either it becomes possible to perform the decrement (i.e., the semaphore value rises above zero), or a signal handler interrupts the call. On success, `sem_wait` returns 0. 

`sem_post` is the semaphore's `V` atom operation. It increments (unlocks) the semaphore pointed to by `sem`.  If the semaphore's value consequently becomes greater than zero, then another process or thread blocked in a `sem_wait` call will be woken up and proceed to lock the semaphore.  On success, `sem_post` returns 0. 

`sem_unlink` removes the named semaphore referred to by name. On success `sem_unlink` returns 0; on error, -1 is returned, with `errno` set to indicate the error.

You should implement these system calls in `linux-0.11/kernel/sem.c`.

<br />

**Write a program `pc.c` to simulate the producer-consumer problem and solve the problem with semaphores:**

1. Create a producer process and N consumer processes (N>1);
2. Create a file buffer as the queue ;
3. The producer sequentially writes the integers 0, 1, 2, ..., M to the buffer (M >= 500);
4. The consumer processes read one integer at a time concurrently, and then delete it from the buffer;
5. The buffer can only hold up to 10 integers at the same time.
6.  Process should print its PID and every integer it produced or consumed in real time.

<br />

**Notice:**

In the following file/functions, due to compatibility issues with inline assembly, **some calls may modify the parameters**:

> - **include/linux/sched.h**： set_base，set_limit
> - **include/string.h** ：strcpy， strncpy，strcat，strncat，strcmp，strncmp，strchr， strrchr，strspn，strcspn，strpbrk，strstr，memcpy，memmove，memcmp，memchr，
> - **mm/memory.c**：copy_page，get_free_page
> - **fs/buffer.c**：COPY_BLK
> - **fs/namei.c**：match
> - **fs/bitmap.c**：clear_block，find_first_zero
> - **kernel/blk_drv/floppy.c**：copy_buffer
> - **kernel/blk_drv/hd.c**：port_read，port_write
> - **kernel/chr_drv/console.c**：scrup，scrdown，csi_J，csi_K，con_write

<br />

In this case, you can write your own functions in C instead of inline function/assembly . For example, we can create a `string_fix.h` which implements `strCmp` :

```c
#ifndef _STRING_FIX_H_
#define _STRING_FIX_H_

/*
 * This header file is for fixing bugs caused by inline assembly
 * in string.h.
 */

int strCmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

#endif
```

<br />

Or you can just manually save the modified register like:

```c
extern inline int strcmp(const char * cs,const char * ct)
{
register int __res ;
__asm__("push %%edi\n\tpush %%esi\n\t" // SAVE
    "cld\n\t"
    "1:\tlodsb\n\t"
    "scasb\n\t"
    "jne 2f\n\t"
    "testb %%al,%%al\n\t"
    "jne 1b\n\t"
    "xorl %%eax,%%eax\n\t"
    "jmp 3f\n"
    "2:\tmovl $1,%%eax\n\t"
    "jl 3f\n\t"
    "negl %%eax\n\t"
    "3:\n\t"
    "pop %%esi\n\tpop %%edi\n" // RESTORE
    :"=a" (__res):"D" (cs),"S" (ct));
return __res;
}
```

<br />

For more information about this Bug, you can visit [一个关于内联优化和调用约定的Bug](https://www.cnblogs.com/liqiuhao/p/11728440.html) (Chinese) .

## Demo

output.log

```
13: produced 0 at position 0
13: produced 1 at position 1
13: produced 2 at position 2
13: produced 3 at position 3
13: produced 4 at position 4
13: produced 5 at position 5
13: produced 6 at position 6
13: produced 7 at position 7
13: produced 8 at position 8
13: produced 9 at position 9
17: consumed 0 at position 0
17: consumed 1 at position 1
17: consumed 2 at position 2
17: consumed 3 at position 3
17: consumed 4 at position 4
17: consumed 5 at position 5
17: consumed 6 at position 6
17: consumed 7 at position 7
17: consumed 8 at position 8
17: consumed 9 at position 9
13: produced 10 at position 10
13: produced 11 at position 11
13: produced 12 at position 12
13: produced 13 at position 13
13: produced 14 at position 14
13: produced 15 at position 15
13: produced 16 at position 16
13: produced 17 at position 17
13: produced 18 at position 18
13: produced 19 at position 19
14: consumed 10 at position 10
14: consumed 11 at position 11

...

16: consumed 494 at position 494
16: consumed 495 at position 495
16: consumed 496 at position 496
13: produced 497 at position 497
13: produced 498 at position 498
13: produced 499 at position 499
13: produced 500 at position 500
13: produced 501 at position 501
13: produced 502 at position 502
13: produced 503 at position 503
13: produced 504 at position 504
13: produced 505 at position 505
13: produced 506 at position 506
15: consumed 497 at position 497
15: consumed 498 at position 498
15: consumed 499 at position 499
15: consumed 500 at position 500
15: consumed 501 at position 501
15: consumed 502 at position 502
15: consumed 503 at position 503
15: consumed 504 at position 504
15: consumed 505 at position 505
15: consumed 506 at position 506
13: produced 507 at position 507
13: produced 508 at position 508
13: produced 509 at position 509
13: produced 510 at position 510
13: produced 511 at position 511
16: consumed 507 at position 507
16: consumed 508 at position 508
16: consumed 509 at position 509
16: consumed 510 at position 510
16: consumed 511 at position 511
16: Done!
17: Done!
14: Done!
15: Done!
13: Done!
```

