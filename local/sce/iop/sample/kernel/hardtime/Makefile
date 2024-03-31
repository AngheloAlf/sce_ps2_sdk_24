#
#  IOP thread sample program Makefile
#
#  $Id: Makefile,v 1.2 2001/05/20 09:23:33 tei Exp $
#

#----------- customize section --------------
DBGFLAG  = -g
#OPTFLAG  = -O2
CFLAGS   = -Wall $(DBGFLAG) $(OPTFLAG)
ASFLAGS  = $(DBGFLAG)
LDFLAGS  = $(DBGFLAG)
LOADLIBES=

PROGS = timer1.irx timer2.irx timer3.irx timer4.irx timer5.irx

all:	$(PROGS)

clean:
	rm -f *.o $(PROGS) *.map

run: all
	dsreset 0 0
	for i in $(PROGS:.c=.irx) ;\
	do \
		dsistart $$i; \
	done; echo allend

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

-include PathDefs
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
