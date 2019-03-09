#C Compiler
CC = gcc

#GCC Flags
CFLAGS = -m32 -nostdlib -nostdinc -e main -Ttext=100 -static -fno-builtin

#Replaces standard includes with linux 0.01 include, includes 
INCLUDES = -I../../include -I../utils 
#Replaces stdlib with lib
LIBS = ../../lib/lib.a

TARGET = calculate.c 

FILES = ../utils/utils.c

all:
	$(CC) $(CFLAGS) -o $(TARGET).bin $(TARGET).c $(FILES) $(LIBS) $(INCLUDES)
