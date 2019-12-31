#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/types.h>

#define HZ	100 /* Time slice factor */
#define LAST 8
#define CPU_TIME 1
#define IO_TIME 1

void cpuio_bound(int last, int cpu_time, int io_time);

int main(int argc, char * argv[])
{
	unsigned long i;
	unsigned long grandpaPid = getpid();
	unsigned long fatherPid, childPid;

	for (i = 0; i < 16; i++)
	{
		if ((fatherPid=fork()) == 0)
		{
			cpuio_bound(0, 0, 0);
			if ((childPid=fork()) == 0)
			{
				cpuio_bound(LAST, CPU_TIME, IO_TIME);
				return 0;
			}
			else
			{
				printf("father process %lu create child process %lu\n", getpid(), childPid);
				fflush(stdout);
			}
			
			if (waitpid(-1, NULL, 0) == -1)
			{
				perror("wairpid in father: ");
				return 1;
			}
			return 0;
		}
		else
		{
			printf("grandpa process %lu create father process %lu\n", grandpaPid, fatherPid);
			fflush(stdout);
		}
	}

	while ((i = waitpid(-1, NULL, 0)))
	{
		if (i == -1)
		{
			if (errno == ECHILD)
			{
				break;
			}
			else
			{
				perror("wairpid in grandpa: ");
				return 1;
			}
			
		}
	}

	return 0;
}

void cpuio_bound(int last, int cpu_time, int io_time)
{
	struct tms start_time, current_time;
	clock_t utime, stime;
	int sleep_time;

	while (last > 0)
	{
		/* CPU Burst */
		times(&start_time);
		do
		{
			times(&current_time);
			utime = current_time.tms_utime - start_time.tms_utime;
			stime = current_time.tms_stime - start_time.tms_stime;
		} while ( ( (utime + stime) / HZ )  < cpu_time );
		last -= cpu_time;

		if (last <= 0 )
			break;

		/* IO Burst */
		sleep_time=0;
		while (sleep_time < io_time)
		{
			sleep(1);
			sleep_time++;
		}
		last -= sleep_time;
	}
}
