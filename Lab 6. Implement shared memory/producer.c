#define   __LIBRARY__
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

_syscall2(long, sem_open, const char *, name, unsigned int, value);
_syscall1(int, sem_wait, sem_t *, sem);
_syscall1(int, sem_post, sem_t *, sem);
_syscall1(int, sem_unlink, const char *, name);

_syscall2(int, shmget, unsigned int, key, size_t, size);
_syscall1(long, shmat, int, shmid);

/* Use a wrapper like glibc to handle different types (long and sem_t *). */
sem_t * sem_open_wrapper(const char *name, unsigned int value)
{
    long returnValue;
    if ((returnValue = sem_open(name, value)) == -1)
    {
        return SEM_FAILED;
    }
    return (sem_t *)returnValue;
}

#define BUFFERSIZE 10
#define MAXDATA (unsigned) 512
#define KEY 8

int main(void)
{
    /* C89 :( */
    sem_t *bufferMutex, *empty, *full /*, *printMutex*/;
    
    int shmid = -1;
    long *p = (long *)NULL;
    int outputFd = -1;
    
    unsigned int producerPos = 0;
	
    unsigned int producerPID = getpid();

    unsigned long item = 0;

    if ((bufferMutex=sem_open_wrapper("bufferMutex", 1)) == SEM_FAILED)
    {
        perror("opening bufferMutex semaphore");
        return EXIT_FAILURE;
    }
    /*     if ((printMutex=sem_open_wrapper("printMutex", O_CREAT|O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        perror("opening printMutex semaphore");
        return EXIT_FAILURE;
    } */
    if ((empty=sem_open_wrapper("empty", BUFFERSIZE)) == SEM_FAILED)
    {
        perror("opening empty semaphore");
        return EXIT_FAILURE;
    }
    if ((full=sem_open_wrapper("full", 0)) == SEM_FAILED)
    {
        perror("opening full semaphore");
        return EXIT_FAILURE;
    }
    
    if ((outputFd = open("./output.log", O_WRONLY|O_CREAT|O_APPEND, 0666)) == -1)
    {
        perror("open ./output.log");
        return EXIT_FAILURE;
    }
    if (dup2(outputFd, 1) == -1)
    {
        perror("producer dup2(outputFd, 1)");
        return EXIT_FAILURE;
    }
    
    if ((shmid = shmget(KEY, sizeof(long) * BUFFERSIZE)) == -1)
    {
        perror("shmget of consumer");
        return EXIT_FAILURE;
    }
    if ((p = (long *)shmat(shmid)) == (long *)-1)
    {
        perror("shmat of consumer");
        return EXIT_FAILURE;
    }

    for (item=0; item<MAXDATA; ++item)
    {
        sem_wait(empty);
        sem_wait(bufferMutex);
        
        p[producerPos % BUFFERSIZE] = item;

        /* sem_wait(printMutex); */
        printf("Producer %u: produced %d at position %u\n", producerPID,\
		item, producerPos);
        fflush(stdout);
        /* sem_post(printMutex); */
        
        ++producerPos;

        sem_post(bufferMutex);
        sem_post(full);
    }

    close(outputFd);
    /* sem_unlink("bufferMutex"); */
    /* sem_unlink("printMutex"); */
    /* sem_unlink("empty"); */
    /* sem_unlink("full"); */

    printf("Producer %u: Done!\n", producerPID);
    fflush(stdout);
    
    return EXIT_SUCCESS;
}
 
