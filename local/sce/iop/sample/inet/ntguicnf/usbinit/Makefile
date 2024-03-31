# SCEI CONFIDENTIAL
# 
# 
# 	GUI Network Setting Application Sample
#		USB AutoLoader Starter
# 
#                          Version 1.2
# 
#      Copyright (C) 2001 Sony Computer Entertainment Inc.
# 	                 All Rights Reserved.
# 
#                          Makefile
# 
#        Version       Date            Design     Log
#  --------------------------------------------------------------------
#        1.1           2001.06.01      tetsu      Beta Release
#        1.2           2001.07.19      tetsu      First Ver. Release
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
LDLIBS   = -ilb=usbmload.ilb
PROGNAME = usbinit.irx
OBJS	 = $(PROGNAME:%.irx=%.o) # add here other objects

all:	$(PROGNAME)

clean:
	rm -f *.o $(PROGNAME) *.map *~ *.log PathDefs

$(PROGNAME:%.irx=%.o) : $(PROGNAME:%.irx=%.c) # add your local include file

$(PROGNAME): $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
