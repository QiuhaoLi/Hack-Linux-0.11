#define __LIBRARY__
#include <unistd.h>
#include <errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>

struct sharedPage
{
    unsigned int count; // shared by count processes
    unsigned long physicalAddress;
};

struct sharedPage sharedPages[MAX_SHARED_PAGES_NUM] = {{0}};

/* key: 0~MAX_SHARED_PAGES_NUM-1 */
int sys_shmget(unsigned int key, size_t size)
{
    unsigned long page = 0;
    
    if (size > PAGE_SIZE)
    {
        printk("sys_shmget: size > PAGE_SIZE\n");
        return -EINVAL;
    }
    
    if (key >= MAX_SHARED_PAGES_NUM)
    {
        printk("key >= MAX_SHARED_PAGES_NUM\n");
        return -ENOMEM;
    }
    
    if (sharedPages[key].physicalAddress && sharedPages[key].count) /* exists */
    {
        sharedPages[key].count++;
        return key;
    }
    
    if ((page = get_free_page()) == 0)
    {
        printk("failed to get_free_page\n");
        return -ENOMEM;
    }
    sharedPages[key].physicalAddress = page;
    sharedPages[key].count++;
    
    return key;
}  

/* shmid: 0~MAX_SHARED_PAGES_NUM-1 */
long sys_shmat(int shmid)  
{
    unsigned int key = (unsigned int) shmid;
    unsigned long logicalAddress = current->brk;
    unsigned long linearAddress = get_base(current->ldt[1]) + current->brk;
    unsigned long physicalAddress = 0;
    
    if (key >= MAX_SHARED_PAGES_NUM || sharedPages[key].count == 0 || sharedPages[key].physicalAddress == 0)
    {
        printk("sys_shmat: key >= MAX_SHARED_PAGES_NUM || sharedPages[key].count == 0 || sharedPages[key].physicalAddress == 0;key = %u \n", key);
        return -EINVAL;
    }
    physicalAddress = sharedPages[key].physicalAddress;
     
    if (!put_page(physicalAddress, linearAddress))
    {
        printk("failed to put_page\n");
        return -EINVAL;
    }
    
    current->brk += PAGE_SIZE;
    
    return (long)logicalAddress;  
}

/* return 0 if there is someone other than current->pid is using this shared page */
int release_shared_page_if_exists(unsigned long addr)
{
    unsigned long address = (addr << 12) + LOW_MEM;
    int i;
    /* The code here can be optimized */
    for (i = 0; i < MAX_SHARED_PAGES_NUM; ++i)
    {
        if (sharedPages[i].physicalAddress == address && sharedPages[i].count)
        {
            --sharedPages[i].count;
            if (sharedPages[i].count == 0)
            {
                return 1;
            }
            return 0;
        }
    }
    return 1;
}


