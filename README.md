# Hack-Linux-0.11

This repository records the code I wrote while reading the book *Operating System Principles, Implementation and Practice* (ISBN: 978-7-04-049245-3).



The textbook designs eight small labs, which are code assignments in the operating system course for undergraduates at [HIT](https://en.wikipedia.org/wiki/Harbin_Institute_of_Technology). Topics include booting process; interrupts; peripherals and drivers; file system; system calls; context switches; process scheduling; virtual memory and memory management; inter-process communication, etc.



Linux 0.11, [a free minix-like kernel for 386-AT](https://groups.google.com/forum/#!msg/comp.os.minix/4995SivOl9o/GwqLJlPSlCEJ) written by [Linus Torvalds](https://en.wikipedia.org/wiki/Linus_Torvalds), is used to illustrate these topics. 



## Directory Structure

[oslab.zip (oslab/)](https://github.com/QiuhaoLi/Hack-Linux-0.11/blob/master/oslab.zip): Linux 0.11 code and tools we needed to set up the development environment, such as Bochs 2.3.7 and GCC 3.4.



[tmp/](https://github.com/QiuhaoLi/Hack-Linux-0.11/tree/master/tmp): Temporary files (mostly files about threads) which are not relevant to the labs.



[Lab 6.Ext. Print paging status/](https://github.com/QiuhaoLi/Hack-Linux-0.11/tree/master/Lab%206.Ext.%20Print%20paging%20status): An interesting lab I designed.



The other eight directories correspond to the labs, each containing **changed** files and a markdown document which illustrates the lab.



You can use tools like `rsync` and `cp` to modify the original `oslab` . For example, on my computer, I start the hacked Linux 0.11 of [Lab6.Ext](https://github.com/QiuhaoLi/Hack-Linux-0.11/tree/master/Lab%206.Ext.%20Print%20paging%20status) on Bochs with following bash commands:

```bash
qiuhaoli@VM:~/tmp/oslab$ rsync -a ../Hack-Linux-0.11/Lab\ 6.Ext.\ Print\ paging\ status/linux-0.11/ ./linux-0.11/

qiuhaoli@VM:~/tmp/oslab$ sudo ./mount-hdc 

qiuhaoli@VM:~/tmp/oslab$ cd ./hdc/usr/include/

qiuhaoli@VM:~/tmp/oslab/hdc/usr/include$ sudo cp ../../../../Hack-Linux-0.11/Lab\ 6.Ext.\ Print\ paging\ status/hdc/usr/include/* .

qiuhaoli@VM:~/tmp/oslab/hdc/usr/include$ cd ../../..

qiuhaoli@VM:~/tmp/oslab$ cd linux-0.11/

qiuhaoli@VM:~/tmp/oslab/linux-0.11$ make && ../run
```




## Acknowledgements

[Sunner](http://sunner.cn/) and students at HIT provided most of the resources to set up the development environment, especially the guides to code and debug. You can get them on [cnblogs.com](https://www.cnblogs.com/liqiuhao/p/12128070.html) and [gitbooks.io](https://hoverwinter.gitbooks.io/hit-oslab-manual/content/environment.html) (written in Chinese).



[oldlinux.org](http://www.oldlinux.org/) has provided many helpful materials related to the ancient Linux.



The Linux 0.11 kernel is supported by [system software](https://en.wikipedia.org/wiki/System_software) and [libraries](https://en.wikipedia.org/wiki/Library_(computer_science)), many of which are provided by the [GNU Project](https://en.wikipedia.org/wiki/GNU_Project) and the [MINIX](https://en.wikipedia.org/wiki/MINIX) operating system.

