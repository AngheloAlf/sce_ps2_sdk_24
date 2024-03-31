#
#  IOP iLINK sample program Makefile
#
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	$(MAKE) all
else
include PathDefs
endif

#----------- customize section --------------
ILINKILB	= ilink.ilb
SOCKILB		= ilsock.ilb
CFLAGS   = -I/usr/local/sce/common/include -Wall
ASFLAGS  =
ILIBS   = -ilb=$(ILINKILB) -ilb=$(SOCKILB)

PROGNAME = ilsample

all:	$(PROGNAME).irx

OBJS	= $(PROGNAME).o

#----------- rules --------------
%.irx: %.o
	$(LINK.o) -o $@ $(OBJS) $(LOADLIBES) $(LDLIBS) $(ILIBS)

clean:
	rm -f *.o $(PROGNAME) *.map $(PROGNAME).irx

install:
	cp $(PROGNAME).irx /usr/local/sce/iop/modules/
