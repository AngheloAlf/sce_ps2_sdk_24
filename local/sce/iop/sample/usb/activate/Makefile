#
#  IOP USB Static Descriptor Printer
#
#  $Id: Makefile,v 1.3 2001/07/05 11:02:08 fukunaga Exp $
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
CFLAGS   =  -Wall -G0 -I/usr/local/sce/common/include
ASFLAGS  =
LDLIBS   = -ilb=usbmload.ilb
PROGNAME = activate.irx
OBJS	 = $(PROGNAME:%.irx=%.o) # add here other objects

all:	$(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

run:	$(PROGNAME)
	dsreset 0 0
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048
	dsistart /usr/local/sce/iop/modules/usbmload.irx conffile=host1:/usr/local/sce/conf/usb/usbdrvho.cnf debug=1
	dsidb -nr -noexit mstart $(PROGNAME) Mouse Keyboard

$(PROGNAME:%.irx=%.o) : $(PROGNAME:%.irx=%.c) # add your local include file

$(PROGNAME): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
