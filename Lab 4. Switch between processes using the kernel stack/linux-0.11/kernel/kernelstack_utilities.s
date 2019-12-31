KERNEL_STACK = ((33*16)+4)  # offset in the task-struct.
ESP0 = 4    # offset in the TSS-struct.

.global first_return_from_kernel, switch_to
.text
first_return_from_kernel:
    popl %edx
    popl %edi
    popl %esi
    pop %gs
    pop %fs
    pop %es
    pop %ds
    iret

switch_to:
    pushl %ebp
    movl %esp,%ebp
    pushl %ecx
    pushl %ebx
    pushl %eax
    movl 8(%ebp),%ebx
    cmpl %ebx,current
    je 1f

    movl %ebx,%eax
    xchgl %eax,current

    movl init_tss,%ecx
    addl $4096,%ebx
    movl %ebx,ESP0(%ecx)

    movl %esp,KERNEL_STACK(%eax)
    movl 8(%ebp),%ebx
    movl KERNEL_STACK(%ebx),%esp

    movl 12(%ebp),%ecx
    lldt %cx
    movl $0x17,%ecx
    mov %cx,%fs

    cmpl %eax,last_task_used_math
    jne 1f
    clts

    1: popl %eax
    popl %ebx
    popl %ecx
    popl %ebp
    ret
