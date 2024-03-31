#
#  IOP test program Makefile
#
#  $Id: makefile,v 1.10 2000/11/09 09:35:57 kaol Exp $
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	$(MAKE) all
else
include PathDefs
endif

TOPDIR  = /usr/local/sce
INCDIR  = -I$(TOPDIR)/common/include

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ -L$(TOPDIR)/iop/lib -L./ $(LOADLIBES) $(LDLIBS) \
	-ilb=libsd.ilb \
	-ilb=modssyn.ilb -ilb=modmidi.ilb -o $@

#----------- customize section --------------
CFLAGS   = $(INCDIR) -Wall -G 0 
ASFLAGS  = $(INCDIR)
LDLIBS   = 
PROGNAME = main.irx

all: $(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

run: all
	dsidb -nr "bload demo1.mid.sq 100000;mstart ../../../modules/modmidi.irx;mstart ../../../modules/modssyn.irx;mstart $(PROGNAME)"
