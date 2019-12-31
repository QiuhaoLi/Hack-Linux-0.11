#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define PAGEDIR 0
#define MAX_MEM 0x4000000
#define PAGE_DIR_MEM 0x400000
#define PAGE_FRAME_SIZE 0x20

int sys_debug_paging(pid_t pid, void *address, const char *logPath)
{
    struct task_struct ** p = NULL;
    unsigned int taskNumber = -1, outputFd = 1;
    unsigned long pageDirectoryIndex = 0, pageTableIndex = 0, physicalOffset = 0;
    void *linearAddressBase = NULL;
    void *linearAddress = NULL, *physicalAddress = NULL;
    void *pageTableAddress = NULL, *physicalPageAddress = NULL;
    long pageTablesAddress[MAX_MEM/PAGE_DIR_MEM] = {0};
    long *tmp = NULL;
    const char *separator = "\n--------------------------------\n";
    int i = 0, j = 0;

    for(p = &LAST_TASK, taskNumber = NR_TASKS-1 ; p > &FIRST_TASK ; --p, --taskNumber)
        if (*p)
            if ((*p)->pid == pid)
                break;
    if (!(p > &FIRST_TASK))
        return -EINVAL;
    linearAddressBase = (void *)(0x4000000 * taskNumber);

    if (logPath)
    {
        if ((outputFd = sys_open(logPath, O_CREAT|O_APPEND|O_WRONLY,0666)) == -1)
        {
            printk("Failed to open output file!\n");
            return -EINVAL;
        }
    }

    fprintk(outputFd, "\nPid: %ld Task: %u linearBaseAddress: 0x%p\n", \
    pid, taskNumber, linearAddressBase);
    
    if (address == NULL) /* print all dirs and pages*/
    {

        pageDirectoryIndex = (unsigned long) linearAddressBase >> (10+12);

        /* find all page tables */
        fprintk(outputFd, "\nIndex\tTable address\n");
        tmp = PAGEDIR;
        for (i = 0; i < MAX_MEM/PAGE_DIR_MEM; i++)
        {
            if (tmp[pageDirectoryIndex+i] & 1) /* P bit set */
            {
                pageTablesAddress[i] = tmp[pageDirectoryIndex+i] & 0xFFFFF000;
                fprintk(outputFd, "%#x\t0x%p\n", pageDirectoryIndex+i, pageTablesAddress[i]);
            }
                
        }

        /* for every page table, find all physical page address */
        for (i = 0; i < MAX_MEM/PAGE_DIR_MEM; i++)
        {
            if ((tmp = (long *)pageTablesAddress[i]))
            {
                fprintk(outputFd, "\nTable %p:\n", tmp);
                fprintk(outputFd, "\tIndex\tPage address\n", tmp);

                for (j = 0; j < PAGE_SIZE/PAGE_FRAME_SIZE; j++)
                {
                    if (tmp[j] & 1)  /* P bit set */
                    {
                        fprintk(outputFd, "\t%#x\t0x%p\n", j, (void *)(tmp[j] & 0xFFFFF000));
                    }
                    
                }
                
            }
        }
        fprintk(outputFd, "%s", separator);
    }
    else /* only address paging */
    {
        linearAddress = (void *)((unsigned long) linearAddressBase + \
        (unsigned long) address);

        fprintk(outputFd, "\nlinearAddress: 0x%p\n\n", linearAddress);

        pageDirectoryIndex = (unsigned long) linearAddress >> (10+12);
        pageTableIndex = ((unsigned long) linearAddress & 0x3FF000) >> 12;
        physicalOffset = (unsigned long) linearAddress & 0xFFF;

        fprintk(outputFd, "dirIndex\ttableIndex\toffset\n");
        fprintk(outputFd, "%#08x\t%#08x\t%#08x\n\n", pageDirectoryIndex, pageTableIndex, physicalOffset);

        tmp = PAGEDIR;
        if (tmp[pageDirectoryIndex] & 1) /* P bit set */
            pageTableAddress = \
            (void *)((unsigned long)tmp[pageDirectoryIndex] & 0xFFFFF000);
        else
            return -EINVAL;
        
        tmp = pageTableAddress;
        if (tmp[pageTableIndex] & 1)
            physicalPageAddress = \
            (void *)((unsigned long)tmp[pageTableIndex] & 0xFFFFF000);
        else
            return -EINVAL;
        
        physicalAddress = physicalPageAddress + physicalOffset;

        fprintk(outputFd, "tableAddress\tpageAddress\tphysicalAddress\n");
        fprintk(outputFd, "0x%p\t0x%p\t0x%p\n", \
        pageTableAddress, physicalPageAddress, physicalAddress);

        fprintk(outputFd, "%s", separator);
    }

    if (logPath)
        sys_close(outputFd);
    
	return 0;
}
