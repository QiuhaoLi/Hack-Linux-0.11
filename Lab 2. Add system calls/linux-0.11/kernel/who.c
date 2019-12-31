#include <errno.h>
#include <string.h>
#include <linux/kernel.h>
#include <asm/segment.h>

/* 23 chars + '\0' */
#define MAX_NAME_LENGTH 24

char nameInKernel[MAX_NAME_LENGTH];

int sys_iam(const char * name)
{
    char tmpNameInKernel[MAX_NAME_LENGTH] = {0};

    /* read name from user space */
     int i;
    for (i = 0; i < MAX_NAME_LENGTH; i++)
    {
        char c = get_fs_byte(name + i);
        if (c == '\0')
        {
            break;
        }
        tmpNameInKernel[i] = c;
    }

    /* detect error for overlong name */
    if (tmpNameInKernel[MAX_NAME_LENGTH-1] != '\0')
    {
        return -EINVAL;
    }

    /* store the name in kernel */
    strncpy(nameInKernel, tmpNameInKernel, MAX_NAME_LENGTH);

    return i;
}

int sys_whoami(char* name, unsigned int size)
{
    unsigned int needLength = strlen(nameInKernel) + 1;

    /* detect error for Insufficient name space */
    if (needLength > size)
    {
        return -EINVAL;
    }

    unsigned int i;
    for (i = 0; i < needLength; i++)
    {
        put_fs_byte(nameInKernel[i], name + i);
    }
    
    return needLength-1;
}

