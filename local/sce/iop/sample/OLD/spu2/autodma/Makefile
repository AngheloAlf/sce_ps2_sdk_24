#
#  IOP test program Makefile
#
#  $Id: Makefile,v 1.15 2000/03/02 01:46:07 xokano Exp $
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
LDLIBS   = -lspu2
PROGNAME = main.irx

all: $(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map


run: all
	dsifilesv -c "\
	dsreset 0 2;\
	dsistart -nf $(PROGNAME)"
