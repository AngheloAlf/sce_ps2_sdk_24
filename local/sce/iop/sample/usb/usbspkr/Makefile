#
#  IOP usbspkr program Makefile
#
#  $Id: Makefile,v 1.1 2001/12/01 10:35:34 fukunaga Exp $
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
PROGNAME = usbspkr.irx
OBJS	 = $(PROGNAME:%.irx=%.o) # add here other objects

WAVFILE  = host1:XXX.wav

all:	$(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

run:	$(PROGNAME)
	dsreset 0 0 ;\
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=1024 ;\
	dsidb -nr -noexit mstart $(PROGNAME) $(WAVFILE)

$(PROGNAME:%.irx=%.o) : $(PROGNAME:%.irx=%.c) # add your local include file

$(PROGNAME): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
