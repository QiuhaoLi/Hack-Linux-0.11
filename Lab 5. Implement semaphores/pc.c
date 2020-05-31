/* To run this code on a modern GNU/Linux distro,
   compile with 'gcc pc.c -lpthread -DMODERN' */
#define   __LIBRARY__
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
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
sem_t * sem_open_wrapper(const char *name, unsigned int value)
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

#define CHILDNUMBER 4
#define BUFFERSIZE 10
#define MAXDATA (unsigned) 512

int main(void)
{
    /* C89 :( */
    sem_t *bufferMutex, *empty, *full /*, *printMutex*/;
    
    int fd = -1;
    int outputFd = -1;
    
    unsigned int producerPos = 0;
    unsigned int consumerPos = 0;

    unsigned consumerPID;
    unsigned producerPID = getpid();

    unsigned int data;
    unsigned int item = 0;

    int j, k;

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
    
    if ((outputFd = open("./output.log", O_WRONLY|O_CREAT, 0666)) == -1)
    {
        perror("open ./output.log");
        return EXIT_FAILURE;
    }
    if (dup2(outputFd, 1) == -1)
    {
        perror("dup2(outputFd, 1)");
        return EXIT_FAILURE;
    }
    if ((fd = open("./buffer", O_RDWR|O_CREAT, 0666)) == -1)
    {
        perror("open ./buffer");
        return EXIT_FAILURE;
    }

    /* consumers need to share the position to consume */
    if (lseek(fd, BUFFERSIZE*sizeof(item), SEEK_SET) == -1)
    {
        perror("store consumerPos lseek");
        return EXIT_FAILURE;
    }
    if (write(fd, (void *)&consumerPos, sizeof(item)) == -1)
    {
        perror("store consumerPos write");
        return EXIT_FAILURE;
    }

    for (j=0; j<CHILDNUMBER; ++j)
    {
        if (!fork()) /* consumer */
        {
            consumerPID = getpid();
            for (item=0; item<MAXDATA; ++item)
            {
                sem_wait(full);

                sem_wait(bufferMutex);
                if (lseek(fd, BUFFERSIZE*sizeof(item), SEEK_SET) == -1)
                {
                    perror("read consumerPos lseek");
                    return EXIT_FAILURE;
                }
                if (read(fd, (void *)&consumerPos, sizeof(consumerPos)) == -1)
                {
                    perror("read consumerPos read");
                    return EXIT_FAILURE;
                }

                if (consumerPos >= MAXDATA)
                {
                    /* no more data */
                    printf("%u: Done!\n", consumerPID);
                    fflush(stdout);
                    /* don't forget to release buffer */
                    sem_post(bufferMutex);
                    break;
                }

                if (lseek(fd, (consumerPos % BUFFERSIZE)* sizeof(item), SEEK_SET) == -1)
                {
                    perror("consuming lseek");
                    return EXIT_FAILURE;
                }
                if (read(fd, (void *)&data, sizeof(item)) == -1)
                {
                    perror("consuming read");
                    return EXIT_FAILURE;
                }

                /* sem_wait(printMutex); */
                printf("%u: consumed %d at position %u\n", consumerPID, data, consumerPos);
                fflush(stdout);
                /* sem_post(printMutex); */

                ++consumerPos;
                if (lseek(fd, BUFFERSIZE*sizeof(item), SEEK_SET) == -1)
                {
                    perror("store consumerPos lseek");
                    return EXIT_FAILURE;
                }
                if (write(fd, (void *)&consumerPos, sizeof(consumerPos)) == -1)
                {
                    perror("store consumerPos write");
                    return EXIT_FAILURE;
                }

                /* the child who consumed the last item */
                if (consumerPos >= MAXDATA)
                {
                    /* let other children release from sem_wait and return */
                    for (k = 0; k < CHILDNUMBER-1; ++k)
                    {
                        sem_post(full);
                    }
                    printf("%u: Done!\n", consumerPID);
                    fflush(stdout);
                    /* don't forget to release buffer */
                    sem_post(bufferMutex);
                    break;
                }

                sem_post(bufferMutex);
                sem_post(empty);
            }
            close(outputFd);
            close(fd);
            close(outputFd);
            return EXIT_SUCCESS;
        }
    }

    for (item=0; item<MAXDATA; ++item)
    {
        sem_wait(empty);

        sem_wait(bufferMutex);
        if (lseek(fd, (producerPos % BUFFERSIZE) * sizeof(item), SEEK_SET) == -1)
        {
            perror("producer lseek");
            return EXIT_FAILURE;
        }
        if (write(fd, (void *)&item, sizeof(item)) == -1)
        {
            perror("producing");
            return EXIT_FAILURE;
        }

        /* sem_wait(printMutex); */
        printf("%u: produced %d at position %u\n", producerPID, item, producerPos);
        fflush(stdout);
        /* sem_post(printMutex); */
        
        ++producerPos;

        sem_post(bufferMutex);
        sem_post(full);
    }

    while ((j = waitpid(-1, NULL, 0)))
	{
		if (j == -1)
		{
			if (errno == ECHILD)
			{
				break;
			}
			else
			{
				perror("producer waits for consumers");
				return EXIT_FAILURE;
			}
			
		}      
	}

    close(fd);
    close(outputFd);
    sem_unlink("bufferMutex");
    /* sem_unlink("printMutex"); */
    sem_unlink("empty");
    sem_unlink("full");

    printf("%u: Done!\n", producerPID);
    fflush(stdout);
    
    return EXIT_SUCCESS;
}
