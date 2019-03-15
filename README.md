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

## User space
As the focus of the class for now is userspace, all of the working programs titles and it's descriptions will be listed here. I will be updating the list with more programs as we make them. 

### Keyboard emulator

This is a keyboard emulator which reads a file, containing predefined scancodes which correspond to a key press. The file must be formatted in a way that each line is a keyboard character scancode, ending with scancode 400 which is hardcoded to be 0 or EOF. You quit the program when you type 'quit' instead of a filename.

#### Hardcoded values

| State / Button| Shift | Alt | Ctrl | 
|:----:|:-----:|:---:|:----:|
| Down |  200  | 201 |  202 |
|  Up  |  300  | 301 |  302 |


#### Funcionalities

It has 3 main functionalities:  

##### Basic + Shift
The scancodes go from 0 to 128 so the first functionality is that the same scancode can be a lowercase or an uppercase letter depending on if the shift down scancode precedees it. Until the shift down scancode is read, all characters that are read after the shift up scancode are uppercase (*shift-x*) values of the same keypress.

##### Alt - Ascii value
The alt scancode allows users to type in their own ascii value and the character for that value is printed **only** when the alt down scancode is read. Ascii values can be written as a whole or each number in a new line and the program will automatically multiply the previous value by 10 and add the new value to it. It has overflow protection, so if a user goes beyond 128 it simply won't print anything.

##### Ctrl - Mnemonics
The ctrl scancode allows mnemonics on all 128 ascii values. Mnemonics are mapped in a *ctrl.map* file whose format will be explained in the map files section. Because you map mnemonics on the character itself, you can map a mnemonic on both lowercase and uppercase version of a letter. Because you can change the *scancodes.tbl* file yourself, if a lowercase letter has the same scancode as the uppercase version of the same letter, shift scancode is used to distinguish mnemonics. If the shift scancode is read mnemonics will be printed as if the uppercase ascii value of the same letter is read. Mnemonics are printed instantly so everything that comes after the ctrl down scancode will be treated as a mnemonic key untill the ctrl up scancode is read.

#### Map files

It has 2 map files:

* **scancodes.tbl**

* **ctrl.map**

[**Scancodes.tbl**](apps/keyboard_emulator/scancodes.tbl) is a 2 line ascii character map file, first line corresponds to normal characters, and the second one to characters you get while *holding* the shift button.

The program reads and maps these characters automatically. Right now it contains 2 lines of 47 characters, though it can be changed to contain 128 characters per line.

[**Ctrl.map**](apps/keyboard_emulator/ctrl.map) is a mnemonic map which emulates keyboard shortcuts with ctrl. It can hold up to 128 characters and 128 mnemonics mapped to these characters up to 64 characters long. First line of the file is just a number of mnemonics, you don't need to change it because it is just read and discarded, it's there because the format of files is a class assigment requirement and my implementation, though memory heavy, just discards it.

It mimicks Ctrl-*x* presses where x is a mapped character.

#### Implementation

Loading and parsing files is done in C, processing scancodes one by one is done in GCC Extended Inline Assembly, with at&t syntax. I am not very fond of the at&t syntax that GCC uses, and it was easier just to write one syntax instead of combining syntaxes as the function uses the extended assembly features. It may not be the most optimized function as i don't write assembly often, though i read it from time to time.


##### Notice

The mnemonic [file](apps/keyboard_emulator/ctrl.map) included, as well as the program prompt is written in Serbian, which is my native language because this is a college project. This is an open source project so if you decide to fork it or clone it, you can change the prompt text in [main](apps/keyboard_emulator/main.c).



## Kernel space
For now no kernel code was written, but since this is only a beginning, in the future it may as well be. Neverentless of the class assignments that we are given, there is a slight annoyance in the keyboard mapping which is currently using a Finnish keyboard layout. As the mapping is table based it will probably be changed to US(International) in the near future.

## Resources
This is a list of useful resources if you want to get this kernel up and running and get a better understanding of it:  
* [Commentary on the Linux 0.01](https://www.academia.edu/5267870/The_Linux_Kernel_0.01_Commentary)  
* [Offical release notes of Linux 0.01 by Linus Torvalds](http://gunkies.org/wiki/Linux_0.01)   
* [A pdf version of the 80836 Intel's reference manual](https://css.csail.mit.edu/6.858/2014/readings/i386.pdf)

## Contact
If you wish to contact me about this, anything code related or even not code related, feel free to do so on [nikolic.uros@me.com](mailto:nikolic.uros@me.com?subject=[Github]%20Linux%20Kernel%200.01).
