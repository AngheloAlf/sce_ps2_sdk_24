#
#  IOP hello program Makefile
#
#  $Id: Makefile,v 1.1 2001/02/06 12:57:01 komaki Exp $
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
	$(LINK.o) $^ $(LOADLIBES) $(LIBDIR)/modem.o $(LDLIBS) -o $@

#----------- customize section --------------
CFLAGS   =  -Wall -g
ASFLAGS  =
LOADLIBES=
MODEMLIBDIR   = /usr/local/sce/iop/gcc/mipsel-scei-elfl/lib
LDLIBS   = -ilb=inet.ilb
PROGNAME = main.irx
OBJS	 = $(PROGNAME:%.irx=%.o) # add here other objects

all:	$(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

$(PROGNAME:%.irx=%.o) : $(PROGNAME:%.irx=%.c)

$(PROGNAME): $(MODEMLIBDIR)/modem.o $(OBJS)
	$(LINK.o) $(LOADLIBES) $^ -o $@ $(LDLIBS)
