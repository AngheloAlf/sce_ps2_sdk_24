#
#  Inet Setting Application Sample's Makefile
#
#  $Id: Makefile,v 1.4 2001/03/19 12:01:13 tetsu Exp $
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	$(MAKE) all
else
include PathDefs
endif

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

#----------- customize section --------------
CFLAGS   =  -Wall -G0
ASFLAGS  =
LDLIBS   = -ilb=inet.ilb -ilb=inetctl.ilb -ilb=netcnf.ilb \
	-ilb=usbmload.ilb
PROGNAME = ifnetcnf.irx
OBJS	 = $(PROGNAME:%.irx=%.o) read_env.o write_env.o update_env.o # add here other objects

all:	$(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map *~ *.log PathDefs

$(PROGNAME:%.irx=%.o) : $(PROGNAME:%.irx=%.c) # add your local include file

$(PROGNAME): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
