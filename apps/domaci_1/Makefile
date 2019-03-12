#C Compiler
CC = gcc

#GCC Flags
CFLAGS = -m32 -nostdlib -nostdinc -e main -Ttext=100 -static -fno-builtin

#Replaces standard includes with linux 0.01 include, includes 
INCLUDES = -I../../include -I../utils -I scan 
#Replaces stdlib with lib
LIBS = ../../lib/lib.a

TARGET = main

FILES = ../utils/utils.c scan/scan.c

all:
	$(CC) $(CFLAGS) -o domaci_1.bin $(TARGET).c $(FILES) $(LIBS) $(INCLUDES)