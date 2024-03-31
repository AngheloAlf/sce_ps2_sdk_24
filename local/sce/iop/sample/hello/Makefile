#
#  IOP hello program Makefile
#
#  $Id: Makefile,v 1.11 2001/05/20 09:23:33 tei Exp $
#

#----------- customize section --------------
DBGFLAG  = -g
#OPTFLAG  = -O2
CFLAGS   = -Wall $(DBGFLAG) $(OPTFLAG)
ASFLAGS  = $(DBGFLAG)
LDFLAGS  = $(DBGFLAG)
LOADLIBES=

PROGNAME = hello.irx
OBJS	 = $(PROGNAME:%.irx=%.o) # add here other objects

all:	$(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

run:	$(PROGNAME)
	dsreset 0 2 ; dsistart $(PROGNAME)

$(PROGNAME:%.irx=%.o) : $(PROGNAME:%.irx=%.c) # add your local include file

$(PROGNAME): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

-include PathDefs
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)

#
#  How to create IOP relocatable executebule object.
#
# full step
#   % iop-gcc -c part1.c
#   % iop-gcc -c part2.c
#   % ioplibld -L<libdir> -s allpartsub.s part1.o part2.o : xxx.ilb
#   % iop-as -o allpartsub.o allpartsub.s 
#   % iop-ld -dc -r -o program.o  part1.o part2.o allpartstub.o -l<lib>
#   % iopfixup -o program  program.o
#   % rm allpartstub.s allpartstub.o program.o
# simple way
#   % iop-gcc -c part1.c
#   % iop-gcc -c part2.c
#   % iop-gcc -o program part1.o part2.o -L<libdir> -l<lib> -ilb=xxx.ilb
#
