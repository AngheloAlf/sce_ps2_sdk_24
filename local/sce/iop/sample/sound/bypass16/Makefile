#
#  IOP test program Makefile
#
#  $Id: Makefile,v 1.1 2001/05/25 11:15:37 kaol Exp $
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
endif
include PathDefs

INCDIR  = -I../../../include -I../../../../common/include \
	  -I/usr/local/sce/include -I/usr/local/sce/common/include

RM = /bin/rm -f

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ -L../../../lib -L./ $(LOADLIBES) $(LDLIBS) \
		-ilb=libsd.ilb -o $@

#----------- customize section --------------
CFLAGS   = -Wall -G 0 -O2 $(INCDIR)
ASFLAGS  = $(INCDIR)
LDLIBS   =
PROGNAME = main.irx

all: $(PROGNAME)

clean:
	$(RM) *.o $(PROGNAME) *.map

clear:
	$(RM) *~

run: all
	dsifilesv -c "\
	dsreset 0 2 ;\
	dsistart ../../../modules/libsd.irx ;\
	dsistart $(PROGNAME) ;\
	"
