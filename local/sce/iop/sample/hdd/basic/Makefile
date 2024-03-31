#
#
#
#

ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	$(MAKE) all
else
include PathDefs
endif

#include $(word 1, $(wildcard Makedefs ../Makedefs ../../Makedefs \
#		../../../Makedefs ) )

IRX      = main.irx
SRCS     = main.c
OBJS    := $(SRCS:%.c=%.o)

CFLAGS   = -g -G 0 -Wall -O2 -I/usr/local/sce/common/include
LDFLAGS  =
EXILBS   = cdvdman.ilb

LOADLIBS = $(LDFLAGS) $(EXILBS:%=-ilb=%)

all:	$(IRX)

$(IRX): $(OBJS)
	$(LINK.o) -o $(@:.irx=.sym) $^ $(LOADLIBS)
	$(IFIXUP) -o $@ $(@:.irx=.sym)

clean:
	rm -f *.o *.irx *.sym PathDefs

run: $(IRX)
	dsreset 0 2
	dsistart $(IRX)
