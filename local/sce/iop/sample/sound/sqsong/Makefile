#
#  IOP test program Makefile
#
#  $Id: Makefile,v 1.3 2001/09/03 05:50:52 xokano Exp $
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
endif
include PathDefs

INCDIR  = /usr/local/sce/common/include
INCOPT  = -I$(INCDIR)

RM = /bin/rm -f

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ -L../../../lib -L./ $(LOADLIBES) $(LDLIBS) \
	-ilb=libsd.ilb \
	-ilb=modhsyn.ilb -ilb=modmidi.ilb -o $@

#----------- customize section --------------
CFLAGS   = -Wall -G 0 $(INCOPT)
ASFLAGS  = $(INCDIR)
LDLIBS   =
PROGNAME = main.irx

all: $(PROGNAME)

clean:
	$(RM) *.o $(PROGNAME) *.map

run: all
	dsifilesv -c "\
	dsreset 0 2 ;\
	dsidb bload /usr/local/sce/data/sound/seq/overlo_s.sq 101000;\
	dsidb bload /usr/local/sce/data/sound/wave/overlo_s.hd 120000;\
        dsidb bload /usr/local/sce/data/sound/wave/overlo_s.bd 130000;\
	dsistart ../../../modules/libsd.irx ;\
	dsistart ../../../modules/modhsyn.irx ;\
	dsistart ../../../modules/modmidi.irx ;\
	dsistart $(PROGNAME) ;\
	"
