#define __LIBRARY__
#include <unistd.h>
#include <stdio.h>

/* 23 chars + '\0' */
#define MAX_NAME_LENGTH 24

_syscall2(int, whoami,char*,name,unsigned int,size);

int main(void)
{
	char name[MAX_NAME_LENGTH] = {0};
	whoami(name, MAX_NAME_LENGTH);
	printf("%s\n", name);
	
	return 0;
}
