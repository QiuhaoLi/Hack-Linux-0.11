#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h> /* DEBUG */

#define TEST_MEM_SIZE_PER_THREAD 0x40000   /* 256KB per thread */

#define MAX_NUM_THREADS	16
#define MIN_NUM_THREADS (0x100000/TEST_MEM_SIZE_PER_THREAD) /* 1MB+ */

#define MAX_NUM_TESTS	5
#define MIN_NUM_TESTS	1

#define TEST_NUM_CHARS 5

struct worker
{
    pthread_t tid;
    char *startAddress;
    char *workAddress;
};
typedef struct worker worker;

worker threads[MAX_NUM_THREADS];
unsigned int testTimes  = MIN_NUM_TESTS;
unsigned int threadsNumber = MIN_NUM_THREADS;

/* worker routine */
void *mem_test_work(void *threadsIndexP);

/* start all workers */
void go(void);

/* print all workers' status */
void status(void);

/* cancel all worker */
void my_abort(void);

/* abort and exit */
void my_exit(void) __attribute__ ((noreturn));

int main (int argc, const char *argv[])
{
    /* read user instructions */
    unsigned int instruction = 0;
    while (1)
    {
        printf("Instructions:\n\
        testTimes(1), threadsNumber(2), go(3), status(4), abort(5), exit(6)\n> ");
        scanf("%u", &instruction);
        switch (instruction)
        {
            case 1:
                printf(">>> ");
                scanf("%u", &testTimes);
                break;
            case 2:
                printf(">>> ");
                scanf("%u", &threadsNumber);
                break;
            case 3:
                go();
                break;
            case 4:
                status();
                break;
            case 5: 
                my_abort();
                break;
            case 6:
                my_exit();
                break;
            default:
                printf("Illegal instruction\n");
                continue;
        }

    }
    
    printf("ASSERT\n");
    pthread_exit((void*) -1);
}

void *mem_test_work(void *threadsIndexP)
{
    testTimes = testTimes > MAX_NUM_TESTS ? MAX_NUM_TESTS : testTimes;
    unsigned int index = *((unsigned int *) threadsIndexP); free(threadsIndexP);
    char * endAdress = threads[index].startAddress + TEST_MEM_SIZE_PER_THREAD;
    volatile char chs[TEST_NUM_CHARS] = {'\0', 0x55, 0xAA, 0xFF, (char)rand()};
    volatile char ch = '\0';
    unsigned int i, j;
    while (threads[index].workAddress <= endAdress)
    {
        usleep(5); /* DEBUG */
        for (i = 0; i < testTimes ; i++)
        {
            for ( j = 0; j < TEST_NUM_CHARS; j++)
            {
                *threads[index].workAddress = chs[j];
                ch = *threads[index].workAddress;
                if (ch != chs[j])
                {
                    printf("Thread: %lu test failed at %p: wrote %#02x, read %#02x\n", \
                    threads[i].tid, threads[i].workAddress, chs[j], ch);
                    pthread_exit((void *) -1);
                }
            }
        }
        
        ++threads[index].workAddress;
    }
    /* return (void *)123; DEBUG */
    pthread_exit(NULL);
}

/* start all workers */
void go(void)
{
    threadsNumber = threadsNumber > MAX_NUM_THREADS ? MAX_NUM_THREADS : threadsNumber;
    threadsNumber = threadsNumber < MIN_NUM_THREADS ? MIN_NUM_THREADS : threadsNumber;
    pthread_attr_t attr;
    void *startAddress = NULL;
    unsigned int *threadsIndexP = NULL;
    unsigned int i;
    int rc;

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);

    for (i = 0; i < threadsNumber; i++)
    {
        if ( (startAddress = malloc(TEST_MEM_SIZE_PER_THREAD)) == NULL)
        {
            printf("ERROR; malloc test memory for thread index %u\n", i);
            pthread_exit((void*) -1);
        }

        threads[i].tid = 0;
        threads[i].startAddress = startAddress;
        threads[i].workAddress = startAddress;

        if ((threadsIndexP = malloc(sizeof(*threadsIndexP))) == NULL)
        {
            printf("ERROR; Failed to malloc threadsIndex\n");
            pthread_exit((void*) -1);
        }
        
        *threadsIndexP = i;
        if ((rc = pthread_create(&(threads[i].tid), &attr, mem_test_work, (void *) threadsIndexP)) != 0)
        {
            threads[i].tid = 0;
            threads[i].startAddress = NULL;
            threads[i].workAddress = NULL;
            free(startAddress);
            printf("ERROR; return code from pthread_create() index %u is %d\n", i, rc);
            my_abort();
            pthread_exit((void*) -1);
        }
        else
        {
            printf("Thread: %lu start test at %p\n", threads[i].tid, startAddress);
        }
        
    }

    /* Free attribute and wait for the other threads */
    pthread_attr_destroy(&attr);
}

/* print all workers' status */
void status(void)
{
    unsigned int i;
    unsigned long testedSize = 0;
    unsigned int process = 0;
    for (i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (threads[i].tid)
        {
            testedSize = (unsigned long)threads[i].workAddress - \
            (unsigned long)threads[i].startAddress;
            if (testedSize >= TEST_MEM_SIZE_PER_THREAD)
            {
                process = 100;
            }
            else
            {
                process = ((double)testedSize/TEST_MEM_SIZE_PER_THREAD) * 100;
            }

            printf("Thread: %lu, testSize: %#x, startAddress: %p, process: %%%u\n", \
            threads[i].tid, TEST_MEM_SIZE_PER_THREAD, threads[i].startAddress, process);
        }
    }
}

/* cancel all worker */
void my_abort(void)
{
    unsigned int i;
    int rc;
    void *status;
    for (i = 0; i < MAX_NUM_THREADS; i++)
    {
        if (threads[i].tid)
        {
            if ((rc = pthread_cancel(threads[i].tid)))
            {
                if ((rc = pthread_join(threads[i].tid, &status)))
                {
                    printf("ERROR; return code from pthread_join(%lu) is %d\n", \
                    threads[i].tid, rc);
                }
                else
                {
                    printf("Thread: %lu joined having a status of %ld\n", \
                    threads[i].tid, (long) status);
                }
            }
        }
        threads[i].tid = 0;
        threads[i].startAddress = NULL;
        threads[i].workAddress = NULL;
        free(threads[i].startAddress);
    }
}

/* abort and exit */
void my_exit(void)
{
    my_abort();
    pthread_exit(NULL);
}
