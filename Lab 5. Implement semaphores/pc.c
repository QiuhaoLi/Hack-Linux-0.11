/* This code can be compiled on Linux 0.11.
   To run this code on a modern GNU/Linux distro,
   compile with 'gcc -pthread -DMODERN pc.c' 
   Change log: https://github.com/QiuhaoLi/Hack-Linux-0.11
   */

#define __LIBRARY__
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> /* For O_* constants */
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#ifndef MODERN
_syscall2(long, sem_open, const char *, name, unsigned int, value);
_syscall1(int, sem_wait, sem_t *, sem);
_syscall1(int, sem_post, sem_t *, sem);
_syscall1(int, sem_unlink, const char *, name);

/* Use a wrapper like glibc to handle different types (long and sem_t *). */
sem_t *sem_open_wrapper(const char *name, unsigned int value)
{
    long returnValue;
    if ((returnValue = sem_open(name, value)) == -1)
    {
        return SEM_FAILED;
    }
    return (sem_t *)returnValue;
}

#else
#define sem_open_wrapper(name, value) sem_open(name, O_CREAT, 0666, value)
#endif

#define CHILD_NUMBER ((unsigned)4)
#define BUFFER_SIZE ((unsigned)10)
#define DATA_SIZE ((unsigned)4) /* 4 bytes - 32 bits GNU/Linux int size */
#define MAX_DATA_NUMBER ((unsigned)512)
#define INIT_POS ((unsigned)0) /* Place data from the beginning */

sem_t *bufferMutex, *empty, *full /*, *printMutex*/;
/* Well.. It turned out that we don't really need a Mutex for stdout */
int bufferFd = -1;
int outputFd = -1;
/* Init all semaphore and buffer files */
int prelude(void);
/* Clean up all semaphore and files for main */
int main_cleanup(void);
int consumer(void);

int main(void)
{
    /* C89 :( */
    unsigned producerPID = getpid();
    unsigned int producerPos = INIT_POS;
    unsigned long data;
    int i;

    if (prelude() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    for (i = 0; i < CHILD_NUMBER; ++i)
    {
        if (!fork()) /* spawn our consumers */
            return consumer();
    }

    /* Producer begins */
    /* For this toy lab, produces data as 0, 1, 2, ... MAX_DATA_NUMBER */
    for (data = 0; data < MAX_DATA_NUMBER; ++data)
    {
        sem_wait(empty);

        sem_wait(bufferMutex);
        if (lseek(bufferFd, (producerPos % BUFFER_SIZE) * DATA_SIZE, SEEK_SET) == -1)
        {
            perror("producer lseek");
            return EXIT_FAILURE;
        }
        if (write(bufferFd, (void *)&data, DATA_SIZE) == -1)
        {
            perror("producing");
            return EXIT_FAILURE;
        }

        /* sem_wait(printMutex); */
        printf("%u: produced %lu at position %u\n", producerPID, data, producerPos);
        fflush(stdout);
        /* sem_post(printMutex); */

        ++producerPos;

        sem_post(bufferMutex);
        sem_post(full);
    }
    printf("Producer %u: Finished!\n", producerPID);
    fflush(stdout);
    /* Producer ends */

    return main_cleanup();
}

int consumer(void)
{
    unsigned int consumerPID = getpid();
    unsigned int consumerPos = 0;
    unsigned long data;
    int i, j;

    /* A consumer can never consume more than MAX_DATA_NUMBER items */
    for (i = 0; i < MAX_DATA_NUMBER; ++i)
    {
        sem_wait(full);

        sem_wait(bufferMutex);
        if (lseek(bufferFd, BUFFER_SIZE * DATA_SIZE, SEEK_SET) == -1)
        {
            perror("read consumerPos lseek");
            return EXIT_FAILURE;
        }
        if (read(bufferFd, (void *)&consumerPos, DATA_SIZE) == -1)
        {
            perror("read consumerPos read");
            return EXIT_FAILURE;
        }

        if (consumerPos >= MAX_DATA_NUMBER)
        {
            /* no more data */
            printf("Consumer %u: Finished!\n", consumerPID);
            fflush(stdout);
            /* Note: don't forget to release the buffer */
            sem_post(bufferMutex);
            break;
        }

        if (lseek(bufferFd, (consumerPos % BUFFER_SIZE) * DATA_SIZE, SEEK_SET) == -1)
        {
            perror("consuming lseek");
            return EXIT_FAILURE;
        }
        if (read(bufferFd, (void *)&data, DATA_SIZE) == -1)
        {
            perror("consuming read");
            return EXIT_FAILURE;
        }

        /* sem_wait(printMutex); */
        printf("%u: consumed %ld at position %u\n", consumerPID, data, consumerPos);
        fflush(stdout);
        /* sem_post(printMutex); */

        ++consumerPos;
        if (lseek(bufferFd, BUFFER_SIZE * DATA_SIZE, SEEK_SET) == -1)
        {
            perror("store consumerPos lseek");
            return EXIT_FAILURE;
        }
        if (write(bufferFd, (void *)&consumerPos, sizeof(consumerPos)) == -1)
        {
            perror("store consumerPos write");
            return EXIT_FAILURE;
        }

        /* the child who consumed the last item */
        if (consumerPos >= MAX_DATA_NUMBER)
        {
            /* let other children release from sem_wait and return */
            for (j = 0; j < CHILD_NUMBER - 1; ++j)
            {
                sem_post(full);
            }
            printf("Consumer %u: Finished!\n", consumerPID);
            fflush(stdout);
            /* don't forget to release buffer */
            sem_post(bufferMutex);
            break;
        }

        sem_post(bufferMutex);
        sem_post(empty);
    }

    close(outputFd);
    close(bufferFd);
    return EXIT_SUCCESS;
}

/* ----------------------- You can just stop here :) ----------------------- */

/* Init all semaphore and buffer files */
int prelude(void)
{
    /* For this toy lab, let's just use a long var to receive our data item */
    unsigned long data;
    unsigned int initConsumerPos = INIT_POS;

    if (sizeof(data) < DATA_SIZE)
    {
        perror("DATA_SIZE is too big to fit in a long type.");
        return EXIT_FAILURE;
    }

    if ((bufferMutex = sem_open_wrapper("bufferMutex", 1)) == SEM_FAILED)
    {
        perror("opening bufferMutex semaphore");
        return EXIT_FAILURE;
    }
    /*     if ((printMutex=sem_open_wrapper("printMutex", O_CREAT|O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        perror("opening printMutex semaphore");
        return EXIT_FAILURE;
    } */
    if ((empty = sem_open_wrapper("empty", BUFFER_SIZE)) == SEM_FAILED)
    {
        perror("opening empty semaphore");
        return EXIT_FAILURE;
    }
    if ((full = sem_open_wrapper("full", 0)) == SEM_FAILED)
    {
        perror("opening full semaphore");
        return EXIT_FAILURE;
    }

    if ((outputFd = open("./output.log", O_WRONLY | O_CREAT, 0666)) == -1)
    {
        perror("open ./output.log");
        return EXIT_FAILURE;
    }
    if (dup2(outputFd, 1) == -1)
    {
        perror("dup2(outputFd, 1)");
        return EXIT_FAILURE;
    }
    if ((bufferFd = open("./buffer", O_RDWR | O_CREAT, 0666)) == -1)
    {
        perror("open ./buffer");
        return EXIT_FAILURE;
    }

    /* consumers need to share the position to consume,
       and we use the last+1 data item to store it.
    */
    if (lseek(bufferFd, BUFFER_SIZE * DATA_SIZE, SEEK_SET) == -1)
    {
        perror("store initConsumerPos lseek");
        return EXIT_FAILURE;
    }
    if (write(bufferFd, (void *)&initConsumerPos, DATA_SIZE) == -1)
    {
        perror("store initConsumerPos write");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main_cleanup(void)
{
    unsigned int i = 0;
    while ((i = waitpid(-1, NULL, 0)))
    {
        if (i == -1)
        {
            if (errno == ECHILD)
                break;
            else
            {
                perror("Claiming consumers...");
                return EXIT_FAILURE;
            }
        }
    }

    close(bufferFd);
    close(outputFd);
    sem_unlink("bufferMutex");
    /* sem_unlink("printMutex"); */
    sem_unlink("empty");
    sem_unlink("full");
    return EXIT_SUCCESS;
}
