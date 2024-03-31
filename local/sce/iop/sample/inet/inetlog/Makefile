#
#  IOP hello program Makefile
#
#  $Id: Makefile,v 1.1 2001/01/17 12:28:28 komaki Exp $
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	$(MAKE) all
else
include PathDefs
endif

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

#----------- customize section --------------
CFLAGS   =  -Wall -g
ASFLAGS  =
LOADLIBES=
LDLIBS   = -ilb=inet.ilb
PROGNAME = inetlog.irx
OBJS	 = $(PROGNAME:%.irx=%.o) # add here other objects

all:	$(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

$(PROGNAME:%.irx=%.o) : $(PROGNAME:%.irx=%.c)

$(PROGNAME): $(OBJS)
	$(LINK.o) $(LOADLIBES) $^ -o $@ $(LDLIBS)
