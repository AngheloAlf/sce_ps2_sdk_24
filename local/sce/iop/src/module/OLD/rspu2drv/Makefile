#
#  IOP test program Makefile
#
#  $Id: Makefile,v 1.9 2000/02/25 10:19:00 kaol Exp $
#

ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	make all
else
include PathDefs
endif

INCOPT = -I/usr/local/sce/common/include 

CFLAGS  = $(INCOPT) -I. -Wall -G0 
ASFLAGS = $(INCOPT)           -G0 

#----------- customize section --------------
PROGNAME = rspu2drv

OBJS     = rsd_main.o  rsd_com.o rsd_cb.o 
LIBS = -lsnd2 -lspu2

INIT_OBJ = rsd_main.o

#----------- rules --------------
all:	$(PROGNAME).irx

install: all
	cp $(PROGNAME).irx ../../../modules


clean:
	rm -f *.o $(PROGNAME).irx *.obj *.map

$(PROGNAME).irx: $(OBJS) 
	$(LINK.o) -s -o $@ $(OBJS) $(LIBS)

