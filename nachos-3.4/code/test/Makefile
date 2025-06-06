# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# if you are cross-compiling, you need to point to the right executables
# and change the flags to ld and the build procedure for as

GCCDIR = ../../../gnu-decstation-ultrix/decstation-ultrix/2.95.3/

LDFLAGS = -T script -N
ASFLAGS = -mips2
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)

PATH = $(GCCDIR):/lib:/usr/bin:/bin

CC = $(GCCDIR)gcc -B../../../gnu-decstation-ultrix/
COFF2NOFF = ../bin/coff2noff
AS = $(GCCDIR)as
LD = $(GCCDIR)ld

CPP = gcc -m32 -E
INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

# all: halt shell matmult sort fork join kill exec memory
all: halt shell matmult sort fork join kill exec memory concurrentRead cp open read write create

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.c > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

fork.o: fork.c
	$(CC) $(CFLAGS) fork.c
fork: fork.o start.o
	$(LD) $(LDFLAGS) start.o fork.o -o fork.coff
	../bin/coff2noff fork.coff fork 

join.o: join.c
	$(CC) $(CFLAGS) join.c
join:   join.o start.o
	$(LD) $(LDFLAGS) start.o join.o -o join.coff
	../bin/coff2noff join.coff join 

kill.o: kill.c
	$(CC) $(CFLAGS) kill.c
kill:   kill.o start.o
	$(LD) $(LDFLAGS) start.o kill.o -o kill.coff
	../bin/coff2noff kill.coff kill 

exec.o: exec.c
	$(CC) $(CFLAGS) exec.c
exec: exec.o start.o
	$(LD) $(LDFLAGS) start.o exec.o -o exec.coff
	../bin/coff2noff exec.coff exec 

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

create.o: create.c
	$(CC) $(CFLAGS) -c create.c
create: create.o start.o
	$(LD) $(LDFLAGS) start.o create.o -o create.coff
	../bin/coff2noff create.coff create

exit.o: exit.c
	$(CC) $(CFLAGS) -c exit.c
exit: exit.o start.o
	$(LD) $(LDFLAGS) start.o exit.o -o exit.coff
	../bin/coff2noff exit.coff exit

shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c
shell: shell.o start.o
	$(LD) $(LDFLAGS) start.o shell.o -o shell.coff
	../bin/coff2noff shell.coff shell

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

memory.o: memory.c
	$(CC) $(CFLAGS) memory.c
memory: memory.o start.o
	$(LD) $(LDFLAGS) start.o memory.o -o memory.coff
	../bin/coff2noff memory.coff memory 

open.o: open.c
	$(CC) $(CFLAGS) -c open.c
open: open.o start.o
	$(LD) $(LDFLAGS) start.o open.o -o open.coff
	../bin/coff2noff open.coff open

read.o: read.c
	$(CC) $(CFLAGS) -c read.c
read: read.o start.o
	$(LD) $(LDFLAGS) start.o read.o -o read.coff
	../bin/coff2noff read.coff read

write.o: write.c
	$(CC) $(CFLAGS) -c write.c
write: write.o start.o
	$(LD) $(LDFLAGS) start.o write.o -o write.coff
	../bin/coff2noff write.coff write

concurrentRead.o: concurrentRead.c
	$(CC) $(CFLAGS) -c concurrentRead.c

concurrentRead: concurrentRead.o start.o
	$(LD) $(LDFLAGS) start.o concurrentRead.o -o concurrentRead.coff
	../bin/coff2noff concurrentRead.coff concurrentRead

cp.o: cp.c
	$(CC) $(CFLAGS) -c cp.c

cp: cp.o start.o
	$(LD) $(LDFLAGS) start.o cp.o -o cp.coff
	$(COFF2NOFF) cp.coff cp



