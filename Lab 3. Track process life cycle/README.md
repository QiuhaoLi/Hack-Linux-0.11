## Objectives

**Write a small multi-process program named `process.c`:**

1. All child processes run in parallel, and the real time of each child process may not exceed 30 seconds.
2. The parent process prints the PIDs of all child processes to stdout and exits only after all child processes exit.

<br />

**Modify the kernel code to track life cycles of all processes since the bootup, and record them in  `/var/process.log` .** 

Each line of the log should be  `PID \t X \t tickTime \n` .

X can be:

1. N: The process is just generated

2. J: The process is ready to run
3. R: The process is now running
4. W: The process is blocked
5. E: Exit

You can get the tick of system by reading the global variable `jiffies` .

<br />

**Run `process.c` on the modified Linux and write a program to analyze `process.log` .**

<br />

**Analyze the kernel code, and comment the code (` /* Time slice factor */`) that can affect the [time slice]( https://en.wikipedia.org/wiki/Preemption_(computing)#Time_slice ).**

<br />

**Change the code you just commented, and run `process.c` again. What’s new about the log file?**


## Demo

The following shows the log after bootup -> execute `sync` command -> power off :

```
1	N	48
1	J	48
0	J	48
1	R	48
2	N	49
2	J	49
1	W	49
2	R	49
3	N	64
3	J	64
2	J	64
3	R	64
3	W	68
2	R	68
2	E	73
1	J	73
1	R	73
4	N	74
4	J	74
1	W	74
4	R	74
5	N	106
5	J	106
4	W	107
5	R	107
4	J	109
5	E	109
4	R	109
4	W	115
0	R	115
4	J	1138
4	R	1138
4	W	1139
0	R	1139
4	J	1176
4	R	1177
4	W	1177
0	R	1177
4	J	1191
4	R	1191
4	W	1191
0	R	1191
4	J	1226
4	R	1226
4	W	1226
0	R	1226
4	J	1292
4	R	1293
4	W	1293
0	R	1293
4	J	1330
4	R	1330
4	W	1330
0	R	1330
4	J	1366
4	R	1366
4	W	1366
0	R	1366
4	J	1414
4	R	1414
4	W	1414
0	R	1414
4	J	1640
4	R	1640
6	N	1643
6	J	1643
4	W	1643
6	R	1643
```

