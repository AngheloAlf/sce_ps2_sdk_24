#
#  IOP test program Makefile
#
#  $Id: Makefile,v 1.2 2001/03/06 10:12:36 xokano Exp $
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
		-ilb=libsd.ilb \
		-ilb=modhsyn.ilb -ilb=modsein.ilb -ilb=modmidi.ilb -o $@

#----------- customize section --------------
CFLAGS   = -Wall -G 0 $(INCDIR)
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
	dsidb bload /usr/local/sce/data/sound/seq/sakana.sq 101000;\
	dsidb bload /usr/local/sce/data/sound/wave/sakana.hd 120000;\
        dsidb bload /usr/local/sce/data/sound/wave/sakana.bd 130000;\
	dsidb bload /usr/local/sce/data/sound/wave/eff.hd 128000;\
        dsidb bload /usr/local/sce/data/sound/wave/eff.bd 190000;\
	dsistart ../../../modules/libsd.irx ;\
	dsistart ../../../modules/modhsyn.irx ;\
	dsistart ../../../modules/modmidi.irx ;\
	dsistart ../../../modules/modsein.irx ;\
	dsistart $(PROGNAME) ;\
	"
