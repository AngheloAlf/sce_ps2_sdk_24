#
#  IOP test program Makefile
#
#  $Id: Makefile,v 1.3 2000/05/20 06:49:54 xokano Exp $
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	$(MAKE) all
else
include PathDefs
endif

INCDIR  = -I/usr/local/sce/common/include

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

#----------- customize section --------------
CFLAGS   = $(INCDIR) -Wall -G 0
ASFLAGS  = $(INCDIR)
LDLIBS   = -ilb=cdvdman.ilb -ilb=libsd.ilb
PROGNAME = main.irx

all: $(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map
run: all
	dsifilesv -c "\
	dsreset 0 2;\
	dsistart -nf ../../../modules/libsd.irx;\
	dsistart -nf $(PROGNAME)"

