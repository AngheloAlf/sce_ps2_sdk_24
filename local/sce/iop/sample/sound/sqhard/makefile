#
#  IOP test program Makefile
#
#  $Id: makefile,v 1.12 2001/03/06 10:12:00 xokano Exp $
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
endif
include PathDefs

INCDIR  = -I/usr/local/sce/common/include

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ -L../../../lib -L./ $(LOADLIBES) $(LDLIBS) \
	-ilb=libsd.ilb \
	-ilb=modhsyn.ilb -ilb=modmidi.ilb -ilb=moddelay.ilb -o $@

#----------- customize section --------------
CFLAGS   = -Wall -G 0 $(INCDIR)
ASFLAGS  = $(INCDIR)
LDLIBS   =
PROGNAME = main.irx

all: $(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map

run: all
	dsifilesv -c "\
	dsreset 0 2 ;\
	dsidb bload /usr/local/sce/data/sound/seq/sakana.sq 101000;\
	dsidb bload /usr/local/sce/data/sound/wave/sakana.hd 120000;\
        dsidb bload /usr/local/sce/data/sound/wave/sakana.bd 130000;\
	dsidb bload /usr/local/sce/data/sound/seq/overload.sq 104000;\
	dsidb bload /usr/local/sce/data/sound/wave/overload.hd 128000;\
        dsidb bload /usr/local/sce/data/sound/wave/overload.bd 190000;\
	dsistart ../../../modules/libsd.irx ;\
	dsistart ../../../modules/modhsyn.irx ;\
	dsistart ../../../modules/modmidi.irx ;\
	dsistart ../../../modules/moddelay.irx ;\
	dsistart $(PROGNAME) ;\
	"
