## Objectives

**Tracking address translation and record it briefly:**

`test.c` is a program that has an infinite loop:

```c
#include <stdio.h>

int i = 0x12345678;

int main(void)
{
    printf("The logical/virtual address of i is 0x%08x\n", &i);
    fflush(stdout);
    while (i);
    return 0;
}

```

First, compile `test.c` under Linux 0.11 and start bochs in debug mode. Then, stop the control flow at the while loop, using the debugger tools to observe the segment registers, control registers, and analyze the address translation process of variable i. Finally, break the while loop by directly setting the physical memory of i to zero.

<br />

**Implement a simplified version of the `shmget` and `shmat` system calls in `mm/shm.c` for memory page sharing:**

```c
int shmget(key_t key, size_t size);
```

`shmget` returns the identifier of the shared memory page associated with the value of the argument key.  It may be used either to obtain the identifier of a previously created shared memory segment, or to create a new shared page (with size equal to the value of size rounded up to `PAGE_SIZE`).

On success, a valid shared memory identifier is returned.  On error, -1 is returned, and `errno` is set to indicate the error.

```c
void *shmat(int shmid);
```

`shmat` attaches the shared memory page identified by `shmid` to the address space of the calling process.

On success `shmat` returns the address of the attached shared memory segment; on error *(void \*) -1* is returned, and `errno` is set to indicate the cause of the error. 

**Notice:**

When process exits, the kernel will release the physical memory pages it occupies. Pay attention to whether other processes are using one of these pages as a shared page.

<br />

**Modify the producer-consumer program `pc.c` we wrote in *5. Implement semaphores*:**

1. Instead of using a file as buffer, use the shared memory via `shmget` and `shmat`；

2. Split pc.c into two single-process programs `producer.c` and `consumer.c`;

<br />

**Run `producer.c` and `consumer.c` to verify your implementation of shared memory.**

## Demo

output.log

```
Producer 7: produced 0 at position 0
Producer 7: produced 1 at position 1
Producer 7: produced 2 at position 2
Producer 7: produced 3 at position 3
Producer 7: produced 4 at position 4
Producer 7: produced 5 at position 5
Producer 7: produced 6 at position 6
Producer 7: produced 7 at position 7
Producer 7: produced 8 at position 8
Producer 7: produced 9 at position 9
Consumer 9: consumed 0 at position 0
Consumer 9: consumed 1 at position 1
Consumer 9: consumed 2 at position 2
Consumer 9: consumed 3 at position 3
Consumer 9: consumed 4 at position 4
Consumer 9: consumed 5 at position 5
Consumer 9: consumed 6 at position 6
Consumer 9: consumed 7 at position 7
Consumer 9: consumed 8 at position 8
Consumer 9: consumed 9 at position 9
Producer 7: produced 10 at position 10
Producer 7: produced 11 at position 11
Producer 7: produced 12 at position 12
Producer 7: produced 13 at position 13
...
Consumer 9: consumed 493 at position 493
Consumer 9: consumed 494 at position 494
Consumer 9: consumed 495 at position 495
Consumer 9: consumed 496 at position 496
Consumer 9: consumed 497 at position 497
Consumer 9: consumed 498 at position 498
Consumer 9: consumed 499 at position 499
Consumer 9: consumed 500 at position 500
Consumer 9: consumed 501 at position 501
Producer 7: produced 502 at position 502
Producer 7: produced 503 at position 503
Producer 7: produced 504 at position 504
Producer 7: produced 505 at position 505
Producer 7: produced 506 at position 506
Producer 7: produced 507 at position 507
Producer 7: produced 508 at position 508
Producer 7: produced 509 at position 509
Producer 7: produced 510 at position 510
Producer 7: produced 511 at position 511
Producer 7: Done!
Consumer 9: consumed 502 at position 502
Consumer 9: consumed 503 at position 503
Consumer 9: consumed 504 at position 504
Consumer 9: consumed 505 at position 505
Consumer 9: consumed 506 at position 506
Consumer 9: consumed 507 at position 507
Consumer 9: consumed 508 at position 508
Consumer 9: consumed 509 at position 509
Consumer 9: consumed 510 at position 510
Consumer 9: consumed 511 at position 511
Consumer 9: Done!

```

