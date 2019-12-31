## Objectives

**In this lab, you need to implement context switch between processes using the kernel stack instead of built-in hardware mechanisms of the 80386.**

<br />

**Run `switch_test.c` to test your implementation.**

<br />

**Notice:**

If the layout of  `struct task_struct` is modified, you **might** need to make corresponding changes to the hard-coded part in  `system_call.s` :

```c
state	= 0		# these are offsets into the task-struct.
counter	= 4
priority = 8
signal	= 12
sigaction = 16		# MUST be 16 (=len of sigaction)
blocked = (33*16)
```



## Demo

![demo](./img/demo.gif)
