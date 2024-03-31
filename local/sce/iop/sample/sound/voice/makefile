#
#  IOP test program Makefile
#
#  $Id: makefile,v 1.10 2001/02/06 22:13:05 kaol Exp $
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	$(MAKE) all
else
include PathDefs
endif

INCDIR  = -I../../../../common/include -I../../../include \
	-I/usr/local/sce/common/include

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ -L../../../lib -L./ $(LOADLIBES) $(LDLIBS) \
	-ilb=libsd.ilb -o $@

#----------- customize section --------------
CFLAGS   = $(INCDIR) -Wall -G 0
ASFLAGS  = $(INCDIR)
LDLIBS   = 
PROGNAME = main.irx

all: $(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

run: all
	dsifilesv -c " dsreset 0 2; dsistart -nf ../../../modules/libsd.irx; dsistart -nf $(PROGNAME);"

