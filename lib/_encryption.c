#define __LIBRARY__
#include <unistd.h>

_syscall0(int,init_encryption)

_syscall2(int, keyset, const char *, key,int,global)

_syscall1(int, encr, const char *, filename)

_syscall1(int, decr, const char *, filename)

_syscall1(int, keyclear,int, global)

_syscall1(int, keygen, int, level)

_syscall0(int,key)