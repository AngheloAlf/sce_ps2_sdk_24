#
#  IOP sifrpc program Makefile
#
#  $Id: Makefile,v 1.3 1999/09/09 13:20:30 tei Exp $
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
CFLAGS   =  -Wall
ASFLAGS  =
PROGNAME = iopmain.irx

all:	$(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

run:	$(PROGNAME)
	dsreset 0 2 ; dsistart $(PROGNAME)
