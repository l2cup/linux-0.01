# Linux-0.01

This is a working historical release of the linux kernel, modified to use linux filesystem instead of minix and to compile with GCC 4.6.  
This is our Operating Systems assignment, for now we are creating simple userland programs, as this is an **ongoing** class things may change in the future.  
As for running this you can either downgrade GCC to 4.6 or install [Ubuntu 12.06](http://releases.ubuntu.com/12.04/) the latest known release to come with this specific version of the compiler, and use [qemu](https://www.qemu.org) and [kvm](https://www.linux-kvm.org/page/Main_Page) to emulate an i386 processor.

## How to run

When in the directory of the OS, run `make`.
The image you are booting from is *Image* and the hard drive that linux uses is *hd_oldlinux.img*.   
For mounting the [apps](apps) directory, you can use an already written [script](apps/make_app.sh) found inside that directory. The userland C programs use make with already preconfigured Makefiles to compile. If you wish to write your own program, look at the Makefiles for flags, as you compile with `--no-stdlib` , `--no-stdinc` and `-static` as some of the flags, which limits you to system calls for I/O.  
After mounting the directory which contain apps using the script or yourself, you will need to emulate the i386 processor. Presuming you've already installed qemu and kvm on your machine, mounted the oldlinux hard drive, you need to run  
`qemu-system-i386 -hdb hd_oldlinux.img -fda Image -boot a` in your terminal when in the os's folder which contains the bootable image and the hard drive.

## Libraries
If you want you can use [dietlibc](https://www.fefe.de/dietlibc/) which is a minimal C standard library used for embedded systems but in this project it will probaby stay unused.  
There is a utility library named [utils.h](apps/utils/utils.h) which will be worked on to contain needed utilities for this project, if there becomes a need for more libraries, they will be added in the *libs* folder which currently doesn't exist, containing the *utils* folder in it.

## Kernel space
As of now no kernel code was written, but since this is only a beginning, in the future it may as well be. Neverentless of the class assignments that we are given, there is a slight annoyance in the keyboard mapping which is currently using a Finnish keyboard layout. As the mapping is table based it will probably be changed to US(International) in the near future.

## Resources
This is a list of useful resources if you want to get this kernel up and running and get a better understanding of it:  
* [Commentary on the Linux 0.01](https://www.academia.edu/5267870/The_Linux_Kernel_0.01_Commentary) 
* [Offical release notes of Linux 0.01 by Linus Torvalds](http://gunkies.org/wiki/Linux_0.01)  
* [A pdf version of the 80836 Intel's reference manual](https://css.csail.mit.edu/6.858/2014/readings/i386.pdf)

## Contact
If you wish to contact me about this, anything code related or even not code related, feel free to do so on [nikolic.uros@me.com](mailto:nikolic.uros@me.com?subject=[Github]%20Linux%20Kernel%200.01).

