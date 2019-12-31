## Objectives

**In this lab, you need to implement a simple virtual file system: [procfs](https://en.wikipedia.org/wiki/Procfs). And create at least two files or nodes under the `/proc`. For example, by reading the `/proc/psinfo` we can get information about the current processes; by reading the `/proc/hdinfo` we can know about resource usage of the root file system.**

<br />

**Hint:**

You can implement the [procfs](https://en.wikipedia.org/wiki/Procfs) by patching the MINIX 1.0 file system so that all files in `proc` are just device files created in memory using `mkdir()` and `mknod()`.

<br />

**Notice:**

During the process of reading a file under `/proc`, the corresponding system status may change. How should you handle this situation?

Task 0, whose PID is zero, is actually part of the kernel rather than a normal [user-mode](http://en.wikipedia.org/wiki/User-mode) process.

## Demo

![demo](./img/demo.gif)

