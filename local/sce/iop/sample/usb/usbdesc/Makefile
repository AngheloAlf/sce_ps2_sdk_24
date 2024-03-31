#
#  IOP USB Static Descriptor Printer
#
#  $Id: Makefile,v 1.5 2001/05/17 11:21:19 fukunaga Exp $
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
CFLAGS   =  -Wall -G0
ASFLAGS  =
LDLIBS   = -ilb=usbd.ilb
PROGNAME = usbdesc.irx
OBJS	 = $(PROGNAME:%.irx=%.o) \
           hid.o
           # add here other objects

all:	$(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

run:	$(PROGNAME)
	dsreset 0 0 ;\
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048 reportd=1;\
	dsistart $(PROGNAME)
	dsidb -nr

$(PROGNAME:%.irx=%.o) : $(PROGNAME:%.irx=%.c) # add your local include file

$(PROGNAME): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
