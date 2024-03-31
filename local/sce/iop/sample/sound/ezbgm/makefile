ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	make all
else
include PathDefs
endif

INCDIR	= -I/usr/local/sce/common/include

CFLAGS  = $(INCDIR) -I. -Wall -G0 -g
ASFLAGS  = $(INCDIR) -G0

COMPILE.s = $(CC) -xassembler-with-cpp $(ASFLAGS) $(CPPFLAGS) $(TARGET_MACH) -c

#----------- customize section --------------
PROGNAME = ezbgm

OBJS     = bgm_entr.o bgm_com.o bgm_play.o bgm_r2s.o bgm_r2sm.o
#ILIBS	= -ilb=$(TOP)lib/iop.ilb 
ILIBS	=

#----------- rules --------------
all:	$(PROGNAME).irx

clean:
	rm -f *.o $(PROGNAME).irx *.obj *.map

$(PROGNAME).irx: $(OBJS)
	$(LINK.o)  -o $@ \
		$(OBJS) -L../../../lib -L./  -ilb=libsd.ilb  -ilb=cdvdman.ilb  $(ILIBS)

